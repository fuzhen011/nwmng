/*************************************************************************
    > File Name: cfgdb.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>

#include "projconfig.h"
#include "cfgdb.h"
#include "err.h"
#if 0
#include "shash.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
#define CHECK_STATE(ret) do { if (!devs) { return err((ret)); } } while (0)
#define CHECK_VOID_RET() do { if (!devs) { return; } } while (0)
/* Static Variables *************************************************** */
static htb_t *devs = NULL;

/* Static Functions Declaractions ************************************* */
static unsigned int addrhash(const void *key)
{
  return (unsigned int)(*(uint16_t *)(key));
}

err_t cfgdb_init(void)
{
  if (devs) {
    return ec_success;
  }

  devs = new_htb(addrhash, GC_EXP_DEV_NUM);

  return ec_success;
}

void cfgdb_deinit(void)
{
  htb_destory(devs);
}

err_t cfgdb_add(node_t *n)
{
  CHECK_STATE(ec_state);
  if (!n || n->addr == 0) {
    return err(ec_param_invalid);
  }
  htb_replace(devs, (void *)&n->addr, n);
  return ec_success;
}

err_t cfgdb_del(node_t *n)
{
  CHECK_STATE(ec_state);
  if (!n || n->addr == 0) {
    return err(ec_param_invalid);
  }
  htb_remove(devs, (void *)&n->addr);
  return ec_success;
}
#else
#include <glib.h>
typedef struct {
  GHashTable *devs;
  GList *lights;
} cfgdb_t;

static cfgdb_t db = { NULL, NULL };

err_t cfgdb_init(void)
{
  if (db.devs || db.lights) {
    return ec_success;
  }

  db.devs = g_hash_table_new(NULL, NULL);
  db.lights = NULL;
  return ec_success;
}

void cfgdb_deinit(void)
{

}
#endif
