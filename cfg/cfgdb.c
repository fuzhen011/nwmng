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

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
#define CHECK_STATE(ret) \
  do { if (!db.initialized) { return err((ret)); } } while (0)
#define CHECK_NULL_RET() \
  do { if (!db.initialized) { return NULL; } } while (0)
#define CHECK_VOID_RET() \
  do { if (!db.initialized) { return; } } while (0)

#define KEY_FROM_UUID(uuid) ((gpointer)(uuid))
#define UNPROV_DEV_KEY(n) (KEY_FROM_UUID((n)->uuid))
#define KEY_FROM_ADDR(addr) ((gpointer)(&addr))
#define NODE_KEY(n) (KEY_FROM_ADDR(n->addr))
#define G_KEY(n) ((n)->addr == 0 ? UNPROV_DEV_KEY((n)) : NODE_KEY((n)))
#define __HTB(x) ((x) ? db.devdb.nodes : db.devdb.unprov_devs)
#define G_HTB(n) (__HTB(n->addr))
/* Static Variables *************************************************** */
static cfgdb_t db = { 0 };

/* Static Functions Declaractions ************************************* */

err_t cfgdb_init(void)
{
  if (db.initialized) {
    return ec_success;
  }

  /* Initialize the device database */
  /* TODO: Is it OK to use direct hash for both case - str and integer? While
   * using str hash, need to check if it requires the str ends with '\0' */
  db.devdb.unprov_devs = g_hash_table_new_full(NULL, NULL, NULL, free);
  db.devdb.nodes = g_hash_table_new_full(NULL, NULL, NULL, free);
  db.devdb.lights = g_queue_new();
  db.devdb.backlog = g_queue_new();

  return ec_success;
}

void cfgdb_deinit(void)
{
  if (!db.initialized) {
    return;
  }
  if (db.self.pubgroups) {
    g_list_free_full(db.self.pubgroups, free);
    db.self.pubgroups = NULL;
  }
  if (db.self.subgroups) {
    g_list_free_full(db.self.subgroups, free);
    db.self.pubgroups = NULL;
  }
  if (db.self.subnets) {
    free(db.self.subnets);
    db.self.subnets = NULL;
  }
  if (db.devdb.backlog) {
    g_queue_free_full(db.devdb.backlog, free);
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
  return g_hash_table_size(__HTB(proved));
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

static node_t *__cfgdb_get(node_t *n)
{
  return (node_t *)g_hash_table_lookup(G_HTB(n), G_KEY(n));
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
  if (!n || n->addr == 0) {
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
