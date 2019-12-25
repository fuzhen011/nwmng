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

/* Global Variables *************************************************** */
extern sock_status_t sock;

/* Static Variables *************************************************** */
static mng_t mng = { 0 };

/* Static Functions Declaractions ************************************* */
err_t handle_rsp(ipcevt_hdr_t hdr)
{
  err_t e = ec_success;
  bool err = false;
  char r[2] = { 0 };
  char *buf = NULL;
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
    if (r[0] == RSP_OK) {
      return e;
    } else if (r[0] == RSP_ERR) {
      e = *(err_t *)buf;
      free(buf);
      return e;
    }
    if (hdr) {
      hdr(r[0], r[1], buf);
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

static int __provcfg_field(opc_t opc, uint8_t len, const char *buf)
{
  switch (opc) {
    default:
      LOGM("recv opc[%d] with len[%d] payload\n", opc, len);
      break;
  }
  return (0);
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
