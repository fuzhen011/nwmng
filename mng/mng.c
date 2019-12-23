/*************************************************************************
    > File Name: mng/mng.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>

#include "hal/bg_uart_cbs.h"
#include "host_gecko.h"

#include "projconfig.h"
/* #include <sys/prctl.h> */
#include "logging.h"
#include "utils.h"
#include "gecko_bglib.h"
#include "bgevt_hdr.h"
#include "mng.h"
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static mng_t mng;

/* Static Functions Declaractions ************************************* */
mng_t *get_mng(void)
{
  return &mng;
}

void __mng_exit(void)
{
  /* gecko_cmd_system_reset(0); */
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

err_t mng_init(bool enc)
{
  bguart_init(false, PORT, NULL);
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
