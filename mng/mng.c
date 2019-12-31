/*************************************************************************
    > File Name: mng/mng.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/* #include <sys/prctl.h> */

#include <sys/socket.h>

#include "hal/bg_uart_cbs.h"
#include "host_gecko.h"

#include "projconfig.h"
#include "logging.h"
#include "utils.h"
#include "generic_parser.h"
#include "gecko_bglib.h"
#include "bgevt_hdr.h"
#include "mng.h"
#include "nwk.h"
#include "cli.h"
#include "cfg.h"
#include "climng_startup.h"
#include "glib.h"
/* Defines  *********************************************************** */
#define INVALID_CONN_HANDLE 0xff

/* Global Variables *************************************************** */
extern const command_t commands[];
extern pthread_mutex_t qlock, hdrlock;
extern pthread_cond_t qready, hdrready;

err_t cmd_ret = ec_success;

typedef struct qitem{
  int offs;
  wordexp_t *cmd;
  struct qitem *next;
}qitem_t;

typedef struct {
  qitem_t *head;
  qitem_t *tail;
}cmdq_t;

cmdq_t cmdq = { 0 };

/* Static Variables *************************************************** */
static mng_t mng = {
  .conn = 0xff
};

/* Static Functions Declaractions ************************************* */
static void poll_cmd(void);
static void load_lists(gpointer key, gpointer value, gpointer data);
/******************************************************************
 * Command queue
 * ***************************************************************/
void cmd_enq(const char *str, int offs)
{
  wordexp_t *w = NULL;
  if (!str) {
    return;
  }
  w = malloc(sizeof(wordexp_t));
  if (wordexp(str, w, WRDE_NOCMD)) {
    free(w);
    return;
  }
  qitem_t *qi = malloc(sizeof(qitem_t));
  qi->cmd = w;
  qi->offs = offs;
  qi->next = NULL;
  PTMTX_LOCK(&qlock);
  if (cmdq.tail) {
    cmdq.tail->next = qi;
    cmdq.tail = qi;
  } else {
    cmdq.tail = cmdq.head = qi;
  }
  PTMTX_UNLOCK(&qlock);
}

wordexp_t *cmd_deq(int *offs)
{
  wordexp_t *w = NULL;
  qitem_t *qi;

  PTMTX_LOCK(&qlock);
  if (!cmdq.head) {
    goto out;
  }

  w = cmdq.head->cmd;
  *offs = cmdq.head->offs;
  qi = cmdq.head;
  cmdq.head = cmdq.head->next;
  if (!cmdq.head) {
    cmdq.tail = NULL;
  }
  free(qi);

  out:
  PTMTX_UNLOCK(&qlock);
  return w;
}

mng_t *get_mng(void)
{
  return &mng;
}

void __ncp_exit(void)
{
  gecko_cmd_system_reset(0);
  LOGD("MNG Exit\n");
}

err_t clr_all(void *p)
{
  int ret;
  err_t e;

  EC(ec_success, cfg_clrctl());

  if (mng.conn != INVALID_CONN_HANDLE) {
    if (bg_err_success != (ret = gecko_cmd_le_connection_close(mng.conn)->result)) {
      LOGBGE("Disconnect", ret);
      return err(ec_bgrsp);
    }
    mng.conn = INVALID_CONN_HANDLE;
  }

  if (bg_err_success != (ret = gecko_cmd_flash_ps_erase_all()->result)) {
    LOGBGE("Erase all", ret);
    return err(ec_bgrsp);
  }
  usleep(300 * 1000);
  return ec_success;
}

err_t init_ncp(void *p)
{
  const proj_args_t *pg = getprojargs();
  if (!pg->initialized) {
    return err(ec_state);
  }

  bguart_init(pg->enc,
              pg->enc ? (char *)pg->sock.srv : (char *)pg->port,
              pg->enc ? (char *)pg->sock.clt : NULL);

  conn_ncptarget();
  sync_host_and_ncp_target();
  atexit(__ncp_exit);
  LOGD("ncp init done\n");
  return ec_success;
}

