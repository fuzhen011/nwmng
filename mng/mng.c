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
#include "nwk.h"
#include "ccipc.h"
#include "cli.h"
/* Defines  *********************************************************** */
#define INVALID_CONN_HANDLE 0xff

/* Global Variables *************************************************** */
extern sock_status_t sock;

/* Static Variables *************************************************** */
static mng_t mng = {
  .conn = 0xff
};

/* Static Functions Declaractions ************************************* */
err_t handle_rsp(ipcevt_hdr_t hdr)
{
  err_t e = ec_success;
  bool err = false;
  uint8_t r[2] = { 0 };
  uint8_t *buf = NULL;
  int n, len;

  while (1) {
    n = recv(sock.fd, r, 2, 0);
    if (-1 == n) {
      LOGE("recv err[%s]\n", strerror(errno));
      err = true;
      goto sock_err;
    } else if (0 == n) {
      err = true;
      goto sock_err;
    }
    if (r[1]) {
      buf = malloc(r[1]);
      len = r[1];
      while (len) {
        n = recv(sock.fd, buf, len, 0);
        if (-1 == n) {
          LOGE("recv err[%s]\n", strerror(errno));
          err = true;
          goto sock_err;
        } else if (0 == n) {
          err = true;
          goto sock_err;
        }
        len -= n;
      }
    }
    /* LOGM("r[0] = %d\n", r[0]); */
    if (r[0] == RSP_OK) {
      return e;
    } else if (r[0] == RSP_ERR) {
      e = *(err_t *)buf;
      free(buf);
      return e;
    }
    if (hdr) {
      if (!hdr(r[0], r[1], buf)) {
        LOGE("Recv unexpected CMD[%u:%u]\n", r[0], r[1]);
      }
    }
    if (buf) {
      free(buf);
      buf = NULL;
    }
  }

  sock_err:
  if (buf) {
    free(buf);
  }
  if (err) {
    on_sock_disconn();
  }
  return e;
}

static int __provcfg_field(opc_t opc, uint8_t len, const uint8_t *buf)
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
  if (sock.fd < 0) {
    return err(ec_state);
  }
  EC(ec_success, sock_send(&sock, CPG_ALL, 0, NULL));
  EC(ec_success, handle_rsp(__provcfg_field));
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
  /* TODO: IPC to clear the provcfg */
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
  while (1) {
    sleep(1);
  }
}

/* int mng_evt_hdr(const void *ve) */
/* { */
/* } */
