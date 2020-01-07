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
#include "startup.h"
#include "glib.h"
#include "socket_handler.h"
#include "gecko_bglib.h"
#include "dev_config.h"
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
static gboolean load_lists(gpointer key, gpointer value, gpointer data);
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
  /* LOGD("ncp init done\n"); */
  return ec_success;
}

/*
 * TODO: on_state_entry, on_state_exit
 */
static void set_mng_state(void)
{
  if (mng.state < starting || mng.state > blacklisting_devices_em) {
    return;
  }

  if (mng.state == adding_devices_em) {
    if (g_list_length(mng.lists.add)) {
      return;
    }
    /* All unprovisioned devices have been provisioned */
    if (mng.status.free_mode < 2) {
      clm_set_scan(0);
    }
    if (g_list_length(mng.lists.config) || mng.cache.config.used) {
      /* configuring in progress, switch directly */
      mng.state = configuring_devices_em;
    }
    mng.state = state_reload;
  } else if (mng.state == configuring_devices_em) {
    if (g_list_length(mng.lists.config) || mng.cache.config.used) {
      return;
    }
    /* All nodes have been configured properly */
    /* TODO */
    /* Need to stop the acc engine */
    mng.state = state_reload;
  } else if (mng.state == removing_devices_em) {
    if (g_list_length(mng.lists.rm) || mng.cache.config.used) {
      return;
    }
    /* All RM set nodes have been removed properly */
    /* TODO */
    /* Need to stop the acc engine */
    mng.state = state_reload;
  } else if (mng.state == blacklisting_devices_em) {
    if (g_list_length(mng.lists.rm) || mng.cache.bl.state != bl_idle) {
      return;
    }
    /* All RM set nodes have been removed properly */
    mng.state = state_reload;
  }

  if (g_list_length(mng.lists.add)) {
    if (mng.status.free_mode == 0) {
      clm_set_scan(1);
    }
    mng.state = adding_devices_em;
    acc_init(true);
  } else if (g_list_length(mng.lists.config)) {
    mng.state = configuring_devices_em;
    acc_init(true);
  } else if (g_list_length(mng.lists.rm)) {
    mng.state = removing_devices_em;
    acc_init(true);
  } else if (g_list_length(mng.lists.bl)) {
    mng.state = blacklisting_devices_em;
    acc_init(true);
  } else {
    mng.state = configured;
  }
}