void *mng_mainloop(void *p)
{
  while (1) {
    bool busy = false;
    poll_cmd();
    bgevt_dispenser();
    switch (mng.state) {
      case starting:
        /* TODO: load lists */
        break;
      case stopping:
        mng.state = configured;
        break;
      case state_reload:
        /* TODO: Load actions */
        break;
      default:
        break;
    }
    if (!busy) {
      usleep(10 * 1000);
    }
  }
  return NULL;
}

static void poll_cmd(void)
{
  int i;
  wordexp_t *w = cmd_deq(&i);
  if (w) {
    DUMP_PARAMS(w->we_wordc, w->we_wordv);
    if (ec_param_invalid == commands[i].fn(w->we_wordc, w->we_wordv)) {
      printf(COLOR_HIGHLIGHT "Invalid Parameter(s)\nUsage: " COLOR_OFF);
      print_cmd_usage(&commands[i]);
    }
    wordfree(w);
    free(w);
  }
}

err_t clm_set_scan(int onoff)
{
  uint16_t ret;
  if (!!mng.status.free_mode == onoff) {
    return ec_success;
  }
  if (onoff) {
    ret = gecko_cmd_mesh_prov_scan_unprov_beacons()->result;
    CHECK_BGCALL(ret, "scan unprov beacon");
  } else {
    ret = gecko_cmd_mesh_prov_stop_scan_unprov_beacons()->result;
    CHECK_BGCALL(ret, "stop unprov beacon scanning");
  }
  mng.status.free_mode = onoff ? fm_freemode : fm_idle;
  LOGD("Scanning unprovisioned beacon [%s]\n", onoff ? "ON" : "OFF");
  return ec_success;
}

err_t mng_init(void *p)
{
  memset(&mng, 0, sizeof(mng_t));
  mng.conn = 0xff;
  mng.cfg = get_provcfg();
  return ec_success;
}

void mng_load_lists(void)
{
  cfg_load_mnglists(load_lists);
}

#define BL_BITMASK  0x01
#define RM_BITMASK  0x10

static void load_lists(gpointer key, gpointer value, gpointer data)
{
  node_t *n = (node_t *)value;
  int in = (gecko_cmd_mesh_prov_ddb_get(16, n->uuid)->result == bg_err_success);

  if (!n->addr) {
    if (in) {
      gecko_cmd_mesh_prov_ddb_delete(*(uuid_128 *)n->uuid);
    }
    if (!n->rmorbl) {
      mng.lists.add = g_list_append(mng.lists.add, n);
    }
  } else {
    /* Priority: Blacklist > remove > config */
    if (!in) {
      /* TODO - set a flag */
      LOGE("CFG and DDB in NCP are Out Of Sync - **Factory Reset Required?**\n");
      /* return TRUE; */
    }
    if (n->rmorbl & BL_BITMASK) {
      mng.lists.bl = g_list_append(mng.lists.bl, n);
    } else if (n->rmorbl & RM_BITMASK) {
      mng.lists.rm = g_list_append(mng.lists.rm, n);
    } else if (!n->done) {
      mng.lists.config = g_list_append(mng.lists.config, n);
    }
  }
  /* return FALSE; */
}

/* int mng_evt_hdr(const void *ve) */
/* { */
/* } */

err_t clicb_sync(int argc, char *argv[])
{
  int s = 1;

  if (argc > 1) {
    s = atoi(argv[1]);
  }
  if (s < 0 || s > 1) {
    return err(ec_param_invalid);
  }
  cfg_init(NULL);

  return ec_success;

  if (mng.state < starting && s) {
    mng.state = starting;
  } else if (mng.state >= starting && !s) {
    mng.state = stopping;
  }

  bt_shell_printf("%s\n", __FUNCTION__);
  return 0;
}
