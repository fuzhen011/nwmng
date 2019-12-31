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

/* Static Functions Declaractions ************************************* */
#if !defined(CLI_MNG) && !defined(CFG)
typedef int (*proc_t)(int argc, char *argv[]);
static proc_t procs[] = {
  cli_proc, cfg_proc
};
static const int proc_num = sizeof(procs) / sizeof(proc_t);
static pid_t pids[10] = { 0 };
#endif

void startup(int argc, char *argv[])
{
#if defined(CLI_MNG)
  err_t e;
  if (ec_success != (e = cli_proc_init(0, NULL))) {
    elog(e);
    return;
  }
  cli_proc(argc, argv); /* should never return */

  logging_deinit();
#elif defined(CFG)
  /* cfg_init is called inside */
  cfg_proc(argc, argv); /* should never return */

  logging_deinit();
  cfgdb_deinit();
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

  if (i == proc_num - 1) {
    e = cli_proc_init(proc_num - 1, pids);
    elog(e);
  }

  procs[proc_num - 1 - i](argc, argv);

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
