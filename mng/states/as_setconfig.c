/*************************************************************************
    > File Name: as_setconfig.c
    > Author: Kevin
    > Created Time: 2020-01-03
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "projconfig.h"
#include "dev_config.h"
#include "utils.h"
#include "logging.h"
/* Defines  *********************************************************** */
#define SET_TTL_MSG \
  "Node[%x]:  --- Set [TTL(%d)]\n"
#define SET_TTL_SUC_MSG \
  "Node[%x]:  --- Set [TTL(%d)] SUCCESS\n"
#define SET_TTL_FAIL_MSG \
  "Node[%x]:  --- Set [TTL(%d)] FAILED, Err <0x%04x>\n"

#define SET_FRIEND_MSG \
  "Node[%x]:  --- Set [Friend(%s)]\n"
#define SET_FRIEND_SUC_MSG \
  "Node[%x]:  --- Set [Friend(%s)] SUCCESS\n"
#define SET_FRIEND_FAIL_MSG \
  "Node[%x]:  --- Set [Friend(%s)] FAILED, Err <0x%04x>\n"

#define SET_RELAY_MSG \
  "Node[%x]:  --- Set [Relay(%s)]\n"
#define SET_RELAY_SUC_MSG \
  "Node[%x]:  --- Set [Relay(%s)] SUCCESS\n"
#define SET_RELAY_FAIL_MSG \
  "Node[%x]:  --- Set [Relay(%s)] FAILED, Err <0x%04x>\n"

#define SET_PROXY_MSG \
  "Node[%x]:  --- Set [Proxy(%s)]\n"
#define SET_PROXY_SUC_MSG \
  "Node[%x]:  --- Set [Proxy(%s)] SUCCESS\n"
#define SET_PROXY_FAIL_MSG \
  "Node[%x]:  --- Set [Proxy(%s)] FAILED, Err <0x%04x>\n"

#define SET_NETTX_MSG \
  "Node[%x]:  --- Set [nettx:count-interval(%d-%dms)]\n"
#define SET_NETTX_SUC_MSG \
  "Node[%x]:  --- Set [nettx:count-interval(%d-%dms)] SUCCESS\n"
#define SET_NETTX_FAIL_MSG \
  "Node[%x]:  --- Set [nettx:count-interval(%d-%dms)] FAILED, Err <0x%04x>\n"

#define SET_SNB_MSG \
  "Node[%x]:  --- Set [SNB(%s)]\n"
#define SET_SNB_SUC_MSG \
  "Node[%x]:  --- Set [SNB(%s)] SUCCESS\n"
#define SET_SNB_FAIL_MSG \
  "Node[%x]:  --- Set [SNB(%s)] FAILED, Err <0x%04x>\n"

#define ELEMENT_ITERATOR_INDEX  0
#define MODEL_ITERATOR_INDEX  1
#define SUB_ADDR_ITERATOR_INDEX  2

/* Global Variables *************************************************** */
enum {
  once_em,
  success_em,
  failed_em
};

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

/* Global Variables *************************************************** */
extern const char *stateNames[];

static const uint32_t events[] = {
  gecko_evt_mesh_config_client_default_ttl_status_id,
  gecko_evt_mesh_config_client_gatt_proxy_status_id,
  gecko_evt_mesh_config_client_friend_status_id,
  gecko_evt_mesh_config_client_relay_status_id,
  gecko_evt_mesh_config_client_beacon_status_id,
  gecko_evt_mesh_config_client_network_transmit_status_id
};

#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))
/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static void set_configs_print_state(int which,
                                    int process,
                                    config_cache_t *cache,
                                    uint16_t err);

static inline void on_feature_configured(config_cache_t *cache, features_em w)
{
  bool on;
  on = IS_BIT_SET(cache->node->config.features.target, w);
  if (on) {
    BIT_SET(cache->node->config.features.current, w);
  } else {
    BIT_CLR(cache->node->config.features.current, w);
  }
}
static inline int __next_config_item(config_cache_t *cache)
{
  lbitmap_t bits = 0;
  bits = ((cache->node->config.features.target) ^ cache->node->config.features.current) & 0x0000FFFF;
  if (!bits) {
    return -1;
  }
  return utils_ctz(bits);
}

