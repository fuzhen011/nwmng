/*************************************************************************
    > File Name: mng.h
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

#ifndef MNG_H
#define MNG_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <wordexp.h>
#include <stdbool.h>
#include "err.h"
#include "projconfig.h"
#include "cfgdb.h"

typedef struct {
  bool busy;
  uint8_t uuid[16];
}add_cache_t;
/*
 * TODO: Need to clear below?
 */
#define EVER_RETRIED_BIT_OFFSET 7
#define WAITING_RESPONSE_BIT_OFFSET 6
#define OOM_BIT_OFFSET  5

#define WAITING_RESPONSE_BIT_MASK  (1 << WAITING_RESPONSE_BIT_OFFSET)
#define EVER_RETRIED_BIT_MASK  (1 << EVER_RETRIED_BIT_OFFSET)
#define OOM_BIT_MASK  (1 << OOM_BIT_OFFSET)
#define GUARD_TIMER_EXPIRED_BIT_MASK  (1 << GUARD_TIMER_EXPIRED_OFFSET)

#define WAIT_RESPONSE(x)  IS_BIT_SET((x)->flags, WAITING_RESPONSE_BIT_OFFSET)
#define WAIT_RESPONSE_CLEAR(x)  BIT_CLEAR((x)->flags, WAITING_RESPONSE_BIT_OFFSET)
#define WAIT_RESPONSE_SET(x)  BIT_SET((x)->flags, WAITING_RESPONSE_BIT_OFFSET)

#define EVER_RETRIED(x) IS_BIT_SET((x)->flags, EVER_RETRIED_BIT_OFFSET)
#define EVER_RETRIED_SET(x) BIT_SET((x)->flags, EVER_RETRIED_BIT_OFFSET)
#define EVER_RETRIED_CLEAR(x) BIT_CLEAR((x)->flags, EVER_RETRIED_BIT_OFFSET)

#define OOM(x) IS_BIT_SET((x)->flags, OOM_BIT_OFFSET)
#define OOM_SET(x)                       \
  do {                                   \
    OOM_SET_PRINT(x);                    \
    BIT_SET((x)->flags, OOM_BIT_OFFSET); \
  } while (0)
#define OOM_CLEAR(x) BIT_CLEAR((x)->flags, OOM_BIT_OFFSET)

#define RETRY_CLEAR(x)                              \
  do {                                              \
    BIT_CLEAR((x)->flags, EVER_RETRIED_BIT_OFFSET); \
    (x)->retry = 0;                                 \
  } while (0)
/*
 * TODO: Need to clear above?
 */
typedef struct {
  uint8_t feature;
  uint8_t element_cnt;
  struct {
    uint8_t sigm_cnt;
    uint8_t vm_cnt;
    uint16_t *sig_models;
    struct {
      uint16_t vid;
      uint16_t mid;
    }*vendor_model;
  }e1;
}dcd_t;

#define ITERATOR_NUM  3
typedef struct {
  int state;
  int next_state;
  uint16_t addr;
  bbitmap_t flag;
  uint8_t remaining_retry;
  struct {
    uint32_t bgcall;
    uint32_t bgevt;
    uint16_t general;
  }err_cache;
  mesh_config_t config;
  dcd_t dcd;
  uint32_t cc_handle; /* Config Client Handle returned by bgcall */
  struct {
    uint16_t vd;
    uint16_t md;
  }vnm;
  int iterators[ITERATOR_NUM];
}config_cache_t;

enum {
  nil,
  syncup,
  initialized,
  configured,
  starting,
  adding_devices_em,
  configuring_devices_em,
  removing_devices_em,
  blacklisting_devices_em,
  stopping,
  state_reload
};

enum {
  fm_idle,
  fm_scanning,
  fm_freemode
};

typedef struct {
  int state;
  uint8_t conn;
  provcfg_t cfg;
  struct {
    int free_mode;
  }status;
  struct {
    add_cache_t add[MAX_PROV_SESSIONS];
    config_cache_t config[MAX_CONCURRENT_CONFIG_NODES];
  }cache;
}mng_t;

err_t init_ncp(void *p);
err_t clr_all(void *p);

void *mng_mainloop(void *p);
mng_t *get_mng(void);

err_t ipc_get_provcfg(void *p);

err_t clm_set_scan(int onoff);

void cmd_enq(const char *str, int offs);
wordexp_t *cmd_deq(int *offs);
#ifdef __cplusplus
}
#endif
#endif //MNG_H
