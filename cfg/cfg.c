/*************************************************************************
    > File Name: cfg.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <errno.h>

#include "projconfig.h"
#include "cfg.h"
#include "cfgdb.h"
#include "logging.h"
#include "ccipc.h"

#include "parser/generic_parser.h"
#include "parser/json_parser.h"

/* TEST */
#include <unistd.h>
#include <stdlib.h>
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static int listenfd, clntfd;

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

static void cfg_test(void)
{
  err_t e;
  /* LOGD("%d devices in DB.\n", cfgdb_devnum(0)); */
  /* LOGD("%d nodes in DB.\n", cfgdb_devnum(1)); */

  e = json_cfg_open(TEMPLATE_FILE,
                    TMPLATE_FILE_PATH,
                    0);
  elog(e);
  json_cfg_close(TEMPLATE_FILE);
  /* dump_tmpl(1); */
  /* dump_tmpl(0x21); */
#if 0
  sleep(5);
  e = json_cfg_open(TEMPLATE_FILE,
                    TMPLATE_FILE_PATH,
                    0,
                    NULL);
  elog(e);
  json_cfg_close(TEMPLATE_FILE);
  test_print_ttl(1);
  test_print_ttl(0x21);
#endif

  e = json_cfg_open(NW_NODES_CFG_FILE,
                    NWNODES_FILE_PATH,
                    0);
  elog(e);
  e = json_cfg_open(PROV_CFG_FILE,
                    SELFCFG_FILE_PATH,
                    0);
  elog(e);

  node_t *n = cfgdb_node_get(0x0001);
  n->err++;
  json_cfg_write(NW_NODES_CFG_FILE, wrt_errbits, n->uuid, &n->err);
#if 0
  node_t *n = cfgdb_node_get(0x0001);
  if (n) {
    n->addr = 0x2;
    json_cfg_write(NW_NODES_CFG_FILE, wrt_node_addr, n->uuid, &n->addr);
  }
#endif

#if 0
  e = json_cfg_write(NW_NODES_CFG_FILE,
                     wrt_add_node,
                     NULL,
                     "\x01\x02\x03\x04\x05\x06\x07\x07\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10");
  elog(e);
  e = json_cfg_write(NW_NODES_CFG_FILE,
                     wrt_add_node,
                     NULL,
                     "\x11\x02\x03\x04\x05\x06\x07\x07\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10");
  elog(e);
  e = json_cfg_write(NW_NODES_CFG_FILE,
                     wrt_add_node,
                     NULL,
                     "\x21\x02\x03\x04\x05\x06\x07\x07\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10");
  elog(e);
#endif
  json_cfg_close(NW_NODES_CFG_FILE);
  json_cfg_close(PROV_CFG_FILE);
}

err_t cfg_init(void)
{
  err_t e;
  uid_t uid;
  EC(ec_success, cfgdb_init());

  if (0 > (listenfd = serv_listen(CC_SOCK_SERV_PATH))) {
    LOGE("Serv[%s] Listen error [%s]\n", CC_SOCK_CLNT_PATH, strerror(errno));
    return err(ec_sock);
  }

  e = json_cfg_open(TEMPLATE_FILE,
                    TMPLATE_FILE_PATH,
                    0);
  elog(e);
  e = json_cfg_open(NW_NODES_CFG_FILE,
                    NWNODES_FILE_PATH,
                    0);
  elog(e);
  e = json_cfg_open(PROV_CFG_FILE,
                    SELFCFG_FILE_PATH,
                    0);
  elog(e);
  LOGD("CFG wait for socket connection\n");
  if (0 > (clntfd = serv_accept(listenfd, &uid))) {
    LOGE("Serv Accept error [ret:%d][%s]\n", clntfd, strerror(errno));
    return err(ec_sock);
  }
  LOGM("Socket connected\n");

  return e;
}

int cfg_proc(void)
{
  err_t e;
  LOGM("CFG process started up.\n");
  e = cfg_init();
  if (ec_success != e) {
    elog(e);
    exit(EXIT_FAILURE);
  }

  while (1) {
    sleep(1);
  }
}

void *cfg_mainloop(void *p)
{
  cfg_init();
  cfg_proc();
  return NULL;
}
