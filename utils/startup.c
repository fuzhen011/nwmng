/*************************************************************************
    > File Name: startup.c
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

#include "bg_uart_cbs.h"
#include "bg_uart_cbs.h"
#include "projconfig.h"
#include "startup.h"
#include "mng.h"
#include "logging.h"
#include "cli.h"
#include "cfg.h"
#include "utils.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
pthread_t cfg_tid;

/* Static Functions Declaractions ************************************* */

void startup(void *args)
{
  err_t e;
  int ret;
  printf("\nStart mng \n");
  LOGD("Sync NCP target \n");
  if (ec_success != (e = mng_init(false))) {
    exit(EXIT_FAILURE);
  }

  printf("\nStart cli \n");

  if (0 != (ret = pthread_create(&cfg_tid,
                                 NULL,
                                 cfg_mainloop,
                                 NULL))) {
    errExitEN(ret, "pthread_create Console");
  }

  cli_mainloop(NULL);

  if (0 != (ret = pthread_join(cfg_tid, NULL))) {
    errExitEN(ret, "pthread_join");
  }

  ASSERT(0);
}
