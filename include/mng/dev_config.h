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

typedef bool (*func_guard)(const config_cache_t *cache);
typedef int (*func_entry)(const config_cache_t *cache,
                          func_guard guard);
typedef int (*func_inprogress)(const struct gecko_cmd_packet *evt,
                               config_cache_t *cache);
typedef int (*func_retry)(void *p, int reason);
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

enum{
  asr_suc,
  asr_oom,
  asr_tonext,
};
#ifdef __cplusplus
}
#endif
#endif //DEV_CONFIG_H
