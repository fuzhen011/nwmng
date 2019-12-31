/*************************************************************************
    > File Name: cfgdb.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
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
/* Get the hash table by address and template ID fields */
#define __HTB(addr, tmpl)                       \
  ((addr) ? db.devdb.nodes                      \
   : ((tmpl) && *(tmpl)) ? db.devdb.unprov_devs \
   : db.devdb.backlog)
#define _HTB(addr) ((addr) ? db.devdb.nodes : db.devdb.unprov_devs)

#define G_KEY(n) ((n)->addr == 0 ? UNPROV_DEV_KEY((n)) : NODE_KEY((n)))
#define G_HTB(n) (__HTB(n->addr, n->tmpl))
/* Static Variables *************************************************** */
static cfgdb_t db = { 0 };

/* Static Functions Declaractions ************************************* */
void dump__(gpointer key,
            gpointer value,
            gpointer ud)
{
  LOGM("%d key found.\n", *(uint8_t *)key);
}

void cfgdb_test(void)
{
  g_hash_table_foreach(db.devdb.templates, dump__, NULL);
}

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
  SAFE_FREE(t->relay_txp);
  SAFE_FREE(t->pub);
  SAFE_FREE(t->bindings);
  SAFE_FREE(t->sublist);
  SAFE_FREE(t);
}

guint u16_hash(gconstpointer key)
{
  return (guint)(*(uint16_t *)key);
}

gboolean u16_equal(gconstpointer a, gconstpointer b)
{
  return ((*(uint16_t *)a) == (*(uint16_t *)b));
}

err_t cfgdb_init(void)
{
  if (db.initialized) {
    return ec_success;
  }

  /* Initialize the device database */
  /* TODO: Is it OK to use direct hash for both case - str and integer? While
   * using str hash, need to check if it requires the str ends with '\0' */
  db.devdb.unprov_devs = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, node_free);
  db.devdb.nodes = g_hash_table_new_full(u16_hash, u16_equal, NULL, node_free);
  db.devdb.templates = g_hash_table_new_full(u16_hash, u16_equal, NULL, tmpl_free);
  db.devdb.lights = g_queue_new();
  db.devdb.backlog = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, node_free);
  db.initialized = 1;

  return ec_success;
}

void cfgdb_deinit(void)
{
  if (!db.initialized) {
    return;
  }
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
    g_hash_table_remove_all(db.devdb.backlog);
    g_hash_table_unref(db.devdb.backlog);
    db.devdb.backlog = NULL;
  }
  if (db.devdb.lights) {
    g_queue_free_full(db.devdb.lights, free);
    db.devdb.lights = NULL;
  }
  if (db.devdb.unprov_devs) {
    g_hash_table_remove_all(db.devdb.unprov_devs);
    g_hash_table_unref(db.devdb.unprov_devs);
    db.devdb.unprov_devs = NULL;
  }
  if (db.devdb.nodes) {
    g_hash_table_remove_all(db.devdb.nodes);
    g_hash_table_unref(db.devdb.nodes);
    db.devdb.nodes = NULL;
  }
  if (db.devdb.templates) {
    g_hash_table_remove_all(db.devdb.templates);
    g_hash_table_unref(db.devdb.templates);
    db.devdb.templates = NULL;
  }
  SAFE_FREE(db.self.net_txp);
  SAFE_FREE(db.self.subnets);
  SAFE_FREE(db.self.ttl);
  memset(&db.self, 0, sizeof(provcfg_t));
  db.initialized = 0;
}

int cfgdb_contains(const node_t *n)
{
  CHECK_STATE(0);
  if (!n) {
    return 0;
  }
  return g_hash_table_contains(G_HTB(n), G_KEY(n));
}

int cfgdb_devnum(bool proved)
{
  CHECK_STATE(0);
  return g_hash_table_size(_HTB(proved));
}

node_t *cfgdb_node_get(uint16_t addr)
{
  CHECK_NULL_RET();
  return (node_t *)g_hash_table_lookup(db.devdb.nodes,
                                       KEY_FROM_ADDR(addr));
}

node_t *cfgdb_unprov_dev_get(const uint8_t *uuid)
{
  CHECK_NULL_RET();
  if (!uuid) {
    return NULL;
  }
  return (node_t *)g_hash_table_lookup(db.devdb.unprov_devs,
                                       KEY_FROM_UUID(uuid));
}

node_t *cfgdb_backlog_get(const uint8_t *uuid)
{
  CHECK_NULL_RET();
  if (!uuid) {
    return NULL;
  }
  return (node_t *)g_hash_table_lookup(db.devdb.backlog,
                                       KEY_FROM_UUID(uuid));
}

static node_t *__cfgdb_get(node_t *n)
{
  return (node_t *)g_hash_table_lookup(G_HTB(n), G_KEY(n));
}

/* err_t cfgdb_tmpl_add(tmpl_t *t) */
/* { */
/* } */

tmpl_t *cfgdb_tmpl_get(uint16_t refid)
{
  return (tmpl_t *)g_hash_table_lookup(db.devdb.templates, TMPL_KEY(refid));
}

err_t cfgdb_tmpl_remove(tmpl_t *n)
{
  CHECK_STATE(ec_state);
  if (!n) {
    return err(ec_param_invalid);
  }
  g_hash_table_remove(db.devdb.templates, TMPL_KEY(n->refid));
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
    /* key n->addr already has a value in hash table and the value doesn't equal
     * to n, need to remove and free it first, then add */
    e = cfgdb_tmpl_remove(tmp);
    if (ec_success != e) {
      return e;
    }
  } else if (n == tmp) {
    return ec_success;
  }

  g_hash_table_insert(db.devdb.templates, TMPL_KEY(n->refid), n);
  return ec_success;
}

err_t cfgdb_add(node_t *n)
{
  err_t e;
  node_t *tmp;
  CHECK_STATE(ec_state);
  if (!n) {
    return err(ec_param_invalid);
  }
  /* Check if it's already in? */
  tmp = __cfgdb_get(n);
  if (tmp && tmp != n) {
    /* key n->addr already has a value in hash table and the value doesn't equal
     * to n, need to remove and free it first, then add */
    e = cfgdb_remove(tmp, 1);
    if (ec_success != e) {
      return e;
    }
  } else if (n == tmp) {
    return ec_success;
  }

  g_hash_table_insert(G_HTB(n), G_KEY(n), n);
  /* TODO: Also update the lists */
  return ec_success;
}

err_t cfgdb_remove(node_t *n, bool destory)
{
  CHECK_STATE(ec_state);
  if (!n) {
    return err(ec_param_invalid);
  }
  if (destory) {
    g_hash_table_remove(G_HTB(n), G_KEY(n));
  } else {
    g_hash_table_steal(G_HTB(n), G_KEY(n));
  }
  /* TODO: Also update the lists */
  return ec_success;
}

provcfg_t *get_provcfg(void)
{
  return &db.self;
}

void set_provcfg(const provcfg_t *src)
{
}

static int offs = 0;
void copy_addr_to_user(gpointer key,
                       gpointer value,
                       gpointer ud)
{
  uint16_t addr = *(uint16_t *)key;
  ((uint16_t *)ud)[offs++] = addr;
}

int get_ng_addrs(uint16_t *addrs)
{
  offs = 0;
  g_hash_table_foreach(db.devdb.nodes, copy_addr_to_user, addrs);
  return offs;
}

void cfg_load_mnglists(GHFunc func)
{
  g_hash_table_foreach(db.devdb.unprov_devs, func, NULL);
  g_hash_table_foreach(db.devdb.nodes, func, NULL);
}
