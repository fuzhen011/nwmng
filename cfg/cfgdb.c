/*************************************************************************
    > File Name: cfgdb.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>

#include "projconfig.h"
#include "cfgdb.h"
#include "err.h"
#include "shash.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static htb_t *devs = NULL;

/* Static Functions Declaractions ************************************* */
static unsigned int addrhash(const void *key)
{
  return (unsigned int)(*(uint16_t *)(key));
}

err_t cfgdb_init(void)
{
  if (devs) {
    return ec_success;
  }

  devs = new_htb(addrhash, GC_EXP_DEV_NUM);

  return ec_success;
}

void cfgdb_deinit(void)
{
  htb_destory(devs);
}
