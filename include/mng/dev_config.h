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
#include "logging.h"

#define SIG_VENDOR_ID 0xffff

#define RETRY_MSG \
  "Node[%x]:  --- Retry %s, Remaining %d Times\n"
#define RETRY_OUT_MSG \
  "Node[%x]:  --- All Retries Failed\n"

#define OOM_MSG \
  "Node[%x]: Resend Last %s Command Due to Out Of Memory\n"
#define OOM_SET_MSG \
  "Node[%x]: Out Of Memory Set for Last %s\n"
#define EXPIRED_MSG \
  "Node[%x]: Resend Last %s Command Due to Command Expired\n"

#define RETRY_OUT_PRINT(x) LOGE(RETRY_OUT_MSG, (x)->node->addr);
#define RETRY_ONCE_PRINT(x) \
  LOGW(RETRY_MSG, (x)->node->addr, state_names[(x)->state], (x)->remaining_retry);
#define OOM_SET_PRINT(x) \
  LOGD(OOM_SET_MSG, (x)->node->addr, state_names[(x)->state]);
#define OOM_ONCE_PRINT(x) \
  LOGD(OOM_MSG, (x)->node->addr, state_names[(x)->state]);
#define EXPIRED_ONCE_PRINT(x) \
  LOGD(EXPIRED_MSG, (x)->node->addr, state_names[(x)->state]);

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
  RELAY_BITOFS,
  PROXY_BITOFS,
  FRIEND_BITOFS,
  LPN_BITOFS, /* Informative, not affect anything */
  NETTX_BITOFS,
  TTL_BITOFS,
  SNB_BITOFS,
  FEATURE_MAX_BITOFS
}features_em;

typedef enum {
  to_next_state_em = -1,
  /* Adding devices state(s) */
  provisioning_em = 0,
  provisioned_em,
  /* Configuring devices state(s) */
  get_dcd_em,
  addappkey_em,
  bindappkey_em,
  setpub_em,
  addsub_em,
  setconfig_em,
  end_em,
  /* Removing devices state(s) */
  rm_em,
  rmend_em
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
  asr_notfnd,
};

typedef enum {
  bgapi_em,
  bgevent_em,
  bg_err_invalid_em
} bgerr_type_t;

static inline void err_set(config_cache_t *cache,
                           uint16_t errcode,
                           int err_type)
{
  if (err_type == bgapi_em) {
    BIT_SET(cache->err_cache.bgcall, cache->state);
  } else if (err_type == bgevent_em) {
    BIT_SET(cache->err_cache.bgevt, cache->state);
  } else {
    return;
  }
  cache->err_cache.general = errcode;
}

static inline void clr_cache_ctl(config_cache_t *cache)
{
  cache->expired = 0;
  cache->flags = 0;
}

static inline void err_set_to_end(config_cache_t *cache,
                                  uint16_t errcode,
                                  int err_type)
{
  err_set(cache, errcode, err_type);
  clr_cache_ctl(cache);
  cache->next_state = end_em;
}

static inline void err_set_to_rm_end(config_cache_t *cache,
                                     uint16_t errcode,
                                     int err_type)
{
  err_set(cache, errcode, err_type);
  clr_cache_ctl(cache);
  cache->next_state = rmend_em;
}

void timer_set(config_cache_t *cache, bool enable);
extern const char *state_names[];
static inline void oom_set(config_cache_t *cache)
{
  BIT_SET(cache->flags, OOM_BIT_OFFSET);
  LOGW(OOM_SET_MSG, cache->node->addr, state_names[cache->state]);
}

int dev_config_hdr(const struct gecko_cmd_packet *e);
bool acc_loop(void *p);
void acc_init(bool use_default);
/******************************************************************
 * State functions
 * ***************************************************************/
int appkey_by_refid(mng_t *mng,
                    uint16_t refid,
                    uint16_t *id);
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
 * Add Appkey State
 */
bool addappkey_guard(const config_cache_t *cache);
int addappkey_entry(config_cache_t *cache, func_guard guard);
int addappkey_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int addappkey_retry(config_cache_t *cache, int reason);
int addappkey_exit(void *p);
bool is_addappkey_pkts(uint32_t evtid);

/*
 * Bind Appkey State
 */
bool bindappkey_guard(const config_cache_t *cache);
int bindappkey_entry(config_cache_t *cache, func_guard guard);
int bindappkey_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int bindappkey_retry(config_cache_t *cache, int reason);
int bindappkey_exit(void *p);
bool is_bindappkey_pkts(uint32_t evtid);

/*
 * Set Pub State
 */
bool setpub_guard(const config_cache_t *cache);
int setpub_entry(config_cache_t *cache, func_guard guard);
int setpub_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int setpub_retry(config_cache_t *cache, int reason);
int setpub_exit(void *p);
bool is_setpub_pkts(uint32_t evtid);

/*
 * Add Sub State
 */
bool addsub_guard(const config_cache_t *cache);
int addsub_entry(config_cache_t *cache, func_guard guard);
int addsub_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int addsub_retry(config_cache_t *cache, int reason);
int addsub_exit(void *p);
bool is_addsub_pkts(uint32_t evtid);

/*
 * Set Config State
 */
bool setconfig_guard(const config_cache_t *cache);
int setconfig_entry(config_cache_t *cache, func_guard guard);
int setconfig_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int setconfig_retry(config_cache_t *cache, int reason);
int setconfig_exit(void *p);
bool is_setconfig_pkts(uint32_t evtid);

/*
 * End State
 */
int end_entry(config_cache_t *cache, func_guard guard);

/*
 * RM State
 */
bool rm_guard(const config_cache_t *cache);
int rm_entry(config_cache_t *cache, func_guard guard);
int rm_inprg(const struct gecko_cmd_packet *evt, config_cache_t *cache);
int rm_retry(config_cache_t *cache, int reason);
int rm_exit(void *p);
bool is_rm_pkts(uint32_t evtid);

/*
 * RM End State
 */
int rmend_entry(config_cache_t *cache, func_guard guard);
#ifdef __cplusplus
}
#endif
#endif //DEV_CONFIG_H
