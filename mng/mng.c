/*************************************************************************
    > File Name: mng/mng.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
/* #include <sys/prctl.h> */

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
#if NOTODO
static int __provcfg_field(opc_t opc, uint8_t len, const char *buf)
{
  switch (opc) {
  }
  return (opc == RSP_OK || opc == RSP_ERR);
}

static err_t ipc_get_provcfg(void)
{
  err_t e;
  if (sock.fd < 0) {
    return err(ec_state);
  }
  EC(ec_success, sock_send(&sock, CPG_ALL, 0, NULL));

  while (1) {
  }
  return e;
}
#endif
mng_t *get_mng(void)
{
  return &mng;
}

void __mng_exit(void)
{
  gecko_cmd_system_reset(0);
  LOGD("MNG Exit\n");
}

int mng_proc(void)
{
  /* prctl(PR_SET_PDEATHSIG, SIGKILL); */
  atexit(__mng_exit);
  for (;;) {
    sleep(1);
  }
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
  atexit(__mng_exit);
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
