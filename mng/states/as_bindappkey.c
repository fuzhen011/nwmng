/*************************************************************************
    > File Name: as_bindappkey.c
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
#define APP_KEY_ITERATOR_INDEX  2

#define ONCE_P(cache)                                                                    \
  do {                                                                                   \
    LOGD("Node[%x]:  --- Bind [refid(%d) <-> %s Model(%04x:%04x)]\n",                    \
         cache->node->addr,                                                              \
         get_mng()->cfg->subnets[0].appkey[cache->iterators[APP_KEY_ITERATOR_INDEX]].id, \
         cache->vnm.vd == SIG_VENDOR_ID ? "SIG" : "Vendor",                              \
         cache->vnm.vd,                                                                  \
         cache->vnm.md);                                                                 \
  } while (0)

#define SUC_P(cache, config)                                                             \
  do {                                                                                   \
    LOGM("Node[%x]:  --- Bind [refid(%d) <-> %s Model(%04x:%04x)] SUCCESS\n",            \
         cache->node->addr,                                                              \
         get_mng()->cfg->subnets[0].appkey[cache->iterators[APP_KEY_ITERATOR_INDEX]].id, \
         cache->vnm.vd == SIG_VENDOR_ID ? "SIG" : "Vendor",                              \
         cache->vnm.vd,                                                                  \
         cache->vnm.md);                                                                 \
  } while (0)

#define FAIL_P(cache, config, err)                                                         \
  do {                                                                                     \
    LOGE("Node[%x]:  --- Bind [refid(%d) <-> %s Model(%04x:%04x)] FAILED, Err <0x%04x>\n", \
         cache->node->addr,                                                                \
         get_mng()->cfg->subnets[0].appkey[cache->iterators[APP_KEY_ITERATOR_INDEX]].id,   \
         cache->vnm.vd == SIG_VENDOR_ID ? "SIG" : "Vendor",                                \
         cache->vnm.vd,                                                                    \
         cache->vnm.md,                                                                    \
         err);                                                                             \
  } while (0)

/* Global Variables *************************************************** */
extern const char *state_names[];

/* Static Variables *************************************************** */
static const uint32_t events[] = {
  gecko_evt_mesh_config_client_binding_status_id
};
#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))

/* Static Functions Declaractions ************************************* */
static int iter_bindings(config_cache_t *cache);

static int __bind_appkey(config_cache_t *cache, mng_t *mng)
{
  int ret;
  uint16_t key_id = 0;
  struct gecko_msg_mesh_config_client_bind_model_rsp_t *rsp;

  cache->vnm.vd =
    (cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt)
    ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].vid
    : 0xFFFF;
  cache->vnm.md =
    (cache->iterators[MODEL_ITERATOR_INDEX] >= cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt)
    ? cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm[cache->iterators[MODEL_ITERATOR_INDEX] - cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt].mid
    : cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sig_models[cache->iterators[MODEL_ITERATOR_INDEX]];

  ret = appkey_by_refid(
    mng,
    cache->node->config.bindings->data[cache->iterators[APP_KEY_ITERATOR_INDEX]],
    &key_id);
  ASSERT(asr_suc == ret);

  rsp = gecko_cmd_mesh_config_client_bind_model(
    mng->cfg->subnets[0].netkey.id,
    cache->node->addr,
    cache->iterators[ELEMENT_ITERATOR_INDEX],
    key_id,
    cache->vnm.vd,
    cache->vnm.md);

  if (rsp->result != bg_err_success) {
    if (rsp->result == bg_err_out_of_memory) {
      oom_set(cache);
      return asr_oom;
    }
    FAIL_P(cache, pconfig, rsp->result);
    err_set_to_end(cache, rsp->result, bgapi_em);
    LOGE("Node[%x]: To <<End>> State\n", cache->node->addr);
    return asr_bgapi;
  } else {
    ONCE_P(cache);
    WAIT_RESPONSE_SET(cache);
    cache->cc_handle = rsp->handle;
    timer_set(cache, 1);
  }
  return asr_suc;
}

bool bindappkey_guard(const config_cache_t *cache)
{
  return (cache->node->config.bindings && (cache->node->config.bindings->len != 0));
}

int bindappkey_entry(config_cache_t *cache, func_guard guard)
{
  if (guard && !guard(cache)) {
    LOGM("To Next State Since %s Guard Not Passed\n",
         state_names[cache->state]);
    return asr_tonext;
  }
  return __bind_appkey(cache, get_mng());
}

int bindappkey_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  uint32_t evtid;

  ASSERT(cache);
  ASSERT(evt);

  evtid = BGLIB_MSG_ID(evt->header);
  timer_set(cache, 0);
  switch (evtid) {
    case gecko_evt_mesh_config_client_binding_status_id:
    {
      WAIT_RESPONSE_CLEAR(cache);
      switch (evt->data.evt_mesh_config_client_binding_status.result) {
        case bg_err_success:
          RETRY_CLEAR(cache);
          SUC_P(cache, pconfig);
          break;
        case bg_err_timeout:
          /* bind any remaining_retry case here */
          if (!EVER_RETRIED(cache)) {
            cache->remaining_retry = BIND_APP_KEY_RETRY_TIMES;
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
                 pconfig,
                 evt->data.evt_mesh_config_client_binding_status.result);
          err_set_to_end(cache, bg_err_timeout, bgevent_em);
          LOGW("To <<End>> State\n");
          return asr_suc;
      }

      if (iter_bindings(cache) == 1) {
        cache->next_state = -1;
        return asr_suc;
      }

      return __bind_appkey(cache, get_mng());
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

int bindappkey_retry(config_cache_t *cache, int reason)
{
  int ret;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __bind_appkey(cache, get_mng());

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

int bindappkey_exit(void *p)
{
  return asr_suc;
}

bool is_bindappkey_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}

static int iter_bindings(config_cache_t *cache)
{
  while (++cache->iterators[APP_KEY_ITERATOR_INDEX] != cache->node->config.bindings->len) {
    if (asr_suc == appkey_by_refid(
          get_mng(),
          cache->node->config.bindings->data[cache->iterators[APP_KEY_ITERATOR_INDEX]],
          NULL)) {
      break;
    }
  }

  if (cache->iterators[APP_KEY_ITERATOR_INDEX] == cache->node->config.bindings->len) {
    cache->iterators[APP_KEY_ITERATOR_INDEX] = 0;
    if (++cache->iterators[MODEL_ITERATOR_INDEX]
        == cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].sigm_cnt
        + cache->dcd.elems[cache->iterators[ELEMENT_ITERATOR_INDEX]].vm_cnt) {
      cache->iterators[MODEL_ITERATOR_INDEX] = 0;
      if (++cache->iterators[ELEMENT_ITERATOR_INDEX] == cache->dcd.element_cnt) {
        cache->iterators[ELEMENT_ITERATOR_INDEX] = 0;
        /* All bind appkey is done */
        return 1;
      }
    }
  }
  return 0;
}