static int __setconfig(config_cache_t *cache, mng_t *mng)
{
  struct gecko_msg_mesh_config_client_set_network_transmit_rsp_t *ntrsp;
  struct gecko_msg_mesh_config_client_set_relay_rsp_t *rrsp;
  struct gecko_msg_mesh_config_client_set_friend_rsp_t *frsp;
  struct gecko_msg_mesh_config_client_set_gatt_proxy_rsp_t *prsp;
  struct gecko_msg_mesh_config_client_set_default_ttl_rsp_t *trsp;
  struct gecko_msg_mesh_config_client_set_beacon_rsp_t *brsp;
  uint16_t retval;
  uint32_t handle;
  int which;

  which = __next_config_item(cache);

  switch (which) {
    case RELAY_BITOFS:
      rrsp = gecko_cmd_mesh_config_client_set_relay(
        mng->cfg->subnets[0].netkey.id,
        cache->node->addr,
        IS_BIT_SET(cache->node->config.features.target, RELAY_BITOFS),
        cache->node->config.features.relay_txp->cnt,
        cache->node->config.features.relay_txp->intv
        ? cache->node->config.features.relay_txp->intv : 50);
      retval = rrsp->result;
      handle = rrsp->handle;
      break;
    case PROXY_BITOFS:
      prsp = gecko_cmd_mesh_config_client_set_gatt_proxy(
        mng->cfg->subnets[0].netkey.id,
        cache->node->addr,
        IS_BIT_SET(cache->node->config.features.target, PROXY_BITOFS));
      retval = prsp->result;
      handle = prsp->handle;
      break;
    case FRIEND_BITOFS:
      frsp = gecko_cmd_mesh_config_client_set_friend(
        mng->cfg->subnets[0].netkey.id,
        cache->node->addr,
        IS_BIT_SET(cache->node->config.features.target, FRIEND_BITOFS));
      retval = frsp->result;
      handle = frsp->handle;
      break;
    case TTL_BITOFS:
      trsp = gecko_cmd_mesh_config_client_set_default_ttl(
        mng->cfg->subnets[0].netkey.id,
        cache->node->addr,
        *cache->node->config.ttl);
      retval = trsp->result;
      handle = trsp->handle;
      break;
    case NETTX_BITOFS:
      ntrsp = gecko_cmd_mesh_config_client_set_network_transmit(
        mng->cfg->subnets[0].netkey.id,
        cache->node->addr,
        cache->node->config.net_txp->cnt,
        cache->node->config.net_txp->intv);
      retval = ntrsp->result;
      handle = ntrsp->handle;
      break;
    case SNB_BITOFS:
      brsp = gecko_cmd_mesh_config_client_set_beacon(
        mng->cfg->subnets[0].netkey.id,
        cache->node->addr,
        IS_BIT_SET(cache->node->config.features.target, SNB_BITOFS));
      retval = brsp->result;
      handle = brsp->handle;
      break;
    default:
      return asr_notfnd;
  }

  if (retval != bg_err_success) {
    if (retval == bg_err_out_of_memory) {
      OOM_SET(cache);
      return asr_oom;
    }
    set_configs_print_state(which, failed_em, cache, retval);
    err_set_to_end(cache, retval, bgapi_em);
    LOGD("Node[%d]: To <<st_end>> State\n", cache->node->addr);
    return asr_bgapi;
  } else {
    /* TODO: Uncomment below line to print the ONCE_P */
    /* set_configs_print_state(which, once_em, cache, pconfig, 0); */
    WAIT_RESPONSE_SET(cache);
    cache->cc_handle = handle;
    /* TODO: startTimer(cache, 1); */
  }

  return asr_suc;
}

bool setconfig_guard(const config_cache_t *cache)
{
  return cache->node->config.features.current ^ cache->node->config.features.target;
}

