/*************************************************************************
    > File Name: dev_add.c
    > Author: Kevin
    > Created Time: 2019-12-26
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
/* #include "dev_add.h" */
#include <glib.h>

#include "cfg.h"
#include "mng.h"
#include "utils.h"
#include "logging.h"
#include "cli.h"
#include "generic_parser.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static void on_beacon_recv(const struct gecko_msg_mesh_prov_unprov_beacon_evt_t *evt);
static void on_prov_failed(const struct gecko_msg_mesh_prov_provisioning_failed_evt_t *evt);
static void on_prov_success(const struct gecko_msg_mesh_prov_device_provisioned_evt_t *evt);

static inline int iscached(const mng_t *mng,
                           const uint8_t *uuid,
                           int *free)
{
  if (free) {
    *free = -1;
  }
  for (int i = 0; i < MAX_PROV_SESSIONS; i++) {
    if (mng->cache.add[i].busy) {
      if (!memcmp(mng->cache.add[i].uuid, uuid, 16)) {
        return i;
      }
      continue;
    }
    if (free && *free == -1) {
      *free = i;
    }
  }
  return -1;
}

static inline void rmcached(mng_t *mng,
                            const uint8_t *uuid)
{
  for (int i = 0; i < MAX_PROV_SESSIONS; i++) {
    if (!mng->cache.add[i].busy
        || memcmp(mng->cache.add[i].uuid, uuid, 16)) {
      continue;
    }
    memset(&mng->cache.add[i], 0, sizeof(add_cache_t));
  }
}

/* only pass the prov class events */
static inline bool is_add_events(const struct gecko_cmd_packet *e)
{
  uint32_t evt_id = BGLIB_MSG_ID(e->header);
  return ((evt_id & 0x000000ff) == 0x000000a0
          && (evt_id & 0x00ff0000) == 0x00150000);
}

int dev_add_hdr(const struct gecko_cmd_packet *evt)
{
  mng_t *mng = get_mng();
  ASSERT(evt);

  if (!is_add_events(evt)) {
    return 0;
  }

  if (mng->state != adding_devices_em && mng->status.free_mode != 2) {
    return 0;
  }

  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_mesh_prov_unprov_beacon_id:
    {
      on_beacon_recv(&evt->data.evt_mesh_prov_unprov_beacon);
    }
    break;
    case gecko_evt_mesh_prov_device_provisioned_id:
    {
      on_prov_success(&evt->data.evt_mesh_prov_device_provisioned);
    }
    break;
    case gecko_evt_mesh_prov_provisioning_failed_id:
    {
      on_prov_failed(&evt->data.evt_mesh_prov_provisioning_failed);
    }
    break;

    default:
      return 0;
  }
  return 1;
}

static void on_beacon_recv(const struct gecko_msg_mesh_prov_unprov_beacon_evt_t *evt)
{
  int freeid;
  uint16_t ret;
  mng_t *mng = get_mng();
  node_t *n;

  ASSERT(evt);

  if (evt->bearer == 1) {
    /* PB-GATT not support for now */
    return;
  }

  if (-1 != iscached(mng, evt->uuid.data, &freeid)) {
    return;
  }

  if (freeid == -1) {
    return;
  }

  n = cfgdb_unprov_dev_get(evt->uuid.data);
  if (!n) {
    if (NULL == cfgdb_backlog_get(evt->uuid.data)) {
      err_t e = backlog_dev(evt->uuid.data);
      if (ec_success == e) {
        char uuid_str[33] = { 0 };
        cbuf2str((char *)evt->uuid.data, 16, 0, uuid_str, 33);
        bt_shell_printf("Add new device (UUID:%s) to backlog\n", uuid_str);
        LOGM("Add new device (UUID:%s) to backlog\n", uuid_str);
      } else {
        elog(e);
      }
    }
    return;
  } else if (n->rmorbl) {
    return;
  }

  LOGM("Unprovisioned beacon match. Start provisioning it\n");
  ret = gecko_cmd_mesh_prov_provision_device(mng->cfg->subnets[0].netkey.id,
                                             16,
                                             evt->uuid.data)->result;
  if (bg_err_success != ret) {
    LOGBGE("provision device", ret);
    return;
  }
  mng->cache.add[freeid].busy = 1;
  memcpy(mng->cache.add[freeid].uuid, evt->uuid.data, 16);
}

static void on_prov_success(const struct gecko_msg_mesh_prov_device_provisioned_evt_t *evt)
{
  err_t e;
  mng_t *mng = get_mng();
  node_t *n;
  char uuid_str[33] = { 0 };
  cbuf2str((char *)evt->uuid.data, 16, 0, uuid_str, 33);
  LOGM("%s Provisioned\n", uuid_str);

  /* inform cfg to move the device from unprovisioned list to node list,
   * meanwhile, set the address */
  e = upl_nodeset_addr(evt->uuid.data, evt->address);
  elog(e);

  /* move the node from add list to config list */
  n = cfgdb_node_get(evt->address);
  ASSERT(n);
  mng->lists.add = g_list_remove(mng->lists.add, n);
  mng->lists.config = g_list_append(mng->lists.config, n);

  /* Remove from cache. */
  rmcached(mng, evt->uuid.data);
}

static void on_prov_failed(const struct gecko_msg_mesh_prov_provisioning_failed_evt_t *evt)
{
  char uuid_str[33] = { 0 };
  cbuf2str((char *)evt->uuid.data, 16, 0, uuid_str, 33);
  LOGE("%s Provisioned FAIL, reason[%u]\n", uuid_str, evt->reason);
  /*
   * TODO:
   *
   * 1. Remove from cache.
   * 2. Set the error bits to cfg
   */
  /* Remove from cache. */
  rmcached(get_mng(), evt->uuid.data);
}
