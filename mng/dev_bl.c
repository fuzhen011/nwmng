/*************************************************************************
    > File Name: dev_bl.c
    > Author: Kevin
    > Created Time: 2019-12-31
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "mng.h"
#include "logging.h"
#include "utils.h"
#include "generic_parser.h"
#include "cli.h"
#include "cfg.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static void bl_result(void);
static void on_bl_done(void);
static void kr_node_update(const struct gecko_msg_mesh_prov_key_refresh_node_update_evt_t *e);
static void kr_nwk_update(const struct gecko_msg_mesh_prov_key_refresh_phase_update_evt_t *e);
static void kr_finish(const struct gecko_msg_mesh_prov_key_refresh_complete_evt_t *e);

static inline bool __is_bl_active(mng_t *mng)
{
  return (g_list_length(mng->lists.bl) || mng->cache.bl.state != bl_idle);
}

static int __bl_till_oom(mng_t *mng)
{
  uint16_t ret = 0;
  int num = 0;
  if (!g_list_length(mng->lists.bl)) {
    return 0;
  }

  do {
    node_t *n = (node_t *)g_list_nth_data(mng->lists.bl, mng->cache.bl.offset);
    if (bg_err_success != (ret = gecko_cmd_mesh_prov_set_key_refresh_blacklist(
                             mng->cfg->subnets[0].netkey.id,
                             1,
                             16,
                             n->uuid)->result)) {
      LOGBGE("blacklist", ret);
      break;
    }
    mng->cache.bl.offset++;
    num++;
  } while (ret == bg_err_success && mng->cache.bl.offset != g_list_length(mng->lists.bl));
  if (num) {
    LOGD("Sent %d node(s) to stack to blacklist.\n", num);
  }
  return num;
}

static err_t kr_start(mng_t *mng)
{
  int ret = 0;
  const int numAppKeys = 0; /* TODO: Not update app key, hard coded to 0 for now */

  if (bg_err_success != (ret = gecko_cmd_mesh_prov_key_refresh_start(
                           mng->cfg->subnets[0].netkey.id,
                           numAppKeys,
                           16 * numAppKeys,
                           NULL)->result)) {
    LOGBGE("kr start", ret);
    return err(ec_bgrsp);
  } else {
    LOGM("Key Refresh Started\n");
  }
  return ec_success;
}

static void load_rem_nodes(void)
{
  mng_t *mng = get_mng();
  uint16list_t *l = get_node_addrs();
  if (!l) {
    return;
  }
  int ofs = 0;
  for (int i = 0; i < l->len; i++) {
    node_t *n = cfgdb_node_get(l->data[i]);
    if (g_list_find(mng->lists.bl, n)) {
      continue;
    }
    mng->cache.bl.rem.nodes[ofs].n = n;
    mng->cache.bl.rem.nodes[ofs].phase = 0xff;
    ofs++;
  }
  ASSERT(mng->cache.bl.rem.num == ofs);
  free(l->data);
  free(l);
}

bool bl_loop(void *p)
{
  err_t e;
  mng_t *mng = (mng_t *)p;
  bool busy = false;

  if (!g_list_length(mng->lists.bl) && mng->cache.bl.state == bl_idle) {
    return false;
  }

  if (mng->cache.bl.state == bl_idle) {
    int num = cfgdb_get_devnum(nodes_em) - g_list_length(mng->lists.bl);
    if (!mng->cache.bl.rem.nodes && num) {
      mng->cache.bl.rem.num = num;
      mng->cache.bl.rem.nodes = calloc(num, sizeof(remainig_nodes_t));
      load_rem_nodes();
    }
    __bl_till_oom(mng);
    if (mng->cache.bl.offset == g_list_length(mng->lists.bl)) {
      mng->cache.bl.state = bl_starting;
    } else {
      mng->cache.bl.state = bl_prepare;
    }
    busy = true;
  } else if (mng->cache.bl.state == bl_prepare) {
    if (!__bl_till_oom(mng) || mng->cache.bl.offset == g_list_length(mng->lists.bl)) {
      mng->cache.bl.state = bl_starting;
    }
  } else if (mng->cache.bl.state == bl_starting) {
    ASSERT(mng->cache.bl.offset != mng->cache.bl.tail);
    if (ec_success != (e = kr_start(mng))) {
      /* handle error */
      TODOASSERT();
    }
    mng->cache.bl.state = bl_busy;
    busy = true;
  } else if (mng->cache.bl.state == bl_done) {
    bl_result();
    on_bl_done();
  }
  return busy;
}

int bl_hdr(const struct gecko_cmd_packet *e)
{
  ASSERT(e);
  mng_t *mng = get_mng();

  if (!__is_bl_active(mng)) {
    return 0;
  }

  switch (BGLIB_MSG_ID(e->header)) {
    case gecko_evt_mesh_prov_key_refresh_node_update_id:
      kr_node_update(&e->data.evt_mesh_prov_key_refresh_node_update);
      break;
    case gecko_evt_mesh_prov_key_refresh_phase_update_id:
      kr_nwk_update(&e->data.evt_mesh_prov_key_refresh_phase_update);
      break;
    case gecko_evt_mesh_prov_key_refresh_complete_id:
      kr_finish(&e->data.evt_mesh_prov_key_refresh_complete);
      break;

    default:
      return 0;
  }
  return 1;
}

