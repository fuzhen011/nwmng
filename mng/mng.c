/*************************************************************************
    > File Name: mng/mng.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* #include <sys/prctl.h> */
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
void __mng_exit(void)
{
  printf("MNG Exit\n");
  fflush(stdout);
}

int mng_proc(void)
{
  /* prctl(PR_SET_PDEATHSIG, SIGKILL); */
  atexit(__mng_exit);
  for (;;) {
    sleep(1);
  }
}
