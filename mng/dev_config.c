/*************************************************************************
    > File Name: dev_config.c
    > Author: Kevin
    > Created Time: 2020-01-02
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "mng.h"

#include "dev_config.h"
#include "logging.h"
#include "cli.h"
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static acc_t acc = { 0 };

static acc_state_t as_get_dcd = {
  get_dcd_em,
#if 0
  getDcdEntryGuard,
  getDcdStateEntry,
  getDcdStateInprogress,
  getDcdStateRetry,
  getDcdStateExit,
  isGetDcdRelatedPacket,
  NULL
#endif
};

static acc_state_t as_end = {
  end_em,
#if 0
  NULL,
  endStateEntry,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
#endif
};

const char *stateNames[] = {
  "Provisioning",
  "Provisioned",
  "Getting DCD",
  "Adding App Key(s)",
  "Binding App Key(s)",
  "Set Model Publication Address",
  "Add Model Subscription Address",
  "Set TTL/Proxy/Friend/Relay/Nettx",
  "Configuration End",
  "Remove Node",
  "Remove Node End"
};
/* Static Functions Declaractions ************************************* */
static int config_engine(void);
static inline void __cache_reset(config_cache_t *c,
                                 node_t *n /* TODO: remove it if not necessary */
                                 )
{
  memset(c, 0, sizeof(config_cache_t));
  c->state = provisioned_em;
  c->next_state = get_dcd_em;
  /* c->nodeIndex = INVALID_NODE_INDEX; */
  /* c->selfId = index; */
  c->node = n;
}

err_t add_state_after(const acc_state_t *ps, acc_state_emt state)
{
  bool found = false;
  acc_state_t *p = acc.states;
  acc_state_t *pi, *pn;
  /* Add to the head */
  if (state == -1) {
    pi = (acc_state_t *)calloc(1, sizeof(acc_state_t));
    memcpy(pi, ps, sizeof(acc_state_t));
    pi->next = acc.states;
    acc.states = pi;
    acc.state_num++;
    return ec_success;
  }

  while (p) {
    if (p->state == state) {
      found = true;
      break;
    }
    p = p->next;
  }

  if (!found) {
    return err(ec_not_exist);
  }

  pi = (acc_state_t *)calloc(1, sizeof(acc_state_t));
  memcpy(pi, ps, sizeof(acc_state_t));
  pn = p->next;
  p->next = pi;
  pi->next = pn;
  acc.state_num++;

  return ec_success;
}

static inline void __add_default_states(void)
{
  /* add_state_after(&addAppKeyIns, getDcd_em); */
  /* add_state_after(&bindAppKeyIns, addAppKey_em); */
  /* add_state_after(&setPubIns, bindAppKey_em); */
  /* add_state_after(&addSubIns, setPub_em); */
  /* add_state_after(&setConfigsIns, addSub_em); */
  /* add_state_after(&removeNodeIns, end_em); */
}

static void __acc_reset(bool use_default)
{
  mng_t *mng = get_mng();

  if (mng->lists.fail) {
    g_list_free(mng->lists.fail);
    mng->lists.fail = NULL;
  }

  for (int i = 0; i < MAX_CONCURRENT_CONFIG_NODES; i++) {
    __cache_reset(&mng->cache.config[i], NULL);
  }

  acc.started = 0;
  acc.state_num = 0;

  acc_state_t *p = acc.states, *pn;
  while (p) {
    pn = p->next;
    free(p);
    p = pn;
  }
  acc.states = NULL;

  add_state_after(&as_get_dcd, -1);
  add_state_after(&as_end, get_dcd_em);
  /* add_state_after(&removeendins, end_em); */
  if (use_default) {
    __add_default_states();
  }
}

void acc_init(bool use_default)
{
  if (!use_default) {
    TODOASSERT();
    return;
  }
  __acc_reset(use_default);

  /* TODO: sigInit(); and timerInit(); */
  acc.started = true;
}