static remainig_nodes_t *find_bl_node(const mng_t *mng, const uint8_t *uuid)
{
  if (!mng->cache.bl.rem.nodes) {
    return NULL;
  }
  for (int i = 0; i < mng->cache.bl.rem.num; i++) {
    if (memcmp(mng->cache.bl.rem.nodes[i].n->uuid, uuid, 16)) {
      continue;
    }
    return &mng->cache.bl.rem.nodes[i];
  }
  return NULL;
}

static void kr_node_update(const struct gecko_msg_mesh_prov_key_refresh_node_update_evt_t *e)
{
  mng_t *mng = get_mng();
  remainig_nodes_t *bln;

  bln = find_bl_node(mng, e->uuid.data);
  if (!bln) {
    char uuid_str[33] = { 0 };
    fmt_uuid(uuid_str, e->uuid.data);
    LOGW("Unexpected: KR-Node-Update from UUID[%s]\n", uuid_str);
    return;
  }
  bln->phase = e->phase;
  LOGV("Node[%x] moved to [%u] phase - Netkey ID [%u]\n",
       bln->n->addr,
       bln->phase,
       e->key);
}

static void kr_nwk_update(const struct gecko_msg_mesh_prov_key_refresh_phase_update_evt_t *e)
{
  LOGM("Network moved to [%u] phase - Netkey ID [%d]\n",
       e->phase,
       e->key);
}

static void kr_finish(const struct gecko_msg_mesh_prov_key_refresh_complete_evt_t *e)
{
  struct gecko_msg_mesh_test_get_key_rsp_t *rsp;
  mng_t *mng = get_mng();

  mng->cache.bl.state = bl_done;

  LOGM("Key Refresh Complete <<%s>>, err[0x%04x]\nNew Network Key Id is 0x%04x\n",
       e->result == 0 ? "YES" : "NO",
       e->result,
       e->key);
  if (e->result) {
    return;
  }

  mng->cfg->subnets[0].netkey.id = e->key;

  provset_netkeyid(&mng->cfg->subnets[0].netkey.id);
  rsp = gecko_cmd_mesh_test_get_key(mesh_test_key_type_net,
                                    mng->cfg->subnets[0].netkey.id,
                                    1);

  if (rsp->result != bg_err_success) {
    LOGBGE("test get netkey", rsp->result);
    return;
  } else {
    memcpy(mng->cfg->subnets[0].netkey.val, rsp->key.data, 16);
    provset_netkeyval(mng->cfg->subnets[0].netkey.val);
    LOGV("New Network Key Recorded.\n");
  }
}

#define __BL_LEN 200
static void bl_result(void)
{
  mng_t *mng = get_mng();
  const char err[] = "ERROR  ", suc[] = "SUCCESS";
  char buf[__BL_LEN] = { 0 };
  int ofs = 0;

  bt_shell_printf("BL result:\n");
  bt_shell_printf("---------------------------------------------------------------\n");
  LOGM("BL result:\n");
  LOGM("---------------------------------------------------------------\n");
  for (int i = 0; i < mng->cache.bl.rem.num; i++) {
    buf[ofs++] = '|';
    ofs += fmt_uuid(buf + ofs, mng->cache.bl.rem.nodes[i].n->uuid);
    snprintf(buf + ofs, __BL_LEN - ofs, " | 0x%04x | 0x%02x | %s |\n",
             mng->cache.bl.rem.nodes[i].n->addr,
             mng->cache.bl.rem.nodes[i].phase,
             mng->cache.bl.rem.nodes[i].phase ? err : suc);
    bt_shell_printf("%s", buf);
    if (!mng->cache.bl.rem.nodes[i].phase) {
      LOGM("%s", buf);
    } else {
      LOGE("%s", buf);
    }
    ofs = 0;
    memset(buf, 0, __BL_LEN);
  }
  LOGM("---------------------------------------------------------------\n");
  bt_shell_printf("---------------------------------------------------------------\n");
}

static void on_bl_done(void)
{
  mng_t *mng = get_mng();
  for (int i = mng->cache.bl.tail; i < mng->cache.bl.offset; i++) {
    node_t *n = (node_t *)g_list_nth_data(mng->lists.bl, i);
    nodes_bl(n->addr);
  }
  if (mng->cache.bl.offset != g_list_length(mng->lists.bl)) {
    mng->cache.bl.state = bl_starting;
    mng->cache.bl.tail = mng->cache.bl.offset;
  } else {
    free(mng->cache.bl.rem.nodes);
    g_list_free(mng->lists.bl);
    mng->lists.bl = NULL;
    memset(&mng->cache.bl, 0, sizeof(bl_cache_t));
  }
}
