/*************************************************************************
    > File Name: as_setpub.c
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
#define SET_PUB_MSG \
  "Node[%x]:  --- Pub [Element-Model(%d-%04x:%04x) -> 0x%04x]\n"
#define SET_PUB_SUC_MSG \
  "Node[%x]:  --- Pub [Element-Model(%d-%04x:%04x) -> 0x%04x] SUCCESS\n"
#define SET_PUB_FAIL_MSG \
  "Node[%x]:  --- Pub [Element-Model(%d-%04x:%04x) -> 0x%04x] FAILED, Err <0x%04x>\n"

#define ELEMENT_ITERATOR_INDEX  0
#define MODEL_ITERATOR_INDEX  1
/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
#define ONCE_P(cache)                              \
  do {                                             \
    LOGD(SET_PUB_MSG,                              \
         cache->node->addr,                        \
         cache->iterators[ELEMENT_ITERATOR_INDEX], \
         cache->vnm.vd,                            \
         cache->vnm.md,                            \
         cache->node->config.pub->addr);           \
  } while (0)

#define SUC_P(cache)                               \
  do {                                             \
    LOGD(SET_PUB_SUC_MSG,                          \
         cache->node->addr,                        \
         cache->iterators[ELEMENT_ITERATOR_INDEX], \
         cache->vnm.vd,                            \
         cache->vnm.md,                            \
         cache->node->config.pub->addr);           \
  } while (0)

#define FAIL_P(cache, err)                         \
  do {                                             \
    LOGE(SET_PUB_FAIL_MSG,                         \
         cache->node->addr,                        \
         cache->iterators[ELEMENT_ITERATOR_INDEX], \
         cache->vnm.vd,                            \
         cache->vnm.md,                            \
         cache->node->config.pub->addr,            \
         err);                                     \
  } while (0)

/* Global Variables *************************************************** */
extern const char *stateNames[];

static const uint32_t events[] = {
  gecko_evt_mesh_config_client_model_pub_status_id
};

static const uint16_t not_pub_models[] = {
  0x1301,
  0x1007,
  0x1304,
  0x1204
};

#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))
#define NSPT_PUB_MODEL_NUM() (sizeof(not_pub_models) / sizeof(uint16_t))
/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static int iter_setpub(config_cache_t *cache);

static int __setpub(config_cache_t *cache, mng_t *mng)
{
  int ret;
  uint16_t key_id = 0;
  struct gecko_msg_mesh_config_client_set_model_pub_rsp_t *rsp;

  cache->vnm.vd =
    cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
    ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].vid
    : 0xFFFF;
  cache->vnm.md =
    cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
    ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].mid
    : cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sig_models[cache->iterators[MODEL_ITERATOR_INDEX]];
  ret = appkey_by_refid(mng,
                        cache->node->config.pub->aki,
                        &key_id);
  ASSERT(ret == asr_suc);

  rsp = gecko_cmd_mesh_config_client_set_model_pub(
    mng->cfg->subnets[0].netkey.id,
    cache->node->addr,
    cache->iterators[ELEMENT_ITERATOR_INDEX],
    cache->vnm.vd,
    cache->vnm.md,
    cache->node->config.pub->addr,
    key_id,
    0,
    cache->node->config.pub->ttl,
    cache->node->config.pub->period,
    cache->node->config.pub->txp.cnt,
    cache->node->config.pub->txp.intv ? cache->node->config.pub->txp.intv : 50);

  if (rsp->result != bg_err_success) {
    if (rsp->result == bg_err_out_of_memory) {
      oom_set(cache);
      return asr_oom;
    }
    FAIL_P(cache, rsp->result);
    err_set_to_end(cache, rsp->result, bgapi_em);
    LOGE("Node[%x]: To <<End>> State\n", cache->node->addr);
    return asr_bgapi;
  } else {
    ONCE_P(cache);
    WAIT_RESPONSE_SET(cache);
    cache->cc_handle = rsp->handle;
    /* TODO: startTimer(cache, 1); */
  }

  return asr_suc;
}

bool setpub_guard(const config_cache_t *cache)
{
  if (!cache->node->config.pub
      || !cache->node->config.bindings
      || !cache->node->config.bindings->len) {
    return false;
  }
  for (int i = 0; i < cache->node->config.bindings->len; i++) {
    if (cache->node->config.bindings->data[i] == cache->node->config.pub->aki) {
      return true;
    }
  }
  return false;
}

int setpub_entry(config_cache_t *cache, func_guard guard)
{
  if (guard && !guard(cache)) {
    LOGM("To Next State Since %s Guard Not Passed\n",
         stateNames[cache->state]);
    return asr_tonext;
  }

  return __setpub(cache, get_mng());
}

int setpub_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  uint32_t evtid;
  ASSERT(cache);
  ASSERT(evt);

  evtid = BGLIB_MSG_ID(evt->header);
  /* TODO: startTimer(cache, 0); */
  switch (evtid) {
    case gecko_evt_mesh_config_client_model_pub_status_id:
    {
      WAIT_RESPONSE_CLEAR(cache);
      switch (evt->data.evt_mesh_config_client_model_pub_status.result) {
        case bg_err_success:
        case bg_err_mesh_not_initialized:
          if (evt->data.evt_mesh_config_client_model_pub_status.result
              == bg_err_mesh_not_initialized) {
            LOGD("0x%04x Model doesn't support publishing\n", cache->vnm.md);
          }
          RETRY_CLEAR(cache);
          SUC_P(cache);
          break;
        case bg_err_timeout:
          /* bind any remaining_retry case here */
          if (!EVER_RETRIED(cache)) {
            cache->remaining_retry = SET_PUB_RETRY_TIMES;
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
          FAIL_P(cache,
                 evt->data.evt_mesh_config_client_model_pub_status.result);
          err_set_to_end(cache, evt->data.evt_mesh_config_client_model_pub_status.result, bgevent_em);
          LOGW("To <<End>> State\n");
          return asr_suc;
      }

      if (iter_setpub(cache) == 1) {
        cache->next_state = -1;
        return asr_suc;
      }

      return __setpub(cache, get_mng());
    }
    break;

    default:
      LOGE("Unexpected event [0x%08x] happend in %s state.\n",
           evtid,
           stateNames[cache->state]);
      return asr_unspec;
  }

  return asr_suc;
}

int setpub_retry(config_cache_t *cache, int reason)
{
  int ret;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __setpub(cache, get_mng());

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

int setpub_exit(void *p)
{
  return asr_suc;
}

bool is_setpub_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}

static inline bool __pub_supported(uint16_t md)
{
  for (int i = 0; i < NSPT_PUB_MODEL_NUM(); i++) {
    if (not_pub_models[i] == md) {
      return false;
    }
  }
  return true;
}

static int iter_setpub(config_cache_t *cache)
{
  int md, increment = 0;

  do {
    if (++cache->iterators[MODEL_ITERATOR_INDEX]
        == cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
        + cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm_cnt) {
      cache->iterators[MODEL_ITERATOR_INDEX] = 0;
      increment = 0;
      break;
    } else {
      md =
        cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
        ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].mid
        : cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sig_models[cache->iterators[MODEL_ITERATOR_INDEX]];

      if (__pub_supported(md)) {
        increment = 1;
        break;
      }
    }
  } while (1);

  if (!increment) {
    if (++cache->iterators[ELEMENT_ITERATOR_INDEX] == cache->dcd.element_cnt) {
      cache->iterators[ELEMENT_ITERATOR_INDEX] = 0;
      return 1;
    }
  }
  return 0;
}
