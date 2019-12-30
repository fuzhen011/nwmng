/*************************************************************************
    > File Name: mng/mng.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/* #include <sys/prctl.h> */

#include <sys/socket.h>

#include "hal/bg_uart_cbs.h"
#include "host_gecko.h"

#include "projconfig.h"
#include "logging.h"
#include "utils.h"
#include "gecko_bglib.h"
#include "bgevt_hdr.h"
#include "mng.h"
#include "mng_ipc.h"
#include "nwk.h"
#include "ccipc.h"
#include "cli.h"
#include "climng_startup.h"
/* Defines  *********************************************************** */
#define INVALID_CONN_HANDLE 0xff

/* Global Variables *************************************************** */
extern sock_status_t sock;

/* Static Variables *************************************************** */
static mng_t mng = {
  .conn = 0xff
};

/* Static Functions Declaractions ************************************* */

static int __provcfg_field(opc_t opc, uint8_t len, const uint8_t *buf,
                           void *out)
{
  int i = 0;
  switch (opc) {
    case RSP_PROV_BASIC:
      memcpy(&mng.cfg.addr, buf + i, sizeof(uint16_t));
      i += sizeof(uint16_t);
      memcpy(&mng.cfg.sync_time, buf + i, sizeof(time_t));
      i += sizeof(time_t);
      memcpy(&mng.cfg.ivi, buf + i, sizeof(uint32_t));
      i += sizeof(uint32_t);
      memcpy(&mng.cfg.subnet_num, buf + i, sizeof(uint8_t));
      /* i += sizeof(uint8_t); */
      break;
    case RSP_PROV_SUBNETS:
      if (mng.cfg.subnets) {
        free(mng.cfg.subnets);
      }
      /* buf[0] is the appkey_num */
      mng.cfg.subnets = malloc(sizeof(subnet_t) + sizeof(meshkey_t) * buf[0]);
      memcpy(mng.cfg.subnets, buf + i, sizeof(subnet_t));
      i += sizeof(subnet_t);
      if (buf[0]) {
        memcpy(mng.cfg.subnets[0].appkey, buf + i,
               sizeof(meshkey_t) * buf[0]);
        i += sizeof(meshkey_t) * buf[0];
      }
      break;
    case RSP_PROV_TTL:
      if (!mng.cfg.ttl) {
        mng.cfg.ttl = malloc(sizeof(uint8_t));
      }
      *mng.cfg.ttl = buf[0];
      break;
    case RSP_PROV_TXP:
      if (!mng.cfg.net_txp) {
        mng.cfg.net_txp = malloc(sizeof(txparam_t));
      }
      memcpy(mng.cfg.net_txp, buf, sizeof(txparam_t));
      break;
    case RSP_PROV_TIMEOUT:
      if (!mng.cfg.timeout) {
        mng.cfg.timeout = malloc(sizeof(timeout_t));
      }
      memcpy(mng.cfg.timeout, buf, sizeof(timeout_t));
      break;
    default:
      return 0;
  }
  return 1;
}

err_t ipc_get_provcfg(void *p)
{
  err_t e;
  EC(ec_success, socktocfg(CPG_ALL, 0, NULL, NULL, __provcfg_field));
  return e;
}

mng_t *get_mng(void)
{
  return &mng;
}

void __ncp_exit(void)
{
  gecko_cmd_system_reset(0);
  LOGD("MNG Exit\n");
}

err_t clr_all(void *p)
{
  int ret;
  err_t e;
  EC(ec_success, socktocfg(CPS_CLRCTL, 0, NULL, NULL, NULL));

  /* TODO: IPC to clear all nodes */

  if (mng.conn != INVALID_CONN_HANDLE) {
    if (bg_err_success != (ret = gecko_cmd_le_connection_close(mng.conn)->result)) {
      LOGBGE("Disconnect", ret);
      return err(ec_bgrsp);
    }
    mng.conn = INVALID_CONN_HANDLE;
  }

  if (bg_err_success != (ret = gecko_cmd_flash_ps_erase_all()->result)) {
    LOGBGE("Erase all", ret);
    return err(ec_bgrsp);
  }
  usleep(300 * 1000);
  return ec_success;
}

err_t init_ncp(void *p)
{
  const proj_args_t *pg = getprojargs();
  if (!pg->initialized) {
    return err(ec_state);
  }

  bguart_init(pg->enc,
              pg->enc ? (char *)pg->sock.srv : (char *)pg->port,
              pg->enc ? (char *)pg->sock.clt : NULL);

  conn_ncptarget();
  sync_host_and_ncp_target();
  atexit(__ncp_exit);
  LOGD("ncp init done\n");
  return ec_success;
}

void *mng_mainloop(void *p)
{
#if 1
  static volatile int i = 0;
  if (mng.state == state_reload) {
    /* TODO: reload the state */
  }
  if (mng.state == adding_devices_em) {
    i++;
    if (i == 0x004fffff) {
      mng.state = configured;
      i = 0;
      LOGM("back\n");
      return NULL;
    }
    return (void *)1;
  }
#endif
  return NULL;
}

err_t clm_set_scan(int onoff)
{
  uint16_t ret;
  if (!!mng.status.free_mode == onoff) {
    return ec_success;
  }
  if (onoff) {
    ret = gecko_cmd_mesh_prov_scan_unprov_beacons()->result;
    CHECK_BGCALL(ret, "scan unprov beacon");
  } else {
    ret = gecko_cmd_mesh_prov_stop_scan_unprov_beacons()->result;
    CHECK_BGCALL(ret, "stop unprov beacon scanning");
  }
  mng.status.free_mode = onoff ? fm_freemode : fm_idle;
  LOGD("Scanning unprovisioned beacon [%s]\n", onoff ? "ON" : "OFF");
  return ec_success;
}

/* int mng_evt_hdr(const void *ve) */
/* { */
/* } */
