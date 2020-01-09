/*************************************************************************
    > File Name: startup.c
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>

#include <setjmp.h>
#include <pthread.h>

#include "startup.h"
#include "logging.h"

#include "cli.h"
#include "mng.h"
#include "nwk.h"
#include "cfg.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
bool mng_started = false;
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
  cfg_init,
  mng_init,
};
static const int inits_num = ARR_LEN(initfs);

/* Static Functions Declaractions ************************************* */
void startup(int argc, char *argv[])
{
  err_t e;
  if (ec_success != (e = cli_proc_init(0, NULL))) {
    elog(e);
    return;
  }
  cli_proc(argc, argv); /* should never return */

  logging_deinit();
}

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
  int ret, tmp;
  err_t e;

#if (__APPLE__ == 1)
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
#endif
  if (0 != setprojargs(argc, argv)) {
    LOGE("Set project args error.\n");
    exit(EXIT_FAILURE);
  }

  ret = setjmp(initjmpbuf);
  LOGM("Program Started Up, Initialization Bitmap - 0x%x\n", ret);
  if (mng_started && 0 != (tmp = pthread_cancel(mng_tid))) {
    LOGE("Cancel pthread error[%d:%s]\n", tmp, strerror(tmp));
  }
  mng_started = false;
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

  if (0 != (ret = pthread_create(&mng_tid, NULL, mng_mainloop, NULL))) {
    err_exit_en(ret, "pthread create");
  }
  mng_started = true;

  cli_mainloop(NULL);

  if (0 != (ret = pthread_join(mng_tid, NULL))) {
    err_exit_en(ret, "pthread_join");
  }

  return 0;
}
