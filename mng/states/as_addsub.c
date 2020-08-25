/*************************************************************************
    > File Name: as_addsub.c
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
#define ELEMENT_ITERATOR_INDEX  0
#define MODEL_ITERATOR_INDEX  1
#define SUB_ADDR_ITERATOR_INDEX  2

#define ONCE_P(cache)                                                                \
  do {                                                                               \
    LOGV(                                                                            \
      "Node[0x%04x]:  --- Sub [Element-Model(%d-%04x:%04x) <- 0x%04x]\n",            \
      cache->node->addr,                                                             \
      cache->iterators[ELEMENT_ITERATOR_INDEX],                                      \
      cache->vnm.vd,                                                                 \
      cache->vnm.md,                                                                 \
      cache->node->config.sublist->data[cache->iterators[SUB_ADDR_ITERATOR_INDEX]]); \
  } while (0)

#define SUC_P(cache)                                                                 \
  do {                                                                               \
    LOGD(                                                                            \
      "Node[0x%04x]:  --- Sub [Element-Model(%d-%04x:%04x) <- 0x%04x] SUCCESS\n",    \
      cache->node->addr,                                                             \
      cache->iterators[ELEMENT_ITERATOR_INDEX],                                      \
      cache->vnm.vd,                                                                 \
      cache->vnm.md,                                                                 \
      cache->node->config.sublist->data[cache->iterators[SUB_ADDR_ITERATOR_INDEX]]); \
  } while (0)

#define FAIL_P(cache, err)                                                                     \
  do {                                                                                         \
    LOGE(                                                                                      \
      "Node[0x%04x]:  --- Sub [Element-Model(%d-%04x:%04x) <- 0x%04x] FAILED, Err <0x%04x>\n", \
      cache->node->addr,                                                                       \
      cache->iterators[ELEMENT_ITERATOR_INDEX],                                                \
      cache->vnm.vd,                                                                           \
      cache->vnm.md,                                                                           \
      cache->node->config.sublist->data[cache->iterators[SUB_ADDR_ITERATOR_INDEX]],            \
      err);                                                                                    \
  } while (0)

/* Global Variables *************************************************** */
extern const char *state_names[];

/* Static Variables *************************************************** */
static const uint32_t events[] = {
  gecko_evt_mesh_config_client_model_sub_status_id
};

static const uint16_t not_sub_models[] = {
  0x1201, /* Time Setup Server */
};

#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))
#define NSPT_SUB_MODEL_NUM() (sizeof(not_sub_models) / sizeof(uint16_t))

/* Static Functions Declaractions ************************************* */
static int iter_addsub(config_cache_t *cache);

static int __addsub(config_cache_t *cache, mng_t *mng)
{
  struct gecko_msg_mesh_config_client_add_model_sub_rsp_t *arsp;
  struct gecko_msg_mesh_config_client_set_model_sub_rsp_t *srsp;
  uint16_t retval;
  uint32_t handle;

  if (cache->iterators[SUB_ADDR_ITERATOR_INDEX] == 0) {
    cache->vnm.vd =
      cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
      ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].vid
      : 0xFFFF;
    cache->vnm.md =
      cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
      ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].mid
      : cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sig_models[cache->iterators[MODEL_ITERATOR_INDEX]];

    srsp = gecko_cmd_mesh_config_client_set_model_sub(
      mng->cfg->subnets[0].netkey.id,
      cache->node->addr,
      cache->iterators[ELEMENT_ITERATOR_INDEX],
      cache->vnm.vd,
      cache->vnm.md,
      cache->node->config.sublist->data[cache->iterators[SUB_ADDR_ITERATOR_INDEX]]);
    retval = srsp->result;
    handle = srsp->handle;
  } else {
    cache->vnm.vd =
      cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
      ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].vid
      : 0xFFFF;
    cache->vnm.md =
      cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
      ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].mid
      : cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sig_models[cache->iterators[MODEL_ITERATOR_INDEX]];
    arsp = gecko_cmd_mesh_config_client_add_model_sub(
      mng->cfg->subnets[0].netkey.id,
      cache->node->addr,
      cache->iterators[ELEMENT_ITERATOR_INDEX],
      cache->vnm.vd,
      cache->vnm.md,
      cache->node->config.sublist->data[cache->iterators[SUB_ADDR_ITERATOR_INDEX]]);
    retval = arsp->result;
    handle = arsp->handle;
  }

  if (retval != bg_err_success) {
    if (retval == bg_err_out_of_memory) {
      oom_set(cache);
      return asr_oom;
    }
    FAIL_P(cache, retval);
    err_set_to_end(cache, retval, bgapi_em);
    return asr_bgapi;
  } else {
    ONCE_P(cache);
    WAIT_RESPONSE_SET(cache);
    cache->cc_handle = handle;
    timer_set(cache, 1);
  }

  return asr_suc;
}

