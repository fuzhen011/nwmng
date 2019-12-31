/*************************************************************************
    > File Name: cfg.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <errno.h>

#include <sys/socket.h>

#include "projconfig.h"
#include "cfgdb.h"
#include "logging.h"
#include "cfg.h"

#include "parser/generic_parser.h"
#include "parser/json_parser.h"

/* TEST */
#include <unistd.h>
#include <stdlib.h>
/* Defines  *********************************************************** */
sock_status_t srv_sock = { -1, -1 };
typedef struct {
  opc_t opc;
  err_t (*hdr)(int len, const char *arg);
}opchdr_t;
/* Global Variables *************************************************** */
static const opchdr_t ops[] = {
  { CPS_CLRCTL, prov_clrctl },
  { CPS_ADDR, provset_addr },
  { CPS_IVI, provset_ivi },
  { CPS_SYNCTIME, provset_synctime },
  { CPS_NETKEYID, provset_netkeyid },
  { CPS_NETKEYDONE, provset_netkeydone },
  { CPS_APPKEYID, provset_appkeyid },
  { CPS_APPKEYDONE, provset_appkeydone },

  { CPG_ALL, prov_get },

  /* Node related operations  */
  { CUPLG_CHECK, _upldev_check },
};

static const int ops_num = sizeof(ops) / sizeof(opchdr_t);

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

#if 0
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
#endif

static void cfgtest_ipc(void)
{
  const char s[] = "hello, cli";
  char r[50] = { 0 };
  int n;
  if (-1 == (n = recv(srv_sock.fd, r, 50, 0))) {
    LOGE("recv[fd:%d] [%s]\n", srv_sock.fd, strerror(errno));
  } else {
    LOGM("CFG received [%d:%s] from client.\n", n, r);
  }
  if (-1 == (n = send(srv_sock.fd, s, sizeof(s), 0))) {
    LOGE("send [%s]\n", strerror(errno));
  } else {
    LOGM("Send [%d:%s]\n", n, s);
  }
  LOGM("CFG Socket TEST DONE\n");
}

err_t cfg_init(void)
{
  err_t e;

  if (ec_success != (e = logging_init(CFG_LOG_FILE_PATH,
                                      0, /* Not output to stdout */
                                      LOG_MINIMAL_LVL(LVL_VER)))) {
    fprintf(stderr, "LOG INIT ERROR (%x)\n", e);
    return e;
  }

  EC(ec_success, cfgdb_init());
  gp_init(cft_json, NULL);

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
  return e;
}

int cfg_proc(int argc, char *argv[])
{
  err_t e;
  LOGM("CFG Process Started Up.\n");

  e = cfg_init();
  if (ec_success != e) {
    elog(e);
    exit(EXIT_FAILURE);
  }

  e = (err_t)(uintptr_t)cfg_mainloop(NULL);

  if (e != ec_success) {
    LOGW("CFG exits with err\n");
    elog(e);
  } else {
    LOGW("CFG exits success\n");
  }
  exit(0);
}

static err_t handle_cmd(void)
{
  char r[6] = { 0 };
  char *buf = NULL;
  int n, len, pos;
  err_t e = ec_not_supported;

  /* Get the opcode and length */
  n = recv(srv_sock.fd, r, 2, 0);
  if (-1 == n) {
    LOGE("recv err[%s]\n", strerror(errno));
    return err(ec_errno);
  } else if (0 == n) {
    e = err(ec_sock_closed);
    goto out;
  }

  if (r[1]) {
    buf = malloc(r[1]);
    len = r[1];
    while (len) {
      n = recv(srv_sock.fd, buf, len, 0);
      if (-1 == n) {
        LOGE("recv err[%s]\n", strerror(errno));
        e = err(ec_errno);
        goto out;
      } else if (0 == n) {
        e = err(ec_sock_closed);
        goto out;
      }
      len -= n;
    }
  }

  for (int i = 0; i < ops_num; i++) {
    if (ops[i].opc != r[0]) {
      continue;
    }
    e = ops[i].hdr(r[1], buf);
  }
  if (r[1]) {
    free(buf);
    buf = NULL;
  }

  memset(r, 0, 5);
  if (e == ec_success) {
    r[0] = RSP_OK;
    len = 2;
  } else {
    r[0] = RSP_ERR;
    r[1] = 4;
    memcpy(r + 2, &e, 4);
    len = 6;
  }

  pos = 0;
  while (pos != len) {
    if (-1 == (n = send(srv_sock.fd, r + pos, len - pos, 0))) {
      LOGE("send [%s]\n", strerror(errno));
      return err(ec_errno);
    }
    pos += n;
  }

  out:
  if (buf) {
    free(buf);
  }
  return e;
}

void *cfg_mainloop(void *p)
{
  /* cfg_init(); */
  /* cfg_proc(); */

  int n, maxfd;
  uid_t uid;
  fd_set rset, allset;
  err_t e;

  FD_ZERO(&allset);

  if (0 > (srv_sock.listenfd = serv_listen(CC_SOCK_SERV_PATH))) {
    LOGE("Serv[%s] Listen error [%s]\n", CC_SOCK_CLNT_PATH, strerror(errno));
    return (void *)(uintptr_t)err(ec_sock);
  }
  LOGV("CFG Wait for Socket Connection\n");
  FD_SET(srv_sock.listenfd, &allset);
  maxfd = srv_sock.listenfd;

  while (1) {
    rset = allset;  /* rset gets modified each time around */
    if ((n = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0) {
      LOGE("Select returns [%d], err[%s]\n", n, strerror(errno));
      return (void *)(uintptr_t)err(ec_sock);
    }

    if (FD_ISSET(srv_sock.listenfd, &rset) && srv_sock.fd < 0) {
      /* there is no connection now, so accept new client request */
      if ((srv_sock.fd = serv_accept(srv_sock.listenfd, &uid)) < 0) {
        LOGE("serv_accept returns [%d], err[%s]\n", srv_sock.fd, strerror(errno));
        return (void *)(uintptr_t)err(ec_sock);
      }
      FD_SET(srv_sock.fd, &allset);
      if (srv_sock.fd > maxfd) {
        maxfd = srv_sock.fd;  /* max fd for select() */
      }
      LOGM("new connection: uid %d, fd %d", uid, srv_sock.fd);
      cfgtest_ipc();
      continue;
    }
    if (srv_sock.fd >= 0 && FD_ISSET(srv_sock.fd, &rset)) {
      e = handle_cmd();
      if (ec_sock_closed == errof(e)) {
        LOGW("Socket closed.\n");
        FD_CLR(srv_sock.fd, &allset);
        close(srv_sock.fd);
        srv_sock.fd = -1;
      } else if (ec_success != e) {
        /* exit for now */
        exit(EXIT_FAILURE);
      }
    }
  }

  return (void *)(uintptr_t)err(ec_not_exist);
}

err_t sendto_client(opc_t opc, uint8_t len, const void *buf)
{
  return sock_send(&srv_sock, opc, len, buf);
}
