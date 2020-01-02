/*************************************************************************
    > File Name: cfg.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <errno.h>

#include "projconfig.h"
#include "cfgdb.h"
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
extern void cfgdb_test(void);

void dump_tmpl(int k)
{
  tmpl_t *t;
  t = cfgdb_tmpl_get(k);
  if (!t) {
    LOGE("KEY[0x%x] is not in the hash table\n", k);
    /* cfgdb_test(); */
    return;
  }
  LOGD("Template [0x%x] Dump:\n", k);

  if (t->ttl) {
    LOGD("\tttl of refid %d is %d\n", k, *t->ttl);
  } else {
    LOGD("\tttl of refid %d is NULL\n", k);
  }

  if (t->pub) {
    LOGD("\tPublication: \n");
    LOGD("\t\tAddress[0x%x]\n", t->pub->addr);
    LOGD("\t\tAppkey[0x%x]\n", t->pub->aki);
    LOGD("\t\tPeriod[0x%x]\n", t->pub->period);
    LOGD("\t\tTTL[0x%x]\n", t->pub->ttl);
    LOGD("\t\tTx Parameter[Count[0x%x]:Interval[0x%x]]\n", t->pub->txp.cnt,
         t->pub->txp.intv);
  } else {
    LOGD("\tPublication: NULL\n");
  }
  LOGD("\tSecure Network Beacon[%s]\n",
       t->snb ? (*t->snb ? "On" : "Off") : "NULL");

  if (t->net_txp) {
    LOGD("\tNetwork Tx Parameter[Count[0x%x]:Interval[0x%x]]\n", t->net_txp->cnt,
         t->net_txp->intv);
  } else {
    LOGD("\tNetwork Tx Parameter: NULL\n");
  }

  if (t->bindings) {
    LOGD("\tBinding number[%d]\n", t->bindings->len);
  } else {
    LOGD("\tBinding: NULL\n");
  }

  if (t->sublist) {
    LOGD("\tSubsription number[%d]\n", t->sublist->len);
  } else {
    LOGD("\tSubsription: NULL\n");
  }

  LOGN();
}

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
