/*************************************************************************
    > File Name: nwk.c
    > Author: Kevin
    > Created Time: 2019-12-23
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <unistd.h>

#include "cli.h"
#include "nwk.h"
#include "mng.h"
#include "logging.h"
#include "gecko_bglib.h"
#include "bg_uart_cbs.h"
#include "socket_handler.h"
#include "generic_parser.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static err_t on_initialized_config(struct gecko_msg_mesh_prov_initialized_evt_t *ein);

err_t nwk_init(void *p)
{
  uint16_t ret;
  struct gecko_cmd_packet *evt;

  mng_t *mng = get_mng();
  if (bg_err_success != (ret = gecko_cmd_mesh_prov_init()->result)) {
    LOGE("Error (0x%04x) Happened when trying initializing provisioner\n", ret);
    return err(ec_bgrsp);
  }

  while (1) {
    evt = gecko_peek_event();
    if (NULL == evt
        || BGLIB_MSG_ID(evt->header) != gecko_evt_mesh_prov_initialized_id) {
      usleep(500);
      continue;
    }
    LOGM("NCP: prov initialized\n");
    mng->state = initialized;
    err_t ein = on_initialized_config(&evt->data.evt_mesh_prov_initialized);
    if (ec_success == ein) {
      mng->state = configured;
      mng_load_lists(); /* do the initial loading */
    }
    return ein;
  }
}

static err_t new_netkey(mng_t *mng)
{
  err_t e;
  struct gecko_msg_mesh_prov_create_network_rsp_t *rsp;

  if (mng->cfg->subnets[0].netkey.done) {
    return ec_success;
  }

  rsp = gecko_cmd_mesh_prov_create_network(16,
                                           mng->cfg->subnets[0].netkey.val);

  if (rsp->result == bg_err_success || rsp->result == bg_err_mesh_already_exists) {
    mng->cfg->subnets[0].netkey.id = rsp->network_id;
    mng->cfg->subnets[0].netkey.done = 1;

    EC(ec_success, provset_netkeyid(&mng->cfg->subnets[0].netkey.id));
    EC(ec_success, provset_netkeydone(&mng->cfg->subnets[0].netkey.done));
    return ec_success;
  } else if (rsp->result == bg_err_out_of_memory
             || rsp->result == bg_err_mesh_limit_reached) {
    LOGBGE("create netkey (max reach?)", rsp->result);
  } else {
    LOGBGE("create netkey", rsp->result);
  }
  return err(ec_bgrsp);
}

static err_t new_appkeys(mng_t *mng)
{
  err_t e;
  int tmp = 0;
  struct gecko_msg_mesh_prov_create_appkey_rsp_t *rsp;

  if (!mng->cfg->subnets[0].netkey.done) {
    LOGE("Must create network before creating appkeys.\n");
    return err(ec_state);
  }

  for (int i = 0; i < mng->cfg->subnets[0].appkey_num; i++) {
    meshkey_t *appkey = &mng->cfg->subnets[0].appkey[i];
    if (appkey->done) {
      tmp++;
      continue;
    }
    rsp = gecko_cmd_mesh_prov_create_appkey(mng->cfg->subnets[0].netkey.id,
                                            16,
                                            appkey->val);

    if (rsp->result == bg_err_success
        || rsp->result == bg_err_mesh_already_exists) {
      appkey->id = rsp->appkey_index;
      appkey->done = 1;
      EC(ec_success, provset_appkeyid(&appkey->refid, &appkey->id));
      EC(ec_success, provset_appkeydone(&appkey->refid, &appkey->done));
      tmp++;
    } else if (rsp->result == bg_err_out_of_memory
               || rsp->result == bg_err_mesh_limit_reached) {
      LOGBGE("create appkey (max reach?)", rsp->result);
    } else {
      LOGBGE("create appkey", rsp->result);
    }
  }
  mng->cfg->subnets[0].active_appkey_num = tmp;
  return ec_success;
}

static void self_config(const mng_t *mng)
{
  uint16_t ret;
  if (mng->cfg->net_txp) {
    if (bg_err_success != (ret = gecko_cmd_mesh_test_set_nettx(
                             mng->cfg->net_txp->cnt,
                             mng->cfg->net_txp->intv)->result)) {
      LOGBGE("Set local nettx", ret);
      return;
    }
    LOGM("Set local network transmission Count/Interval [%d/%dms] Success\n",
         mng->cfg->net_txp->cnt + 1,
         (mng->cfg->net_txp->intv + 1) * 10);
  }
  if (mng->cfg->timeout) {
    if (bg_err_success != (ret = gecko_cmd_mesh_config_client_set_default_timeout(
                             mng->cfg->timeout->normal,
                             mng->cfg->timeout->lpn)->result)) {
      LOGBGE("Set local timeouts", ret);
      return;
    }
    LOGM("Set config client timeout [normal node/LPN] = [%dms/%dms] Success\n",
         mng->cfg->timeout->normal,
         mng->cfg->timeout->lpn);
  }
}

static err_t on_initialized_config(struct gecko_msg_mesh_prov_initialized_evt_t *ein)
{
  err_t e;
  mng_t *mng = get_mng();

  /* pProv->state = initialized_em; */
  /* mng->cfg.addr = ein->address; */
  /* mng->cfg.ivi = ein->ivi; */
  /* Turncate to 1 since only 1 subnet is supported now */
  /* mng->cfg.subnet_num = ein->networks ? 1 : 0; */

  if (ein->networks == 0 && (mng->cfg->addr || mng->cfg->ivi)) {
    uint16_t ret = gecko_cmd_mesh_prov_initialize_network(mng->cfg->addr,
                                                          mng->cfg->ivi)->result;
    if (bg_err_success != ret) {
      LOGBGE("init network", ret);
      return err(ec_bgrsp);
    }
  } else if (ein->networks && (!mng->cfg->subnets || !mng->cfg->subnets[0].netkey.done)) {
    bt_shell_printf(COLOR_HIGHLIGHT
                    "**NETKEY MISMATCH**, NEED FACTORY RESET?"
                    COLOR_OFF
                    "\n");
    return err(ec_state);
  } else if (ein->networks) {
    if (ein->address != mng->cfg->addr) {
      mng->cfg->addr = ein->address;
      EC(ec_success, provset_addr(&mng->cfg->addr));
    }
    if (ein->ivi != mng->cfg->ivi) {
      mng->cfg->ivi = ein->ivi;
      EC(ec_success, provset_ivi(&mng->cfg->ivi));
    }
  }

  /* Network Keys */
  EC(ec_success, new_netkey(mng));
  /* App Keys */
  EC(ec_success, new_appkeys(mng));
  /* set nettx and timeouts */
  self_config(mng);

  return ec_success;
}


int bgevt_dflt_hdr(const struct gecko_cmd_packet *evt)
{
  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_le_gap_adv_timeout_id:
      /* Ignore */
      break;
    default:
      return 0;
  }
  return 1;
}