int setconfig_entry(config_cache_t *cache, func_guard guard)
{
  int ret;
  if (guard && !guard(cache)) {
    LOGM("To Next State Since %s Guard Not Passed\n",
         stateNames[cache->state]);
    return asr_tonext;
  }

  ret = __setconfig(cache, get_mng());

  if (ret == asr_notfnd) {
    cache->next_state = -1;
    ret = asr_suc;
  }
  return ret;
}

int setconfig_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  int ret = 0, which = -1;
  uint32_t evtid;
  uint16_t retval;
  ASSERT(cache);
  ASSERT(evt);

  evtid = BGLIB_MSG_ID(evt->header);
  /* TODO: startTimer(cache, 0); */
  switch (evtid) {
    case gecko_evt_mesh_config_client_relay_status_id:
    {
      which = RELAY_BITOFS;
      retval = evt->data.evt_mesh_config_client_relay_status.result;
    }
    break;

    case gecko_evt_mesh_config_client_friend_status_id:
    {
      which = FRIEND_BITOFS;
      retval = evt->data.evt_mesh_config_client_friend_status.result;
    }
    break;

    case gecko_evt_mesh_config_client_gatt_proxy_status_id:
    {
      which = PROXY_BITOFS;
      retval = evt->data.evt_mesh_config_client_gatt_proxy_status.result;
    }
    break;

    case gecko_evt_mesh_config_client_default_ttl_status_id:
    {
      which = TTL_BITOFS;
      retval = evt->data.evt_mesh_config_client_default_ttl_status.result;
    }
    break;

    case gecko_evt_mesh_config_client_network_transmit_status_id:
    {
      which = NETTX_BITOFS;
      retval = evt->data.evt_mesh_config_client_network_transmit_status.result;
    }
    break;

    case gecko_evt_mesh_config_client_beacon_status_id:
    {
      which = SNB_BITOFS;
      retval = evt->data.evt_mesh_config_client_beacon_status.result;
    }
    break;

    default:
      LOGE("Unexpected event [0x%08x] happend in %s state.\n",
           evtid,
           stateNames[cache->state]);
      return asr_unspec;
  }

  WAIT_RESPONSE_CLEAR(cache);
  switch (retval) {
    case bg_err_success:
      RETRY_CLEAR(cache);
      set_configs_print_state(which, success_em, cache, 0);
      on_feature_configured(cache, which);
      break;
    case bg_err_timeout:
      /* bind any remaining_retry case here */
      if (!EVER_RETRIED(cache)) {
        cache->remaining_retry = SET_CONFIGS_RETRY_TIMES;
        EVER_RETRIED_SET(cache);
      } else if (cache->remaining_retry <= 0) {
        RETRY_CLEAR(cache);
        RETRY_OUT_PRINT(cache);
        err_set_to_end(cache, bg_err_timeout, bgevent_em);
        LOGD("Node[%d]: To <<st_end>> State\n", cache->node->addr);
      }
      return asr_suc;
      break;
    default:
      set_configs_print_state(which,
                              failed_em,
                              cache,
                              retval);
      err_set_to_end(cache, bg_err_timeout, bgevent_em);
      LOGD("Node[%d]: To <<st_end>> State\n", cache->node->addr);
      return asr_suc;
  }

  if (__next_config_item(cache) == -1) {
    cache->next_state = -1;
    return asr_suc;
  }

  ret = __setconfig(cache, get_mng());

  if (ret == asr_notfnd) {
    cache->next_state = -1;
    ret = asr_suc;
  }

  return ret;
}

int setconfig_retry(config_cache_t *cache, int reason)
{
  int ret = 0;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __setconfig(cache, get_mng());

  if (ret == asr_suc) {
    switch (reason) {
      case on_timeout_em:
        if (!EVER_RETRIED(cache) || cache->remaining_retry-- <= 0) {
          ASSERT(0);
        }
        RETRY_ONCE_PRINT(cache);
        break;
      case on_oom_em:
        ASSERT(OOM(cache));
        OOM_ONCE_PRINT(cache);
        OOM_CLEAR(cache);
        break;
      case on_guard_timer_expired_em:
        ASSERT(cache->expired);
        EXPIRED_ONCE_PRINT(cache);
        cache->expired = 0;
        break;
    }
  }
  return ret;
}

