/*************************************************************************
    > File Name: as_addappkey.c
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
#define APP_KEY_ITERATOR_INDEX  0

#define ONCE_P(cache)                                                                     \
  do {                                                                                    \
    LOGD("Node[%x]:  --- Add App Key[%d (Ref ID)]\n",                                     \
         cache->node->addr,                                                               \
         get_mng()->cfg->subnets[0].appkey[cache->iterators[APP_KEY_ITERATOR_INDEX]].id); \
  } while (0)

#define SUC_P(cache)                                                                      \
  do {                                                                                    \
    LOGM("Node[%x]:  --- Add App Key[%d (Ref ID)] SUCCESS \n",                            \
         cache->node->addr,                                                               \
         get_mng()->cfg->subnets[0].appkey[cache->iterators[APP_KEY_ITERATOR_INDEX]].id); \
  } while (0)

#define FAIL_P(cache, err)                                                               \
  do {                                                                                   \
    LOGE("Node[%x]:  --- Add App Key[%d (Ref ID)] FAILED, Err <0x%04x>\n",               \
         cache->node->addr,                                                              \
         get_mng()->cfg->subnets[0].appkey[cache->iterators[APP_KEY_ITERATOR_INDEX]].id, \
         err);                                                                           \
  } while (0)

/* Global Variables *************************************************** */
extern const char *state_names[];

/* Static Variables *************************************************** */
static const uint32_t events[] = {
  gecko_evt_mesh_config_client_appkey_status_id
};
#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))

/* Static Functions Declaractions ************************************* */
static int iter_appkey(config_cache_t *cache);

static int __add_appkey(config_cache_t *cache, mng_t *mng)
{
  int ret;
  uint16_t key_id = 0;
  struct gecko_msg_mesh_config_client_add_appkey_rsp_t *rsp;

  ret = appkey_by_refid(
    mng,
    cache->node->config.bindings->data[cache->iterators[APP_KEY_ITERATOR_INDEX]],
    &key_id);
  ASSERT(ret == asr_suc);

  rsp = gecko_cmd_mesh_config_client_add_appkey(
    mng->cfg->subnets[0].netkey.id,
    cache->node->addr,
    key_id,
    mng->cfg->subnets[0].netkey.id);

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
    timer_set(cache, 1);
  }
  return asr_suc;
}

bool addappkey_guard(const config_cache_t *cache)
{
  return (cache->node->config.bindings && (cache->node->config.bindings->len != 0));
}

int addappkey_entry(config_cache_t *cache, func_guard guard)
{
  if (guard && !guard(cache)) {
    LOGM("To Next State Since %s Guard Not Passed\n",
         state_names[cache->state]);
    return asr_tonext;
  }
  return __add_appkey(cache, get_mng());
}

int addappkey_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  uint32_t evtid;

  ASSERT(cache);
  ASSERT(evt);

  evtid = BGLIB_MSG_ID(evt->header);
  timer_set(cache, 0);
  switch (evtid) {
    case gecko_evt_mesh_config_client_appkey_status_id:
    {
      WAIT_RESPONSE_CLEAR(cache);
      switch (evt->data.evt_mesh_config_client_appkey_status.result) {
        case bg_err_success:
          RETRY_CLEAR(cache);
          SUC_P(cache);
          break;
        case bg_err_timeout:
          /* add any remaining_retry case here */
          if (!EVER_RETRIED(cache)) {
            cache->remaining_retry = ADD_APP_KEY_RETRY_TIMES;
            EVER_RETRIED_SET(cache);
          } else if (cache->remaining_retry <= 0) {
            RETRY_CLEAR(cache);
            RETRY_OUT_PRINT(cache);
            err_set_to_end(cache, bg_err_timeout, bgevent_em);
            LOGD("Node[%x]: To <<st_end>> State\n", cache->node->addr);
          }
          return asr_suc;
          break;
        default:
          FAIL_P(cache,
                 evt->data.evt_mesh_config_client_appkey_status.result);
          err_set_to_end(cache, bg_err_timeout, bgevent_em);
          LOGW("To <<End>> State\n");
          return asr_suc;
      }

      if (iter_appkey(cache) == 1) {
        cache->next_state = -1;
        return asr_suc;
      }

      return __add_appkey(cache, get_mng());
    }
    break;

    default:
      LOGE("Unexpected event [0x%08x] happend in %s state.\n",
           evtid,
           state_names[cache->state]);
      return asr_unspec;
  }
  return 1;
}

int addappkey_retry(config_cache_t *cache, int reason)
{
  int ret = 0;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __add_appkey(cache, get_mng());
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

int addappkey_exit(void *p)
{
  return asr_suc;
}

bool is_addappkey_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}

static int iter_appkey(config_cache_t *cache)
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
    return 1;
  }
  return 0;
}

int appkey_by_refid(mng_t *mng,
                    uint16_t refid,
                    uint16_t *id)
{
  for (int i = 0; i < mng->cfg->subnets[0].active_appkey_num; i++) {
    if (refid != mng->cfg->subnets[0].appkey[i].refid) {
      continue;
    }
    if (id) {
      *id = mng->cfg->subnets[0].appkey[i].id;
    }
    return asr_suc;
  }
  return asr_notfnd;
}
