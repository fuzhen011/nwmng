/*************************************************************************
    > File Name: as_getdcd.c
    > Author: Kevin
    > Created Time: 2020-01-02
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "projconfig.h"
#include "dev_config.h"
#include "utils.h"
#include "logging.h"
#include "generic_parser.h"

/* Defines  *********************************************************** */
#define GENERIC_ONOFF_SERVER_MDID       0x1000
#define GENERIC_ONOFF_CLIENT_MDID       0x1001

#define LIGHT_LIGHTNESS_SERVER_MDID     0x1300
#define LIGHT_CTL_SERVER_MDID     0x1303
#define LIGHT_LC_SERVER_MDID  0x130f
#define SENSOR_SERVER_MDID  0x1100

#define CONFIGURATION_SERVER_MDID       0x0000
#define CONFIGURATION_CLIENT_MDID       0x0001
#define HEALTH_SERVER_MDID              0x0002
#define HEALTH_CLIENT_MDID              0x0003

#define ONCE_P(cache)                                        \
  do {                                                       \
    LOGV("Node[0x%04x]:  --- Get DCD\n", cache->node->addr); \
  } while (0)

#define SUC_P(cache)                                                 \
  do {                                                               \
    LOGD("Node[0x%04x]:  --- Get DCD SUCCESS\n", cache->node->addr); \
  } while (0)

#define FAIL_P(cache, err)                                    \
  do {                                                        \
    LOGE("Node[0x%04x]:  --- Get DCD Failed, Err <0x%04x>\n", \
         cache->node->addr,                                   \
         err);                                                \
  } while (0)

/* Global Variables *************************************************** */
extern const char *state_names[];

/* Static Variables *************************************************** */
static const uint32_t events[] = {
  gecko_evt_mesh_config_client_dcd_data_id,
  gecko_evt_mesh_config_client_dcd_data_end_id
};
#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))

/* Static Functions Declaractions ************************************* */
static void __dcd_store(const uint8_t *data,
                        uint8_t len,
                        config_cache_t *cache);

static int __dcd_get(config_cache_t *cache, mng_t *mng)
{
  struct gecko_msg_mesh_config_client_get_dcd_rsp_t *rsp;

  rsp = gecko_cmd_mesh_config_client_get_dcd(mng->cfg->subnets[0].netkey.id,
                                             cache->node->addr,
                                             0);

  if (rsp->result != bg_err_success) {
    if (rsp->result == bg_err_out_of_memory) {
      oom_set(cache);
      return asr_oom;
    }
    FAIL_P(cache, rsp->result);
    err_set_to_end(cache, rsp->result, bgapi_em);
    return asr_bgapi;
  } else {
    ONCE_P(cache);
    WAIT_RESPONSE_SET(cache);
    cache->cc_handle = rsp->handle;
    timer_set(cache, 1);
  }

  return asr_suc;
}

bool getdcd_guard(const config_cache_t *cache)
{
  return (cache->node && cache->node->addr != UNASSIGNED_ADDRESS);
}

int getdcd_entry(config_cache_t *cache, func_guard guard)
{
  /* Alarm SHOULD be set in the main engine */
  if (guard && !guard(cache)) {
    LOGW("State[%s] Guard Not Passed\n", state_names[cache->state]);
    return asr_tonext;
  }
  return __dcd_get(cache, get_mng());
}

int getdcd_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  uint32_t evtid;

  evtid = BGLIB_MSG_ID(evt->header);
  timer_set(cache, 0);
  switch (evtid) {
    case gecko_evt_mesh_config_client_dcd_data_id:
      /* Ignore pages other than 0 for now */
      if (evt->data.evt_mesh_config_client_dcd_data.page == 0) {
        LOGV("Node[0x%04x]: DCD Page 0 Received\n", cache->node->addr);
        __dcd_store(evt->data.evt_mesh_config_client_dcd_data.data.data,
                    evt->data.evt_mesh_config_client_dcd_data.data.len,
                    cache);
      }
      break;

    case gecko_evt_mesh_config_client_dcd_data_end_id:
    {
      WAIT_RESPONSE_CLEAR(cache);
      switch (evt->data.evt_mesh_config_client_dcd_data_end.result) {
        case bg_err_success:
          RETRY_CLEAR(cache);
          SUC_P(cache);
          if (cache->node->err > ERROR_BIT(get_dcd_em)
              && cache->node->err < ERROR_BIT(end_em)) {
            for (int a = addappkey_em; a < end_em; a++) {
              if (cache->node->err & ERROR_BIT(a)) {
                LOGM("Node[0x%04x]: Configure the Node From <<<%s>>> State\n",
                     cache->node->addr,
                     state_names[a]);
                cache->next_state = a;
              }
            }
          } else {
            cache->next_state = -1;
          }
          break;
        case bg_err_timeout:
          /* add any retry case here */
          if (!EVER_RETRIED(cache)) {
            cache->remaining_retry = GET_DCD_RETRY_TIMES;
            EVER_RETRIED_SET(cache);
          } else if (cache->remaining_retry <= 0) {
            RETRY_CLEAR(cache);
            RETRY_OUT_PRINT(cache);
            err_set_to_end(cache, bg_err_timeout, bgevent_em);
          }
          break;
        default:
          FAIL_P(cache,
                 evt->data.evt_mesh_config_client_dcd_data_end.result);
          err_set_to_end(cache, bg_err_timeout, bgevent_em);
          break;
      }
    }
    break;

    default:
      LOGE("Unexpected event [0x%08x] happend in %s state.\n",
           evtid,
           state_names[cache->state]);
      return asr_unspec;
  }

  return asr_suc;
}