int setconfig_exit(void *p)
{
  return asr_suc;
}

bool is_setconfig_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}

static void set_configs_print_state(int which,
                                    int process,
                                    config_cache_t *cache,
                                    uint16_t err)
{
  switch (which) {
    case RELAY_BITOFS:
      switch (process) {
        case once_em:
          LOGD(SET_RELAY_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, RELAY_BITOFS) ? "ON" : "OFF");
          break;
        case success_em:
          LOGD(SET_RELAY_SUC_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, RELAY_BITOFS) ? "ON" : "OFF");
          break;
        case failed_em:
          LOGD(SET_RELAY_FAIL_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, RELAY_BITOFS) ? "ON" : "OFF",
               err);
          break;
        default:
          return;
      }
      break;
    case PROXY_BITOFS:
      switch (process) {
        case once_em:
          LOGD(SET_PROXY_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, PROXY_BITOFS) ? "ON" : "OFF");
          break;
        case success_em:
          LOGD(SET_PROXY_SUC_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, PROXY_BITOFS) ? "ON" : "OFF");
          break;
        case failed_em:
          LOGD(SET_PROXY_FAIL_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, PROXY_BITOFS) ? "ON" : "OFF",
               err);
          break;
        default:
          return;
      }
      break;
    case FRIEND_BITOFS:
      switch (process) {
        case once_em:
          LOGD(SET_FRIEND_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, FRIEND_BITOFS) ? "ON" : "OFF");
          break;
        case success_em:
          LOGD(SET_FRIEND_SUC_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, FRIEND_BITOFS) ? "ON" : "OFF");
          break;
        case failed_em:
          LOGD(SET_FRIEND_FAIL_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, FRIEND_BITOFS) ? "ON" : "OFF",
               err);
          break;
        default:
          return;
      }
      break;
    case TTL_BITOFS:
      switch (process) {
        case once_em:
          LOGD(SET_TTL_MSG,
               cache->node->addr,
               *cache->node->config.ttl);
          break;
        case success_em:
          LOGD(SET_TTL_SUC_MSG,
               cache->node->addr,
               *cache->node->config.ttl);
          break;
        case failed_em:
          LOGD(SET_TTL_FAIL_MSG,
               cache->node->addr,
               *cache->node->config.ttl,
               err);
          break;
        default:
          return;
      }
      break;
    case NETTX_BITOFS:
      switch (process) {
        case once_em:
          LOGD(SET_NETTX_MSG,
               cache->node->addr,
               cache->node->config.net_txp->cnt,
               cache->node->config.net_txp->intv);
          break;
        case success_em:
          LOGD(SET_NETTX_SUC_MSG,
               cache->node->addr,
               cache->node->config.net_txp->cnt,
               cache->node->config.net_txp->intv);
          break;
        case failed_em:
          LOGD(SET_NETTX_FAIL_MSG,
               cache->node->addr,
               cache->node->config.net_txp->cnt,
               cache->node->config.net_txp->intv,
               err);
          break;
        default:
          return;
      }
      break;
    case SNB_BITOFS:
      switch (process) {
        case once_em:
          LOGD(SET_SNB_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, SNB_BITOFS) ? "ON" : "OFF");
          break;
        case success_em:
          LOGD(SET_SNB_SUC_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, SNB_BITOFS) ? "ON" : "OFF");
          break;
        case failed_em:
          LOGD(SET_SNB_FAIL_MSG,
               cache->node->addr,
               IS_BIT_SET(cache->node->config.features.target, SNB_BITOFS) ? "ON" : "OFF",
               err);
          break;
        default:
          return;
      }
      break;
    default:
      return;
  }
}
