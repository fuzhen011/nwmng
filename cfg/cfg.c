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

#include "parser/generic_parser.h"
#include "parser/json_parser.h"
/* TEST */
#include <unistd.h>
#include <stdlib.h>
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
extern void cfgdb_test(void);
static void test_print_ttl(int k)
{
  tmpl_t *t;
  t = cfgdb_tmpl_get(k);
  if (!t) {
    LOGE("key %d is not in the hash table\n", k);
    /* cfgdb_test(); */
    return;
  }
  if (t->ttl) {
    LOGD("ttl of refid %d is %d\n", k, *t->ttl);
  } else {
    LOGD("ttl of refid %d is NULL\n", k);
  }
}

static void cfg_test(void)
{
  err_t e;

  /* LOGD("%d devices in DB.\n", cfgdb_devnum(0)); */
  /* LOGD("%d nodes in DB.\n", cfgdb_devnum(1)); */

  e = json_cfg_open(TEMPLATE_FILE,
                    "/home/zhfu/work/projs/nwmng/tools/mesh_config/templates.json",
                    0,
                    NULL);
  elog(e);
  json_close(TEMPLATE_FILE);
  test_print_ttl(1);
  test_print_ttl(0x21);
  sleep(5);
  e = json_cfg_open(TEMPLATE_FILE,
                    "/home/zhfu/work/projs/nwmng/tools/mesh_config/templates.json",
                    0,
                    NULL);
  elog(e);
  json_close(TEMPLATE_FILE);
  test_print_ttl(1);
  test_print_ttl(0x21);

  e = json_cfg_open(NW_NODES_CFG_FILE,
                    "/home/zhfu/work/projs/nwmng/tools/mesh_config/test1/nwk.json",
                    0,
                    NULL);
  elog(e);
  e = json_cfg_open(PROV_CFG_FILE,
                    "/home/zhfu/work/projs/nwmng/tools/mesh_config/test1/prov.json",
                    0,
                    NULL);
  elog(e);

  json_close(NW_NODES_CFG_FILE);
  json_close(PROV_CFG_FILE);
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
