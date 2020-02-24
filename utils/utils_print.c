/*************************************************************************
    > File Name: utils_print.c
    > Author: Kevin
    > Created Time: 2020-01-10
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "utils.h"
#include "logging.h"
#include "cfg.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
void uprint_tmpl(uint16_t refid)
{
  tmpl_t *t;
  t = cfgdb_tmpl_get(refid);
  if (!t) {
    LOGE("KEY[0x%x] is not in the hash table\n", refid);
    /* cfgdb_test(); */
    return;
  }
  LOGD("Template [0x%x] Dump:\n", refid);

  if (t->ttl) {
    LOGD("\tttl of refid %d is %d\n", refid, *t->ttl);
  } else {
    LOGD("\tttl of refid %d is NULL\n", refid);
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