int getdcd_retry(config_cache_t *cache, int reason)
{
  int ret = 0;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __dcd_get(cache, get_mng());

  if (ret != asr_suc) {
    return ret;
  }
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
  return ret;
}

int getdcd_exit(void *p)
{
  return asr_suc;
}

bool is_getdcd_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}

static void __dcd_store(const uint8_t *data,
                        uint8_t len,
                        config_cache_t *cache)
{
  uint8_t i = 0, eles = 0, sigm_cnt = 0, vm_cnt = 0;
  dcd_t *dcd = &cache->dcd;
  i = 12;
  sigm_cnt = data[i];
  vm_cnt = data[i + 1];
  i += 2;
  do {
    i += sizeof(uint16_t) * sigm_cnt;
    i += sizeof(uint16_t) * vm_cnt * 2;
    eles++;
  } while (i < len);

  dcd->element_cnt = eles;

  i = 8;
  dcd->feature = BUILD_UINT16(data[i], data[i + 1]);
  i += 2;
  if (dcd->elems) {
    free(dcd->elems);
  }
  dcd->elems = (elem_t *)calloc(dcd->element_cnt, sizeof(elem_t));
  i += 2;   /* skip loc */
  for (uint8_t e = 0; e < dcd->element_cnt; e++) {
    if (e) {
      i += 2;   /* skip loc */
    }
    dcd->elems[e].sigm_cnt = data[i++];
    dcd->elems[e].vm_cnt = data[i++];
    if (dcd->elems[e].sigm_cnt) {
      if (dcd->elems[e].sig_models) {
        free(dcd->elems[e].sig_models);
      }
      dcd->elems[e].sig_models = (uint16_t *)calloc(dcd->elems[e].sigm_cnt,
                                                    sizeof(uint16_t));
      uint8_t offset = 0;
      for (uint8_t ms = 0; ms < dcd->elems[e].sigm_cnt; ms++) {
        uint16_t mdid = BUILD_UINT16(data[i], data[i + 1]);
        if (mdid == GENERIC_ONOFF_SERVER_MDID) {
          cache->node->models.func |= ONOFF_SV_BIT;
        } else if (mdid == LIGHT_LIGHTNESS_SERVER_MDID) {
          cache->node->models.func |= LIGHTNESS_SV_BIT;
        } else if (mdid == LIGHT_CTL_SERVER_MDID) {
          cache->node->models.func |= CTL_SV_BIT;
        } else if (mdid == SENSOR_SERVER_MDID) {
          cache->node->models.func |= SENSOR_SV_BIT;
        } else if (mdid == LIGHT_LC_SERVER_MDID) {
          cache->node->models.func |= LC_SV_BIT;
        }
#if 0
        if (mdid == GENERIC_ONOFF_CLIENT_MDID) {
          CS_LOG("-|||>>>OnOff Client Node (Switch)<<<|||-\n");
        } else if (mdid == GENERIC_ONOFF_SERVER_MDID) {
          CS_LOG("-|||>>>OnOff Server Node (Light)<<<|||-\n");
        }
#endif
        i += 2;
        if (mdid == CONFIGURATION_CLIENT_MDID
            || mdid == CONFIGURATION_SERVER_MDID) {
          offset++;
          continue;
        }
        dcd->elems[e].sig_models[ms - offset] = mdid;
      }
      dcd->elems[e].sigm_cnt -= offset;
    }
    nodeset_func(cache->node->addr, cache->node->models.func);

    if (dcd->elems[e].vm_cnt) {
      if (dcd->elems[e].vm) {
        free(dcd->elems[e].vm);
      }
      dcd->elems[e].vm = (vendor_model_t *)calloc(dcd->elems[e].vm_cnt,
                                                  sizeof(vendor_model_t));
      for (uint8_t ms = 0; ms < dcd->elems[e].vm_cnt; ms++) {
        dcd->elems[e].vm[ms].vid = BUILD_UINT16(data[i], data[i + 1]);
        i += 2;
        dcd->elems[e].vm[ms].mid = BUILD_UINT16(data[i], data[i + 1]);
        i += 2;
      }
    }
  }
}
