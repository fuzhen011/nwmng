/*************************************************************************
    > File Name: dev_config.h
    > Author: Kevin
    > Created Time: 2020-01-02
    > Description:
 ************************************************************************/

#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
#include <stdint.h>

#include "mng.h"
#include "gecko_bglib.h"

#define RETRY_MSG \
  "Node[%x]: --- Retry %s on TIMEOUT, remaining %d times before failure\n"
#define RETRY_OUT_MSG \
  "Node[%x]: All Retry Done BUT Still FAILED\n"
#define RETRY_TO_END_MSG \
  "Got continueous failure, device present?\n"
#define OOM_MSG \
  "Node[%x]: Resend Last %s Command Due to Out Of Memory\n"
#define OOM_SET_MSG \
  "Node[%x]: Out Of Memory Set for Last %s\n"
#define EXPIRED_MSG \
  "Node[%x]: Resend Last %s Command Due to Command Expired\n"

#define RETRY_OUT_PRINT(x) LOGE(RETRY_OUT_MSG, (x)->node->addr);
#define RETRY_ONCE_PRINT(x) \
  LOGD(RETRY_MSG, (x)->node->addr, stateNames[(x)->state], (x)->remaining_retry);
#define OOM_SET_PRINT(x) \
  LOGD(OOM_SET_MSG, (x)->node->addr, stateNames[(x)->state]);
#define OOM_ONCE_PRINT(x) \
  LOGD(OOM_MSG, (x)->node->addr, stateNames[(x)->state]);
#define EXPIRED_ONCE_PRINT(x) \
  LOGD(EXPIRED_MSG, (x)->node->addr, stateNames[(x)->state]);

#define UNASSIGNED_ADDRESS  0

#define ERROR_BIT(x)        BITOF(x)
#define SET_ERROR_BIT(x, b) ((x) |= ERROR_BIT((b)))
#define SET_USER_ERR_BIT(x) ((x) |= 0x80000000)

typedef bool (*func_guard)(const config_cache_t *cache);
typedef int (*func_entry)(config_cache_t *cache,
                          func_guard guard);
typedef int (*func_inprogress)(const struct gecko_cmd_packet *evt,
                               config_cache_t *cache);
typedef int (*func_retry)(config_cache_t *cache, int reason);
typedef int (*func_exit)(void *p);
typedef bool (*func_delegate)(uint32_t evtid);

typedef enum {
  to_next_state_em = -1,
  /* Adding devices state(s) */
  provisioning_em = 0,
  provisioned_em,
  /* Configuring devices state(s) */
  get_dcd_em,
  add_appkey_em,
  bind_appKey_em,
  setpub_em,
  addsub_em,
  set_config_em,
  end_em,
  /* Removing devices state(s) */
  reset_node_em,
  reset_node_end_em
}acc_state_emt;

typedef enum {
  on_timeout_em,
  on_oom_em,
  on_guard_timer_expired_em,
  retry_on_max_em
} retry_reason_t;

typedef struct acc_state{
  acc_state_emt state;
  func_guard guard;
  func_entry entry;
  func_inprogress inpg;
  func_retry retry;
  func_exit exit;
  func_delegate dlg;
  struct acc_state *next;
}acc_state_t;

typedef struct {
  bool started;
  int state_num;
  acc_state_t *states;
}acc_t;

enum {
  asr_suc,
  asr_oom,
  asr_tonext,
  asr_bgapi,
  asr_unspec,
};

typedef enum {
  bgapi_em,
  bgevent_em,
  bg_err_invalid_em
} bgerr_type_t;

static inline void err_set(config_cache_t *cache,
                           uint16_t errcode,
                           int which)
{
  if (which == bgapi_em) {
    BIT_SET(cache->err_cache.bgcall, cache->state);
  } else if (which == bgevent_em) {
    BIT_SET(cache->err_cache.bgevt, cache->state);
  } else {
    return;
  }
  cache->err_cache.general = errcode;
}

static inline void err_set_to_end(config_cache_t *cache,
                                  uint16_t errcode,
                                  int which)
{
  err_set(cache, errcode, which);
  cache->next_state = end_em;
}

/******************************************************************
 * State functions
 * ***************************************************************/
/*
 * Get DCD State
 */
bool getdcd_guard(const config_cache_t *cache);
int getdcd_entry(config_cache_t *cache, func_guard guard);
int getdcd_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int getdcd_retry(config_cache_t *cache, int reason);
int getdcd_exit(void *p);
bool is_getdcd_pkts(uint32_t evtid);

/*
 * End State
 */
int end_entry(config_cache_t *cache, func_guard guard);
#ifdef __cplusplus
}
#endif
#endif //DEV_CONFIG_H
