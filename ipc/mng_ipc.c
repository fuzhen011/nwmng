/*************************************************************************
    > File Name: mng_ipc.c
    > Author: Kevin
    > Created Time: 2019-12-30
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>

#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>

#include "projconfig.h"
#include "mng_ipc.h"
#include "ccipc.h"
#include "logging.h"
#include "ccipc.h"
#include "utils.h"
#include "climng_startup.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
sock_status_t sock = { -1, -1 };

/* Static Functions Declaractions ************************************* */
static void sock_cln_ipc_test(void)
{
  const char s[] = "hello, cfg";
  char r[50] = { 0 };
  int n;
  if (-1 == (n = send(sock.fd, s, sizeof(s), 0))) {
    LOGE("send [%s]\n", strerror(errno));
  } else {
    LOGM("Send [%d:%s]\n", n, s);
  }
  if (-1 == (n = recv(sock.fd, r, 50, 0))) {
    LOGE("recv [%s]\n", strerror(errno));
  } else {
    LOGM("CLI received [%d:%s] from client.\n", n, r);
  }
  LOGM("CLI Socket TEST DONE\n");
}

err_t conn_socksrv(void *p)
{
  if (sock.fd >= 0) {
    close(sock.fd);
    sock.fd = -1;
  }

  usleep(20 * 1000);

  for (int numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
    if (0 <= (sock.fd = cli_conn(CC_SOCK_CLNT_PATH, CC_SOCK_SERV_PATH))) {
      break;
    }
    LOGW("Connect server failed ret[%d] reason[%s]\n", sock.fd, strerror(errno));
    /*
     * Delay before trying again.
     */
    if (numsec <= MAXSLEEP / 2) {
      sleep(numsec);
    }
  }
  if (sock.fd < 0) {
    LOGE("Connect server timeout. Exit.\n");
    exit(EXIT_FAILURE);
  }
  LOGM("Socket connected\n");
  sock_cln_ipc_test();
  LOGD("sock conn done\n");
  return ec_success;
}

static err_t handle_rsp(ipcevt_hdr_t hdr, void *out);

err_t socktocfg(opc_t opc, uint8_t len, const void *buf, void *out,
                ipcevt_hdr_t hdr)
{
  err_t e;
  if (sock.fd < 0) {
    return err(ec_state);
  }
  EC(ec_success, sock_send(&sock, opc, len, buf));
  EC(ec_success, handle_rsp(hdr, out));
  return ec_success;
}

err_t socktocfg_va(opc_t opc, void *out, ipcevt_hdr_t hdr, ...)
{
  err_t e = ec_success;
  int s, len = 0;
  uint8_t buf[255] = { 0 };
  uint8_t *p;
  va_list param_list;

  if (sock.fd < 0) {
    return err(ec_state);
  }
  va_start(param_list, hdr);

  s = va_arg(param_list, int);
  while (s != 0) {
    if (255 - len < s) {
      e = err(ec_length_leak);
      goto end;
    }
    p = va_arg(param_list, uint8_t *);
    memcpy(buf + len, p, s);
    len += s;
    s = va_arg(param_list, int);
  }
  ECG(ec_success, sock_send(&sock, opc, len, buf), end);
  ECG(ec_success, handle_rsp(hdr, out), end);

  end:
  va_end(param_list);
  return e;
}

static err_t handle_rsp(ipcevt_hdr_t hdr, void *out)
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
      goto out;
    } else if (r[0] == RSP_ERR) {
      e = *(err_t *)buf;
      goto out;
    }
    if (hdr) {
      if (!hdr(r[0], r[1], buf, out)) {
        LOGE("Recv unexpected CMD[%u:%u]\n", r[0], r[1]);
      }
    }
    if (buf) {
      free(buf);
      buf = NULL;
    }
  }

  out:
  if (buf) {
    free(buf);
  }
  return e;

  sock_err:
  if (buf) {
    free(buf);
  }
  if (err) {
    on_sock_disconn();
  }
  return e;
}