bool acc_loop(void *p)
{
  if (!acc.started) {
    return false;
  }
  return config_engine();
}

static acc_state_t *as_get(int state)
{
  acc_state_t *p = acc.states;
  while (p) {
    if (p->state == state) {
      break;
    }
    p = p->next;
  }
  return p;
}

int to_next_state(config_cache_t *cache)
{
  int ret, transitioned = 0;
  acc_state_t *as = NULL, *nas = NULL;
  /* If current state exit callback exist, exist first */
  as = as_get(cache->state);
  if (cache->next_state == -1) {
    nas = as->next;
  } else {
    nas = as_get(cache->next_state);
  }

  LOGD("Node[%x]: Exiting from %s state\n",
       cache->node->addr,
       stateNames[cache->state]);
  if (as && as->exit) {
    ret = as->exit(cache);
    LOGD("Node[%x]: Exiting CB called.\n", cache->node->addr);
  }

  if (!nas) {
    /* If specified next state doesn't exist, load the next of the current */
    /* state */
    nas = as->next;
  }

  /* Find next state with valid entry */
  while (nas && !nas->entry) {
    nas = nas->next;
  }

  while (nas) {
    LOGD("Node[%x]: Try to enter %s state\n",
         cache->node->addr,
         stateNames[nas->state]);
    ret = nas->entry(cache, nas->guard);
    switch (ret) {
      case asr_suc:
      case asr_oom:
        cache->state = nas->state;
        cache->next_state = nas->state;
        transitioned = 1;
        LOGD("Node[%x]: Enter %s state Success\n",
             cache->node->addr,
             stateNames[nas->state]);
        break;
      /* Implementation of the callback should make sure that won't return this
       * if not more states to load */
      case asr_tonext:
        nas = nas->next;
        break;
      default:
        nas = as_get(end_em);
        break;
    }
    if (transitioned) {
      break;
    }
  }

  return transitioned;
}

static int config_engine(void)
{
  int i, busy = 0;
  int ret = 0;
  acc_state_t *as = NULL;
  mng_t *mng = get_mng();
  config_cache_t *pc = NULL;

  /*
   * Check if any **Exception** (OOM | Guard timer expired) happened in last round
   */
  for (int i = 0; i < MAX_CONCURRENT_CONFIG_NODES; i++) {
    if (!mng->cache.config[i].node) {
      continue;
    }
    as = as_get(mng->cache.config[i].state);
    if (mng->cache.config[i].expired && as->retry) {
      ret = as->retry(&mng->cache.config[i], on_guard_timer_expired_em);
      if (ret != asr_suc) {
        LOGE("Expired Retry Return %d\n", ret);
      }
    } else if (OOM(&mng->cache.config[i]) && as->retry) {
      ret = as->retry(&mng->cache.config[i], on_oom_em);
      LOGD("Node[0x%x]: OOM Recovery Once.\n", mng->cache.config[i].node->addr);
      if (ret != asr_suc && ret != asr_oom) {
        LOGE("OOM Retry Return %d\n", ret);
      }
    }
  }

  /* 4. Check if any state needs to update */
  for (i = 0; i < MAX_CONCURRENT_CONFIG_NODES; i++) {
    pc = &mng->cache.config[i];
    if (!pc->node || WAIT_RESPONSE(pc)) {
      continue;
    }

    if (pc->state != pc->next_state) {
      busy |= to_next_state(pc);
      if (pc->state == end_em || pc->state == reset_node_end_em) {
        if (pc->err_cache.bgcall != 0
            || pc->err_cache.bgevt != 0) {
          /* Error happens, add the node to fail list */
          mng->lists.fail = g_list_append(mng->lists.fail, pc->node);
        }
        bt_shell_printf("Node[0x%x] Configured", pc->node->addr);
        LOGM("Node[0x%x] Configured", pc->node->addr);
        __cache_reset(pc, NULL);
      }
    }
  }

  return busy;
}
