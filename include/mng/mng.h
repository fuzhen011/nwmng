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
#include "host_gecko.h"
#include "cfg.h"

typedef struct {
  bool busy;
  time_t expired;
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
#define WAIT_RESPONSE_CLEAR(x)  BIT_CLR((x)->flags, WAITING_RESPONSE_BIT_OFFSET)
#define WAIT_RESPONSE_SET(x)  BIT_SET((x)->flags, WAITING_RESPONSE_BIT_OFFSET)

#define EVER_RETRIED(x) IS_BIT_SET((x)->flags, EVER_RETRIED_BIT_OFFSET)
#define EVER_RETRIED_SET(x) BIT_SET((x)->flags, EVER_RETRIED_BIT_OFFSET)
#define EVER_RETRIED_CLEAR(x) BIT_CLR((x)->flags, EVER_RETRIED_BIT_OFFSET)

#define OOM(x) IS_BIT_SET((x)->flags, OOM_BIT_OFFSET)

#define OOM_CLEAR(x)                     \
  do {                                   \
    BIT_CLR((x)->flags, OOM_BIT_OFFSET); \
  } while (0)

#define RETRY_CLEAR(x)                            \
  do {                                            \
    BIT_CLR((x)->flags, EVER_RETRIED_BIT_OFFSET); \
    (x)->remaining_retry = 0;                     \
  } while (0)
/*
 * TODO: Need to clear above?
 */
typedef struct {
  uint16_t vid;
  uint16_t mid;
}vendor_model_t;

typedef struct {
  uint8_t sigm_cnt;
  uint8_t vm_cnt;
  uint16_t *sig_models;
  vendor_model_t *vm;
}elem_t;

typedef struct {
  uint16_t feature;
  uint8_t element_cnt;
  elem_t *elems;
}dcd_t;

#define ITERATOR_NUM  3
typedef struct {
  int state;
  int next_state;
  /* NULL if not in use */
  node_t *node;
  /* POSIX timer is not supported by macOS, so use the less efficient way to
   * poll it in every loop */
  time_t expired;
  bbitmap_t flags;
  uint8_t remaining_retry;
  struct {
    uint32_t bgcall;
    uint32_t bgevt;
    uint16_t general;
  }err_cache;
  dcd_t dcd;
  uint32_t cc_handle; /* Config Client Handle returned by bgcall */
  struct {
    uint16_t vd;
    uint16_t md;
  }vnm;
  int iterators[ITERATOR_NUM];
}config_cache_t;

enum {
  bl_idle,
  bl_prepare,
  bl_starting,
  bl_busy,
  bl_done
};

typedef struct {
  node_t *n;
  uint8_t phase;
}remainig_nodes_t;

typedef struct {
  int state; /* @ref{bl_xxx} */
  int offset;
  int tail;
  struct {
    int num;
    remainig_nodes_t *nodes;
  }rem;
}bl_cache_t;

typedef enum {
  nil,
  initialized,
  configured,
  starting,
  adding_devices_em,
  configuring_devices_em,
  removing_devices_em,
  blacklisting_devices_em,
  stopping,
  state_reload
}mng_state_t;

typedef struct {
  int offs;
  char prios[4];  /* actually 3 bytes are used, one more for print ending */
}seqprio_t;

enum {
  fm_idle,
  fm_scanning,
  fm_freemode
};

typedef struct {
  mng_state_t state;
  uint8_t conn;
  provcfg_t *cfg;

  struct {
    int free_mode;
    seqprio_t seq;
    bool oom;
    time_t oom_expired;
  }status;

  struct {
    add_cache_t add[MAX_PROV_SESSIONS];
    struct {
      lbitmap_t used;
      config_cache_t cache[MAX_CONCURRENT_CONFIG_NODES];
    }config;
    bl_cache_t bl;
    struct {
      uint8_t type;
      uint8_t value;
      GList *nodes;
    }model_set;
  }cache;

  struct {
    GList *add;
    GList *config;
    GList *bl;
    GList *rm;
    GList *fail;
  }lists;
}mng_t;

err_t mng_init(void *p);
err_t init_ncp(void *p);
err_t clr_all(void *p);

void *mng_mainloop(void *p);
mng_t *get_mng(void);

err_t ipc_get_provcfg(void *p);

void cmd_enq(const char *str, int offs);
wordexp_t *cmd_deq(int *offs);

int dev_add_hdr(const struct gecko_cmd_packet *evt);
int bl_hdr(const struct gecko_cmd_packet *e);

void mng_load_lists(void);
void on_lists_changed(void);

#define DECLARE_CB(name)  err_t clicb_##name(int argc, char *argv[])

DECLARE_CB(freemode);

DECLARE_CB(sync);
DECLARE_CB(list);
DECLARE_CB(info);
DECLARE_CB(rmall);
DECLARE_CB(rmblclr);
DECLARE_CB(seqset);
DECLARE_CB(onoff);
DECLARE_CB(lightness);
DECLARE_CB(ct);
DECLARE_CB(status);
DECLARE_CB(loglvlset);
#ifdef DEMO_EN
DECLARE_CB(demo);
#endif

bool bl_loop(void *p);
bool add_loop(void *p);

bool models_loop(mng_t *mng);
uint16_t send_onoff(uint16_t addr, uint8_t onoff);
uint16_t send_lightness(uint16_t addr, uint8_t lightness);
uint16_t send_ctl(uint16_t addr, uint8_t ctl);

void demo_run(void);
void demo_start(int en);
#ifdef __cplusplus
}
#endif
#endif //MNG_H
