/*************************************************************************
    > File Name: generic_parser.c
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "json_parser.h"
#include "generic_parser.h"
#include "cfgdb.h"
#include "cfg.h"
#include "ccipc.h"

/* Defines  *********************************************************** */
typedef struct {
  bool initialized;
  gp_init_func_t init;
  gp_deinit_func_t deinit;
  gp_open_func_t open;
  gp_read_func_t read;
  gp_write_func_t write;
  gp_close_func_t close;
  gp_flush_func_t flush;
}gp_t;

/* Global Variables *************************************************** */
#define CHECK_STATE(ret) \
  do { if (!gp.initialized) { return err((ret)); } } while (0)
#define CHECK_NULL_RET() \
  do { if (!gp.initialized) { return NULL; } } while (0)
#define CHECK_VOID_RET() \
  do { if (!gp.initialized) { return; } } while (0)

/* Static Variables *************************************************** */
gp_t gp = { 0 };

/* Static Functions Declaractions ************************************* */

void gp_init(int cfg_filetype, void *init_data)
{
  if (cfg_filetype != cft_json) {
    return;
  }
  if (gp.initialized) {
    return;
  }
  /* gp.init = NULL; */
  /* gp.deinit = NULL; */
  gp.open = json_cfg_open;
  gp.write = json_cfg_write;
  gp.read = json_cfg_read;
  gp.close = json_cfg_close;
  gp.flush = json_cfg_flush;

  if (gp.init) {
    gp.init(init_data);
  }
  gp.initialized = true;
}

void gp_deinit(void)
{
  memset(&gp, 0, sizeof(gp_t));
}

err_t prov_clrctl(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_clrctl, NULL, NULL);
}

err_t provset_addr(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_addr, NULL, (void *)arg);
}

err_t provset_ivi(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_ivi, NULL, (void *)arg);
}

err_t provset_synctime(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_synctime, NULL, (void *)arg);
}

err_t provset_netkeyid(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_netkey_id, NULL, (void *)arg);
}

err_t provset_netkeydone(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_netkey_done, NULL, (void *)arg);
}

err_t provset_appkeyid(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_appkey_id, (void *)arg, (void *)arg + 2);
}

err_t provset_appkeydone(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_appkey_done, arg, (void *)arg + 2);
}

err_t prov_get(int len, const char *arg)
{
  err_t e;
  provcfg_t *pc = get_provcfg();
  uint8_t buf[0xff] = { 0 };
  uint8_t i = 0;

  /* Send the provcfg basic data */
  memcpy(buf + i, &pc->addr, sizeof(uint16_t));
  i += sizeof(uint16_t);
  memcpy(buf + i, &pc->sync_time, sizeof(time_t));
  i += sizeof(time_t);
  memcpy(buf + i, &pc->ivi, sizeof(uint32_t));
  i += sizeof(uint32_t);
  memcpy(buf + i, &pc->subnet_num, sizeof(uint8_t));
  i += sizeof(uint8_t);
  EC(ec_success, sendto_client(RSP_PROV_BASIC, i, buf));
  memset(buf, 0, i);
  i = 0;

  if (pc->subnet_num) {
    memcpy(buf + i, pc->subnets, sizeof(subnet_t));
    i += sizeof(subnet_t);
    if (pc->subnets[0].appkey_num) {
      memcpy(buf + i, pc->subnets[0].appkey,
             sizeof(meshkey_t) * pc->subnets[0].appkey_num);
      i += sizeof(meshkey_t) * pc->subnets[0].appkey_num;
    }
    EC(ec_success, sendto_client(RSP_PROV_SUBNETS, i, buf));
    memset(buf, 0, i);
    i = 0;
  }

  if (pc->ttl) {
    buf[0] = *pc->ttl;
    i += 1;
    EC(ec_success, sendto_client(RSP_PROV_TTL, i, buf));
    memset(buf, 0, i);
    i = 0;
  }
  if (pc->net_txp) {
    memcpy(buf + i, pc->net_txp, sizeof(txparam_t));
    i += sizeof(txparam_t);
    EC(ec_success, sendto_client(RSP_PROV_TXP, i, buf));
    memset(buf, 0, i);
    i = 0;
  }
  if (pc->timeout) {
    memcpy(buf + i, pc->timeout, sizeof(timeout_t));
    i += sizeof(timeout_t);
    EC(ec_success, sendto_client(RSP_PROV_TIMEOUT, i, buf));
    memset(buf, 0, i);
    i = 0;
  }
  return ec_success;
}

err_t _upldev_check(int len, const char *arg)
{
  uint8_t ret = 1;
  err_t e;
  node_t *n = cfgdb_unprov_dev_get((const uint8_t *)arg);

  if (!n || n->rmorbl) {
    ret = 0;
  }
  EC(ec_success, sendto_client(RSP_UPL_CHECK, 1, &ret));
  return ec_success;
}

