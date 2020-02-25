/*************************************************************************
    > File Name: cfgdb.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "projconfig.h"
#include "err.h"
#include "cfgdb.h"
#include "logging.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
#define CHECK_STATE(ret) \
  do { if (!db.initialized) { return err((ret)); } } while (0)
#define CHECK_NULL_RET() \
  do { if (!db.initialized) { return NULL; } } while (0)
#define CHECK_VOID_RET() \
  do { if (!db.initialized) { return; } } while (0)

/* Unprovisioned and backlog devices use UUID as key */
#define KEY_FROM_UUID(uuid) ((gpointer)(uuid))
#define UNPROV_DEV_KEY(n) (KEY_FROM_UUID((n)->uuid))
#define BACKLOG_DEV_KEY(n) (KEY_FROM_UUID((n)->uuid))

/* Provisioned nodes use unicast address as key */
#define KEY_FROM_ADDR(addr) ((gpointer)(&addr))
#define NODE_KEY(n) (KEY_FROM_ADDR(n->addr))
#define TMPL_KEY(refid) ((gpointer)(&(refid)))

/* Get the tree by address and template ID fields */
#define __HTB(addr, tmpl)                       \
  ((addr) ? db.devdb.nodes                      \
   : ((tmpl) && *(tmpl)) ? db.devdb.unprov_devs \
   : db.devdb.backlog)

#define G_KEY(n) ((n)->addr == 0 ? UNPROV_DEV_KEY((n)) : NODE_KEY((n)))
#define G_HTB(n) (__HTB(n->addr, n->tmpl))

/* Static Variables *************************************************** */
static cfgdb_t db = { 0 };

/* Static Functions Declaractions ************************************* */
static void node_free(void *p)
{
  if (!p) {
    return;
  }
  node_t *n = (node_t *)p;
  SAFE_FREE(n->tmpl);
  SAFE_FREE(n->config.ttl);
  SAFE_FREE(n->config.snb);
  SAFE_FREE(n->config.net_txp);
  SAFE_FREE(n->config.features.relay_txp);
  SAFE_FREE(n->config.pub);
  SAFE_FREE(n->config.bindings);
  SAFE_FREE(n->config.sublist);
  SAFE_FREE(n);
}

static void tmpl_free(void *p)
{
  if (!p) {
    return;
  }
  tmpl_t *t = (tmpl_t *)p;
  SAFE_FREE(t->ttl);
  SAFE_FREE(t->snb);
  SAFE_FREE(t->net_txp);
  SAFE_FREE(t->features.relay_txp);
  SAFE_FREE(t->pub);
  SAFE_FREE(t->bindings);
  SAFE_FREE(t->sublist);
  SAFE_FREE(t);
}

static gint u16_comp(gconstpointer a, gconstpointer b, gpointer user_data)
{
  return (*(uint16_t *)a == *(uint16_t *)b ? 0
          : *(uint16_t *)a > *(uint16_t *)b ? 1 : -1);
}

static gint uuid_comp(gconstpointer a, gconstpointer b, gpointer user_data)
{
  return memcmp(a, b, 16);
}

err_t cfgdb_init(void)
{
  int ret;
  if (db.initialized) {
    return ec_success;
  }

  if (0 != (ret = pthread_rwlock_init(&db.lock, NULL))) {
    err_exit_en(ret, "pthread_rwlock_init");
  }
  /* Initialize the device database */
  db.devdb.unprov_devs = g_tree_new_full(uuid_comp, NULL, NULL, node_free);
  db.devdb.nodes = g_tree_new_full(u16_comp, NULL, NULL, node_free);
  db.devdb.templates = g_tree_new_full(u16_comp, NULL, NULL, tmpl_free);
  db.devdb.backlog = g_tree_new_full(uuid_comp, NULL, NULL, node_free);
  db.initialized = 1;
  return ec_success;
}

