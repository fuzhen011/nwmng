/*************************************************************************
    > File Name: main.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include "logging.h"
/* TODO: above includes are used for test */
#include <unistd.h>

#include <sys/wait.h>

#include "cli/cli.h"
#include "mng/mng.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
typedef int (*proc)(void);

static proc procs[] = {
  cli_proc, mng_proc
};

static const int proc_num = sizeof(procs) / sizeof(proc);
pid_t pids[10] = { 0 };
/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

int main(int argc, char *argv[])
{
  pid_t pid;
  int i = 0;
#if 1
  /* char t[] = "ask"; */
  /* char s[] = "asksdfasdfasdfasdfasdf"; */
  /* printf("%8.8s\n", t); */
  /* printf("%8.8s\n", s); */
  logging_init(NULL);
  LOG();



























































  LOG();
  return 0;
#endif

  for (i = 0; i < proc_num - 1; i++) {
    pid = fork();

    if (pid == 0 || pid == -1) {
      /* If it's child, break, so only the parent process will fork */
      break;
    }
    pids[proc_num - 1 - i] = pid;
  }

  if (i == proc_num - 1) {
    cli_proc_init(proc_num - 1, pids);
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

  cli_proc();

  return 0;
}