void *mng_mainloop(void *p)
{
  while (1) {
    bool busy = false;
    poll_cmd();
    bgevt_dispenser();
    switch (mng.state) {
      case starting:
        if (file_modified(NW_NODES_CFG_FILE)) {
          load_cfg_file(NW_NODES_CFG_FILE, 0);
        }
        break;
      case adding_devices_em:
      /* Do adding_devices_em only jobs */
      case configuring_devices_em:
        busy |= acc_loop(&mng);
        break;
      case removing_devices_em:
        busy |= acc_loop(&mng);
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
    set_mng_state();
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

err_t clm_set_scan(int status)
{
  uint16_t ret;
  if (mng.status.free_mode == status) {
    return ec_success;
  }
  if (!mng.status.free_mode && status) {
    ret = gecko_cmd_mesh_prov_scan_unprov_beacons()->result;
    CHECK_BGCALL(ret, "scan unprov beacon");
  } else if (mng.status.free_mode && !status) {
    ret = gecko_cmd_mesh_prov_stop_scan_unprov_beacons()->result;
    CHECK_BGCALL(ret, "stop unprov beacon scanning");
  }
  mng.status.free_mode = status;
  LOGD("Scanning unprovisioned beacon [%s]\n", status ? "ON" : "OFF");
  return ec_success;
}

static inline void __lists_clr(void)
{
  g_list_free(mng.lists.add);
  mng.lists.add = NULL;
  g_list_free(mng.lists.bl);
  mng.lists.bl = NULL;
  g_list_free(mng.lists.config);
  mng.lists.config = NULL;
  g_list_free(mng.lists.rm);
  mng.lists.rm = NULL;
  g_list_free(mng.lists.fail);
  mng.lists.fail = NULL;
}

err_t mng_init(void *p)
{
  __lists_clr();
  memset(&mng, 0, sizeof(mng_t));
  mng.conn = 0xff;
  mng.cfg = get_provcfg();
  return ec_success;
}

void mng_load_lists(void)
{
  if (mng.state < configured) {
    return;
  }
  __lists_clr();
  cfg_load_mnglists(load_lists);
}

#define BL_BITMASK  0x01
#define RM_BITMASK  0x10

void list_nodes(void)
{
  struct gecko_msg_mesh_prov_ddb_list_devices_rsp_t *rsp;
  uint16_t cnt;

  rsp = gecko_cmd_mesh_prov_ddb_list_devices();

  if (rsp->result != bg_err_success) {
    LOGBGE("ddb list devices", rsp->result);
    return;
  }

  cnt = rsp->count;

  LOGM("%d nodes\n", cnt);
  while (cnt) {
    if (get_bguart_impl()->enc) {
      poll_update(50);
    }
    struct gecko_cmd_packet *evt = gecko_peek_event();
    if (NULL == evt
        || BGLIB_MSG_ID(evt->header) != gecko_evt_mesh_prov_ddb_list_id) {
      usleep(500);
      continue;
    }
    LOGD("dev - [%x:%x:%x]\n",
         evt->data.evt_mesh_prov_ddb_list.uuid.data[12],
         evt->data.evt_mesh_prov_ddb_list.uuid.data[11],
         evt->data.evt_mesh_prov_ddb_list.uuid.data[10]);
    cnt--;
  }
}

err_t clicb_list(int argc, char *argv[])
{
  list_nodes();
  bt_shell_printf("%s\n", __FUNCTION__);
  return err(ec_param_invalid);
}

static gboolean load_lists(gpointer key, gpointer value, gpointer data)
{
  node_t *n = (node_t *)value;
  uint16_t ret = gecko_cmd_mesh_prov_ddb_get(16, n->uuid)->result;
  int in = (ret == bg_err_success);

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
      list_nodes();
      LOGE("CFG and DDB in NCP are Out Of Sync - **Factory Reset Required?**\n");
      return TRUE;
    }
    if (n->rmorbl & BL_BITMASK) {
      mng.lists.bl = g_list_append(mng.lists.bl, n);
    } else if (n->rmorbl & RM_BITMASK) {
      mng.lists.rm = g_list_append(mng.lists.rm, n);
    } else if (!n->done) {
      mng.lists.config = g_list_append(mng.lists.config, n);
    }
  }
  LOGM("Lists One %s\n", n->addr ? "Node" : "Unprovisioned device");
  return FALSE;
}

err_t clicb_sync(int argc, char *argv[])
{
  int s = 1;

  if (argc > 1) {
    s = atoi(argv[1]);
  }
  if (s < 0 || s > 1) {
    return err(ec_param_invalid);
  }

  if (mng.state < starting && s) {
    mng.state = starting;
  } else if (mng.state >= starting && !s) {
    mng.state = stopping;
  }

  bt_shell_printf("%s\n", __FUNCTION__);
  return 0;
}

static void info_addr(const node_t *node)
{
  const char *v = nodeget_cfgstr(node->addr);
  struct gecko_msg_mesh_prov_ddb_get_rsp_t *rsp
    = gecko_cmd_mesh_prov_ddb_get(16, node->uuid);

  if (bg_err_success != rsp->result) {
    LOGE("Error (0x%04x) Happened when trying getting dcd\n", rsp->result);
    return;
  }
  cli_print_dev(node, rsp);
  if (v) {
    bt_shell_printf("%s\n", v);
  }
}

err_t clicb_info(int argc, char *argv[])
{
  uint16_t addr;
  node_t *n;
  if (argc < 2) {
    return err(ec_param_invalid);
  }
  for (int i = 1; i < argc; i++) {
    if (ec_success != str2uint(argv[i], strlen(argv[i]), &addr, sizeof(uint16_t))) {
      LOGD("str2uint failed\n");
      continue;
    }
    if (NULL == (n = cfgdb_node_get(addr))) {
      LOGW("Info CMD invalid address[%s]\n", argv[i]);
      bt_shell_printf("Info CMD invalid address[%s]\n", argv[i]);
      continue;
    }
    info_addr(n);
  }
  return ec_success;
}
