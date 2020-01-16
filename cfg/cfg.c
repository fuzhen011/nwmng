/*************************************************************************
    > File Name: cfg.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <errno.h>

#include "projconfig.h"
#include "logging.h"
#include "cfg.h"

#include "generic_parser.h"

/* TEST */
#include <unistd.h>
#include <stdlib.h>
/* Defines  *********************************************************** */
/* Global Variables *************************************************** */
/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
err_t cfg_init(void *p)
{
  err_t e;
  EC(ec_success, cfgdb_init());
  gp_init(cft_json, NULL);
  e = load_cfg_file(TEMPLATE_FILE, 1);
  elog(e);
  e = load_cfg_file(NW_NODES_CFG_FILE, 1);
  elog(e);
  e = load_cfg_file(PROV_CFG_FILE, 1);
  elog(e);
  return e;
}
