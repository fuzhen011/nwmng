/*************************************************************************
    > File Name: startup.c
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

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

#if 0
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
#endif

#if !defined(CLI_MNG) && !defined(CFG)
typedef int (*proc)(void);
static proc procs[] = {
  cli_proc, cfg_proc
};
static const int proc_num = sizeof(procs) / sizeof(proc);
static pid_t pids[10] = { 0 };
#endif

void startup(int argc, char *argv[])
{
#if defined(CLI_MNG)
  err_t e;
  if (ec_success != (e = logging_init(LOG_FILE_PATH,
                                      0, /* Not output to stdout */
                                      LOG_MINIMAL_LVL(LVL_VER)))) {
    fprintf(stderr, "LOG INIT ERROR (%x)\n", e);
    return;
  }
  cli_proc_init(0, NULL);
  cli_proc();
  out:
  logging_deinit();
#elif defined(CFG)
  err_t e;
  if (ec_success != (e = logging_init(LOG_FILE_PATH,
                                      0, /* Not output to stdout */
                                      LOG_MINIMAL_LVL(LVL_VER)))) {
    fprintf(stderr, "LOG INIT ERROR (%x)\n", e);
    return;
  }
  EC(ec_success, cfg_init());
  cfg_proc();
  out:
  logging_deinit();
  cfgdb_deinit();
  return 0;
#else
  /* Single exec - fork the cfg process */
  err_t e;
  pid_t pid;
  int i = 0;
  for (i = 0; i < proc_num - 1; i++) {
    pid = fork();

    if (pid == 0 || pid == -1) {
      /* If it's child, break, so only the parent process will fork */
      break;
    }
    pids[proc_num - 1 - i] = pid;
  }

  if (ec_success != (e = logging_init(LOG_FILE_PATH,
                                      0, /* Not output to stdout */
                                      LOG_MINIMAL_LVL(LVL_VER)))) {
    fprintf(stderr, "LOG INIT ERROR (%x)\n", e);
    return;
  }

  if (i == proc_num - 1) {
    e = cli_proc_init(proc_num - 1, pids);
    elog(e);
  }

  procs[proc_num - 1 - i]();

  if (i == proc_num - 1) {
    wait(NULL);
  }

  switch (pid) {
    case -1:
      break;
    case 0:
      break;
    default:
      break;
  }
#endif
}
