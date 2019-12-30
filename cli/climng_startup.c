/*************************************************************************
    > File Name: climng_startup.c
    > Author: Kevin
    > Created Time: 2019-12-30
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "climng_startup.h"
#include "logging.h"
#include "nwk.h"
#include "cli.h"
#include "mng_ipc.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
pthread_t mng_tid;

pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER,
                hdrlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t qready = PTHREAD_COND_INITIALIZER,
               hdrready = PTHREAD_COND_INITIALIZER;

jmp_buf initjmpbuf;

/* Static Variables *************************************************** */
static proj_args_t projargs = { 0 };

static init_func_t initfs[] = {
  cli_init,
  clr_all,
  init_ncp,
  conn_socksrv,
  ipc_get_provcfg,
};

static const int inits_num = ARR_LEN(initfs);

/* Static Functions Declaractions ************************************* */
int offsetof_initfunc(init_func_t fn)
{
  int i;
  for (i = 0; i < inits_num; i++) {
    if (initfs[i] == fn) {
      break;
    }
  }
  return i;
}

void on_sock_disconn(void)
{
  longjmp(initjmpbuf, offsetof_initfunc(init_ncp));
}

const proj_args_t *getprojargs(void)
{
  return &projargs;
}

static int setprojargs(int argc, char *argv[])
{
#if 0
  /* TODO: Impl pending */
#else
  projargs.enc = false;
  memcpy(projargs.port, PORT, sizeof(PORT));
  projargs.initialized = true;
  return 0;
#endif
}

int cli_proc(int argc, char *argv[])
{
  int ret;
  err_t e;
  LOGD("CLI-MNG Process Started Up\n");
  if (0 != setprojargs(argc, argv)) {
    LOGE("Set project args error.\n");
    exit(EXIT_FAILURE);
  }

  ret = setjmp(initjmpbuf);
  LOGM("Init cli-mng from %d\n", ret);
  if (ret == 0) {
    ret = FULL_RESET;
  }
  for (int i = 0; i < sizeof(int) * 8; i++) {
    if (!IS_BIT_SET(ret, i)) {
      continue;
    }
    if (ec_success != (e = initfs[i](NULL))) {
      elog(e);
      exit(EXIT_FAILURE);
    }
  }

  e = nwk_init(NULL);
  elog(e);

  if (0 != (ret = pthread_create(&mng_tid,
                                 NULL,
                                 mng_mainloop,
                                 NULL))) {
    err_exit_en(ret, "pthread create");
  }

  cli_mainloop(NULL);

  if (0 != (ret = pthread_join(mng_tid, NULL))) {
    err_exit_en(ret, "pthread_join");
  }

  return 0;
}
