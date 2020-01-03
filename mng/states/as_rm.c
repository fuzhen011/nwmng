/*************************************************************************
    > File Name: as_rm.c
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
#define REMOVE_NODE_MSG \
  "Node[%x]:  --- RM\n"
#define REMOVE_NODasr_suc_MSG \
  "Node[%x]:  --- SUCCESS\n"
#define REMOVE_NODE_FAIL_MSG \
  "Node[%x]:  --- RM FAILED, Err <0x%04x>\n"

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
#define ONCE_P(cache)     \
  do {                    \
    LOGD(                 \
      REMOVE_NODE_MSG,    \
      cache->node->addr); \
  } while (0)

#define SUC_P(cache)         \
  do {                       \
    LOGD(                    \
      REMOVE_NODasr_suc_MSG, \
      cache->node->addr);    \
  } while (0)

#define FAIL_P(cache, err)  \
  do {                      \
    LOGE(                   \
      REMOVE_NODE_FAIL_MSG, \
      cache->node->addr,    \
      err);                 \
  } while (0)

/* Global Variables *************************************************** */
extern const char *stateNames[];

static const uint32_t events[] = {
  gecko_evt_mesh_config_client_reset_status_id
};

#define RELATE_EVENTS_NUM() (sizeof(events) / sizeof(uint32_t))
/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

static int __rm(config_cache_t *cache, mng_t *mng)
{
  struct gecko_msg_mesh_config_client_reset_node_rsp_t *rsp;

  /* First one, should set */
  rsp = gecko_cmd_mesh_config_client_reset_node(
    mng->cfg->subnets[0].netkey.id,
    cache->node->addr);

  if (rsp->result != bg_err_success) {
    if (rsp->result == bg_err_out_of_memory) {
      OOM_SET(cache);
      return asr_oom;
    }
    FAIL_P(cache, rsp->result);
    err_set_to_rm_end(cache, rsp->result, bgapi_em);
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

bool rm_guard(const config_cache_t *cache)
{
  return cache->node->addr != UNASSIGNED_ADDRESS;
}

int rm_entry(config_cache_t *cache, func_guard guard)
{
  if (guard && !guard(cache)) {
    LOGM("To Next State Since %s Guard Not Passed\n",
         stateNames[cache->state]);
    return asr_tonext;
  }

  return __rm(cache, get_mng());
}

int rm_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache)
{
  uint32_t evtid;
  ASSERT(cache);
  ASSERT(evt);

  evtid = BGLIB_MSG_ID(evt->header);
  /* TODO: startTimer(cache, 0); */
  switch (evtid) {
    case gecko_evt_mesh_config_client_reset_status_id:
      WAIT_RESPONSE_CLEAR(cache);
      switch (evt->data.evt_mesh_config_client_reset_status.result) {
        case bg_err_success:
          RETRY_CLEAR(cache);
          SUC_P(cache);
          cache->next_state = -1;
          break;
        case bg_err_timeout:
          /* bind any remaining_retry case here */
          if (!EVER_RETRIED(cache)) {
            cache->remaining_retry = REMOVE_NODE_RETRY_TIMES;
            EVER_RETRIED_SET(cache);
          } else if (cache->remaining_retry <= 0) {
#if (BETTER_SOLUTION == 1)
            RETRY_CLEAR(cache);
            RETRY_OUT_PRINT(cache);
            err_set_to_rm_end(cache, bg_err_timeout, bgevent_em);
            LOGD("Node[%d]: To <<st_end>> State\n", cache->node->addr);
#else
            RETRY_CLEAR(cache);
            RETRY_OUT_PRINT(cache);
            cache->next_state = -1;
#endif
          }
          break;
        default:
          FAIL_P(cache,
                 evt->data.evt_mesh_config_client_reset_status.result);
          err_set_to_rm_end(cache, bg_err_timeout, bgevent_em);
          LOGW("To <<End>> State\n");
          break;
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

int rm_retry(config_cache_t *cache, int reason)
{
  int ret;
  ASSERT(cache);
  ASSERT(reason < retry_on_max_em);

  ret = __rm(cache, get_mng());

  if (ret == asr_suc) {
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

int rm_exit(void *p)
{
  return asr_suc;
}

int is_rm_pkts(uint32_t evtid)
{
  int i;
  for (i = 0; i < RELATE_EVENTS_NUM(); i++) {
    if (BGLIB_MSG_ID(evtid) == events[i]) {
      return 1;
    }
  }
  return 0;
}