void cfgdb_deinit(void)
{
  if (!db.initialized) {
    return;
  }
  pthread_rwlock_destroy(&db.lock);

  if (db.devdb.pubgroups) {
    g_list_free_full(db.devdb.pubgroups, free);
    db.devdb.pubgroups = NULL;
  }
  if (db.devdb.subgroups) {
    g_list_free_full(db.devdb.subgroups, free);
    db.devdb.pubgroups = NULL;
  }
  if (db.self.subnets) {
    free(db.self.subnets);
    db.self.subnets = NULL;
  }
  if (db.devdb.backlog) {
    g_tree_destroy(db.devdb.backlog);
    db.devdb.backlog = NULL;
  }
  if (db.devdb.unprov_devs) {
    g_tree_destroy(db.devdb.unprov_devs);
    db.devdb.unprov_devs = NULL;
  }
  if (db.devdb.nodes) {
    g_tree_destroy(db.devdb.nodes);
    db.devdb.nodes = NULL;
  }
  if (db.devdb.templates) {
    g_tree_destroy(db.devdb.templates);
    db.devdb.templates = NULL;
  }
  SAFE_FREE(db.self.net_txp);
  SAFE_FREE(db.self.subnets);
  SAFE_FREE(db.self.ttl);
  memset(&db.self, 0, sizeof(provcfg_t));
  db.initialized = 0;
}

int cfgdb_get_devnum(int which)
{
  int ret;
  CHECK_STATE(0);
  pthread_rwlock_rdlock(&db.lock);
  switch (which) {
    case tmpl_em:
      ret = g_tree_nnodes(db.devdb.templates);
      break;
    case upl_em:
      ret = g_tree_nnodes(db.devdb.unprov_devs);
      break;
    case nodes_em:
      ret = g_tree_nnodes(db.devdb.nodes);
      break;
    case backlog_em:
      ret = g_tree_nnodes(db.devdb.backlog);
      break;
    default:
      ret = 0;
  }
  pthread_rwlock_unlock(&db.lock);
  return ret;
}

node_t *cfgdb_node_get(uint16_t addr)
{
  CHECK_NULL_RET();
  return (node_t *)g_tree_lookup(db.devdb.nodes, KEY_FROM_ADDR(addr));
}

node_t *cfgdb_unprov_dev_get(const uint8_t *uuid)
{
  CHECK_NULL_RET();
  if (!uuid) {
    return NULL;
  }
  return (node_t *)g_tree_lookup(db.devdb.unprov_devs, KEY_FROM_UUID(uuid));
}

node_t *cfgdb_backlog_get(const uint8_t *uuid)
{
  CHECK_NULL_RET();
  if (!uuid) {
    return NULL;
  }
  return (node_t *)g_tree_lookup(db.devdb.backlog, KEY_FROM_UUID(uuid));
}

tmpl_t *cfgdb_tmpl_get(uint16_t refid)
{
  return (tmpl_t *)g_tree_lookup(db.devdb.templates, TMPL_KEY(refid));
}

err_t cfgdb_tmpl_remove(tmpl_t *n)
{
  CHECK_STATE(ec_state);
  if (!n) {
    return err(ec_param_invalid);
  }
  g_tree_remove(db.devdb.templates, TMPL_KEY(n->refid));
  return ec_success;
}

err_t cfgdb_tmpl_add(tmpl_t *n)
{
  err_t e;
  tmpl_t *tmp;
  CHECK_STATE(ec_state);
  if (!n) {
    return err(ec_param_invalid);
  }
  /* Check if it's already in? */
  tmp = cfgdb_tmpl_get(n->refid);
  if (tmp && tmp != n) {
    /* key n->addr already has a value in tree and the value doesn't equal
     * to n, need to remove and free it first, then add */
    e = cfgdb_tmpl_remove(tmp);
    if (ec_success != e) {
      return e;
    }
  } else if (n == tmp) {
    return ec_success;
  }

  g_tree_insert(db.devdb.templates, TMPL_KEY(n->refid), n);
  return ec_success;
}

static err_t __cfgdb_remove(node_t *n, GTree *tree, bool destory)
{
  CHECK_STATE(ec_state);
  if (!n || !tree) {
    return err(ec_param_invalid);
  }
  pthread_rwlock_wrlock(&db.lock);
  if (destory) {
    g_tree_remove(G_HTB(n), G_KEY(n));
  } else {
    g_tree_steal(G_HTB(n), G_KEY(n));
  }
  pthread_rwlock_unlock(&db.lock);
  return ec_success;
}