bool addsub_guard(const config_cache_t *cache)
{
  return (cache->node->config.sublist && cache->node->config.sublist->len);
}

int addsub_entry(config_cache_t *cache, func_guard guard)
{
  if (guard && !guard(cache)) {
    LOGW("State[%s] Guard Not Passed\n", state_names[cache->state]);
    return asr_tonext;
  }

  return __addsub(cache, get_mng());
}

int addsub_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  uint32_t evtid;
  ASSERT(cache);
  ASSERT(evt);

  evtid = BGLIB_MSG_ID(evt->header);
  timer_set(cache, 0);
  switch (evtid) {
    case gecko_evt_mesh_config_client_model_sub_status_id:
    {
      WAIT_RESPONSE_CLEAR(cache);
      switch (evt->data.evt_mesh_config_client_model_sub_status.result) {
        case bg_err_success:
          RETRY_CLEAR(cache);
          SUC_P(cache);
          break;
        case bg_err_timeout:
          /* bind any remaining_retry case here */
          if (!EVER_RETRIED(cache)) {
            cache->remaining_retry = ADD_SUB_RETRY_TIMES;
            EVER_RETRIED_SET(cache);
          } else if (cache->remaining_retry <= 0) {
            RETRY_CLEAR(cache);
            RETRY_OUT_PRINT(cache);
            err_set_to_end(cache, bg_err_timeout, bgevent_em);
          }
          return asr_suc;
          break;
        case bg_err_mesh_foundation_insufficient_resources:
          LOGW("Node[0x%04x]: Cannot Sub More Address, Passing\n", cache->node->addr);
          cache->iterators[SUB_ADDR_ITERATOR_INDEX] = cache->node->config.sublist->len - 1;
          break;
        default:
          FAIL_P(cache,
                 evt->data.evt_mesh_config_client_model_sub_status.result);
          err_set_to_end(cache, bg_err_timeout, bgevent_em);
          return asr_suc;
      }

      if (iter_addsub(cache) == 1) {
        cache->next_state = -1;
        return asr_suc;
      }

      return __addsub(cache, get_mng());
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

int addsub_retry(config_cache_t *cache, int reason)
{
  int ret;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __addsub(cache, get_mng());

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

int addsub_exit(void *p)
{
  return asr_suc;
}

bool is_addsub_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}

static inline bool __sub_supported(uint16_t md)
{
  for (int i = 0; i < NSPT_SUB_MODEL_NUM(); i++) {
    if (not_sub_models[i] == md) {
      return false;
    }
  }
  return true;
}

static int iter_addsub(config_cache_t *cache)
{
  int md, increment = 0;

  if (++cache->iterators[SUB_ADDR_ITERATOR_INDEX] == cache->node->config.sublist->len) {
    cache->iterators[SUB_ADDR_ITERATOR_INDEX] = 0;
    do{
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
        if (__sub_supported(md)) {
          increment = 1;
          break;
        }
        LOGW("Model - 0x%04x doesn't support Sub, pass.\n",
             md);
      }
    }while(1);

    if (!increment) {
      if (++cache->iterators[ELEMENT_ITERATOR_INDEX] == cache->dcd.element_cnt) {
        cache->iterators[ELEMENT_ITERATOR_INDEX] = 0;
        return 1;
      }
    }
  }
  return 0;
}
