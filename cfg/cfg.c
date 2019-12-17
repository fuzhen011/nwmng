/*************************************************************************
    > File Name: cfg.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "cfg.h"
#include "cfgdb.h"
#include "logging.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static void cfg_test(void)
{
  LOGD("%d devices in DB.\n", cfgdb_devnum(0));
  LOGD("%d nodes in DB.\n", cfgdb_devnum(1));
}

err_t cfg_init(void)
{
  err_t e;
  e = cfgdb_init();
  return e;
}

void cfg_proc(void)
{
  cfg_test();
}