static err_t __cfgdb_add(node_t *n, GTree *tree)
{
  err_t e;
  node_t *node;
  CHECK_STATE(ec_state);
  if (!n || !tree) {
    return err(ec_param_invalid);
  }
  /* Check if it's already in? */
  pthread_rwlock_rdlock(&db.lock);
  node = g_tree_lookup(tree, G_KEY(n));
  pthread_rwlock_unlock(&db.lock);
  if (node && node != n) {
    /* key n->addr already has a value in tree and the value doesn't equal
     * to n, need to remove and free it first, then add */
    if (ec_success != (e = __cfgdb_remove(node, tree, 1))) {
      return e;
    }
  } else if (n == node) {
    return ec_success;
  }

  pthread_rwlock_wrlock(&db.lock);
  g_tree_insert(tree, G_KEY(n), n);
  pthread_rwlock_unlock(&db.lock);

  /* TODO: Also update the lists */
  return ec_success;
}

err_t cfgdb_backlog_add(node_t *n)
{
  return __cfgdb_add(n, db.devdb.backlog);
}

err_t cfgdb_unpl_add(node_t *n)
{
  return __cfgdb_add(n, db.devdb.unprov_devs);
}

err_t cfgdb_nodes_add(node_t *n)
{
  return __cfgdb_add(n, db.devdb.nodes);
}

err_t cfgdb_backlog_remove(node_t *n, bool destory)
{
  return __cfgdb_remove(n, db.devdb.backlog, destory);
}

err_t cfgdb_unpl_remove(node_t *n, bool destory)
{
  return __cfgdb_remove(n, db.devdb.unprov_devs, destory);
}

err_t cfgdb_nodes_remove(node_t *n, bool destory)
{
  return __cfgdb_remove(n, db.devdb.nodes, destory);
}

provcfg_t *get_provcfg(void)
{
  return &db.self;
}

void cfgdb_remove_all_upl(void)
{
  CHECK_VOID_RET();
  pthread_rwlock_wrlock(&db.lock);
  g_tree_destroy(db.devdb.unprov_devs);
  db.devdb.unprov_devs = g_tree_new_full(uuid_comp, NULL, NULL, node_free);
  pthread_rwlock_unlock(&db.lock);
}

void cfgdb_remove_all_nodes(void)
{
  CHECK_VOID_RET();
  pthread_rwlock_wrlock(&db.lock);
  g_tree_destroy(db.devdb.nodes);
  db.devdb.nodes = g_tree_new_full(u16_comp, NULL, NULL, node_free);
  pthread_rwlock_unlock(&db.lock);
}

void set_provcfg(const provcfg_t *src)
{
}

static int offs = 0;
static uint8_t kind = 0;
gboolean copy_addr_to_user(gpointer key,
                           gpointer value,
                           gpointer ud)
{
  uint16_t addr = *(uint16_t *)key;
  ((uint16_t *)ud)[offs++] = addr;
  return FALSE;
}

gboolean copy_light_addr_to_user(gpointer key,
                                 gpointer value,
                                 gpointer ud)
{
  node_t *n = (node_t *)value;
  if (n->models.func & kind) {
    ((uint16_t *)ud)[offs++] = n->addr;
  }
  return FALSE;
}

uint16list_t *get_node_addrs(void)
{
  uint16list_t *addr;
  int num;

  num = cfgdb_get_devnum(nodes_em);
  if (!num) {
    return NULL;
  }
  addr = calloc(1, sizeof(uint16list_t));
  addr->data = calloc(num, sizeof(uint16_t));

  addr->len = num;
  offs = 0;
  pthread_rwlock_rdlock(&db.lock);
  g_tree_foreach(db.devdb.nodes, copy_addr_to_user, addr->data);
  pthread_rwlock_unlock(&db.lock);
  return addr;
}

uint16list_t *get_lights_addrs(uint8_t func)
{
  uint16list_t *addr;
  int num;
  if (!func) {
    return NULL;
  }

  num = cfgdb_get_devnum(nodes_em);
  if (!num) {
    return NULL;
  }
  addr = calloc(1, sizeof(uint16list_t));
  addr->data = calloc(num, sizeof(uint16_t));

  offs = 0;
  kind = func;
  pthread_rwlock_rdlock(&db.lock);
  g_tree_foreach(db.devdb.nodes, copy_light_addr_to_user, addr->data);
  pthread_rwlock_unlock(&db.lock);
  addr->len = offs;
  return addr;
}

void cfg_load_mnglists(GTraverseFunc func)
{
  g_tree_foreach(db.devdb.unprov_devs, func, NULL);
  g_tree_foreach(db.devdb.nodes, func, NULL);
}
