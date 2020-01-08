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
#include "utils.h"
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static acc_t acc = { 0 };

static const acc_state_t as_get_dcd = {
  get_dcd_em,
  getdcd_guard,
  getdcd_entry,
  getdcd_inprg,
  getdcd_retry,
  getdcd_exit,
  is_getdcd_pkts,
  NULL
};

static const acc_state_t as_addappkey = {
  addappkey_em,
  addappkey_guard,
  addappkey_entry,
  addappkey_inprg,
  addappkey_retry,
  addappkey_exit,
  is_addappkey_pkts,
  NULL
};

static const acc_state_t as_bindappkey = {
  bindappkey_em,
  bindappkey_guard,
  bindappkey_entry,
  bindappkey_inprg,
  bindappkey_retry,
  bindappkey_exit,
  is_bindappkey_pkts,
  NULL
};

static const acc_state_t as_setpub = {
  setpub_em,
  setpub_guard,
  setpub_entry,
  setpub_inprg,
  setpub_retry,
  setpub_exit,
  is_setpub_pkts,
  NULL
};

static const acc_state_t as_addsub = {
  addsub_em,
  addsub_guard,
  addsub_entry,
  addsub_inprg,
  addsub_retry,
  addsub_exit,
  is_addsub_pkts,
  NULL
};

static const acc_state_t as_setconfig = {
  setconfig_em,
  setconfig_guard,
  setconfig_entry,
  setconfig_inprg,
  setconfig_retry,
  setconfig_exit,
  is_setconfig_pkts,
  NULL
};

static acc_state_t as_end = {
  end_em,
  NULL,
  end_entry,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static const acc_state_t as_rm = {
  rm_em,
  rm_guard,
  rm_entry,
  rm_inprg,
  rm_retry,
  rm_exit,
  is_rm_pkts,
  NULL
};

static acc_state_t as_rmend = {
  rmend_em,
  NULL,
  rmend_entry,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
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

static inline void __cache_reset_idx(int i)
{
  BIT_CLR(get_mng()->cache.config.used, i);
  __cache_reset(&get_mng()->cache.config.cache[i], NULL);
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
  add_state_after(&as_addappkey, get_dcd_em);
  add_state_after(&as_bindappkey, addappkey_em);
  add_state_after(&as_setpub, bindappkey_em);
  add_state_after(&as_addsub, setpub_em);
  add_state_after(&as_setconfig, addsub_em);
  add_state_after(&as_rm, end_em);
}

static void __acc_reset(bool use_default)
{
  mng_t *mng = get_mng();

  if (mng->lists.fail) {
    mng->lists.config = g_list_concat(mng->lists.config, mng->lists.fail);
    mng->lists.fail = NULL;
  }

  for (int i = 0; i < MAX_CONCURRENT_CONFIG_NODES; i++) {
    __cache_reset(&mng->cache.config.cache[i], NULL);
  }
  mng->cache.config.used = 0;

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
  add_state_after(&as_rmend, end_em);
  if (use_default) {
    __add_default_states();
  }
}

void acc_init(bool use_default)
{
  if (acc.started) {
    return;
  }
  if (!use_default) {
    TODOASSERT();
    return;
  }
  __acc_reset(use_default);

  /* TODO: sigInit(); and timerInit(); */
  acc.started = true;
}

enum {
  type_config,
  type_rm,
};

static void __cache_item_load(mng_t *mng,
                              int ofs,
                              node_t *node,
                              int type)
{
  config_cache_t * cache = &mng->cache.config.cache[ofs];

  /* TODO: needed? */
  __cache_reset_idx(ofs);

  cache->node = node;
  if (type == type_config) {
    cache->state = provisioned_em;
    cache->next_state = get_dcd_em;
    /* TODO:parseFeatures(&cache->features, &pconfig->pNodes[nodeId]); */
    LOGM("Node[%x]: Configuring started\n", node->addr);
  } else if (type == type_rm) {
    cache->state = end_em;
    cache->next_state = rm_em;
    LOGM("Node[%x]: Removing started\n", node->addr);
  }
  BIT_SET(mng->cache.config.used, ofs);
}

static int __caches_load(mng_t *mng, int type)
{
  int loaded = 0, ofs;

  if (MAX_CONCURRENT_CONFIG_NODES == utils_popcount(mng->cache.config.used)
      || g_list_length((type == type_config)
                       ? mng->lists.config : mng->lists.rm) == 0) {
    /* No nodes to config or no room for config for now */
    return 0;
  }

  while ((ofs = utils_frz(mng->cache.config.used)) < MAX_CONCURRENT_CONFIG_NODES) {
    GList *item = g_list_first((type == type_config) ? mng->lists.config : mng->lists.rm);
    if (!item) {
      break;
    }
    __cache_item_load(mng, ofs, item->data, type);
    loaded++;
    if (type == type_config) {
      mng->lists.config = g_list_remove_link(mng->lists.config, item);
    } else {
      mng->lists.rm = g_list_remove_link(mng->lists.rm, item);
    }
    g_list_free(item);
  }
  return loaded;
}

bool acc_loop(void *p)
{
  int cnt;
  if (!acc.started) {
    return false;
  }
  mng_t *mng = (mng_t *)p;
  if (mng->state == removing_devices_em) {
    /* TODO: Load the rm nodes */
    cnt = __caches_load(mng, type_rm);
    if (cnt) {
      LOGM("Loaded %d nodes to remove\n", cnt);
    }
  } else if (mng->state == adding_devices_em || mng->state == configuring_devices_em) {
    cnt = __caches_load(mng, type_config);
    if (cnt) {
      LOGM("Loaded %d nodes to config\n", cnt);
    }
  } else {
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
  config_cache_t *cache = NULL;
  lbitmap_t usedmap;

  usedmap = mng->cache.config.used;
  while (usedmap) {
    i = utils_ctz(usedmap);
    ASSERT(i < MAX_CONCURRENT_CONFIG_NODES);
    BIT_CLR(usedmap, i);
    cache = &mng->cache.config.cache[i];
    as = as_get(cache->state);

    /*
     * Check if any **Exception** (OOM | Guard timer expired) happened in last round
     */
    if (cache->expired && (time(NULL) > cache->expired) && as->retry) {
      ret = as->retry(cache, on_guard_timer_expired_em);
      if (ret != asr_suc) {
        LOGE("Expired Retry Return %d\n", ret);
      }
    } else if (OOM(cache) && as->retry) {
      ASSERT(!WAIT_RESPONSE(cache));
      LOGD("OOM set, now try to recover\n");
      ret = as->retry(cache, on_oom_em);
      if (ret == asr_oom) {
        LOGE("OOM once again, need backoff mechanism\n");
      } else if (ret != asr_suc) {
        LOGE("OOM Retry Return %d\n", ret);
      } else {
        LOGD("Node[%x]: OOM Recovery Once.\n", cache->node->addr);
      }
    }

    /*
     * Check if any state needs to update
     */
    if (WAIT_RESPONSE(cache) || cache->state == cache->next_state) {
      continue;
    }

    busy |= to_next_state(cache);
    if (cache->state == end_em || cache->state == rmend_em) {
      if (cache->err_cache.bgcall != 0
          || cache->err_cache.bgevt != 0) {
        /* Error happens, add the node to fail list */
        mng->lists.fail = g_list_append(mng->lists.fail, cache->node);
      }
      bt_shell_printf("Node[%x] Configured\n", cache->node->addr);
      LOGM("Node[%x] Configured\n", cache->node->addr);
      __cache_reset_idx(i);
    }
  }

  return busy;
}

static inline bool is_config_device_events(const struct gecko_cmd_packet *e)
{
  uint32_t evt_id = BGLIB_MSG_ID(e->header);
  return ((evt_id & 0x000000ff) == 0x000000a0
          && (evt_id & 0x00ff0000) == 0x00270000);
}

static config_cache_t *cache_from_cchandle(const struct gecko_cmd_packet *e)
{
  int i;
  uint32_t handle;
  lbitmap_t usedmap;
  mng_t *mng = get_mng();

  switch (BGLIB_MSG_ID(e->header)) {
    case gecko_evt_mesh_config_client_dcd_data_id:
      handle = e->data.evt_mesh_config_client_dcd_data.handle;
      break;
    case gecko_evt_mesh_config_client_dcd_data_end_id:
      handle = e->data.evt_mesh_config_client_dcd_data_end.handle;
      break;
    case gecko_evt_mesh_config_client_appkey_status_id:
      handle = e->data.evt_mesh_config_client_appkey_status.handle;
      break;
    case gecko_evt_mesh_config_client_binding_status_id:
      handle = e->data.evt_mesh_config_client_binding_status.handle;
      break;
    case gecko_evt_mesh_config_client_model_pub_status_id:
      handle = e->data.evt_mesh_config_client_model_pub_status.handle;
      break;
    case gecko_evt_mesh_config_client_model_sub_status_id:
      handle = e->data.evt_mesh_config_client_model_sub_status.handle;
      break;
    case gecko_evt_mesh_config_client_relay_status_id:
      handle = e->data.evt_mesh_config_client_relay_status.handle;
      break;
    case gecko_evt_mesh_config_client_friend_status_id:
      handle = e->data.evt_mesh_config_client_friend_status.handle;
      break;
    case gecko_evt_mesh_config_client_gatt_proxy_status_id:
      handle = e->data.evt_mesh_config_client_gatt_proxy_status.handle;
      break;
    case gecko_evt_mesh_config_client_default_ttl_status_id:
      handle = e->data.evt_mesh_config_client_default_ttl_status.handle;
      break;
    case gecko_evt_mesh_config_client_network_transmit_status_id:
      handle = e->data.evt_mesh_config_client_network_transmit_status.handle;
      break;
    case gecko_evt_mesh_config_client_reset_status_id:
      handle = e->data.evt_mesh_config_client_reset_status.handle;
      break;
    case gecko_evt_mesh_config_client_beacon_status_id:
      handle = e->data.evt_mesh_config_client_beacon_status.handle;
      break;

    default:
      LOGA("NEED ADD a case[0x%08x] to %s\n",
           BGLIB_MSG_ID(e->header),
           __FUNCTION__);
      break;
  }

  usedmap = mng->cache.config.used;
  while (usedmap) {
    i = utils_ctz(usedmap);
    ASSERT(i < MAX_CONCURRENT_CONFIG_NODES);
    BIT_CLR(usedmap, i);
    if (mng->cache.config.cache[i].node && mng->cache.config.cache[i].cc_handle == handle) {
      return &mng->cache.config.cache[i];
    }
  }

  LOGA("No Cache Found by handle\n");
  return NULL;
}

int dev_config_hdr(const struct gecko_cmd_packet *e)
{
  int ret = 0;
  ASSERT(e);
  config_cache_t *cache;
  acc_state_t *state;

  if (!is_config_device_events(e)) {
    return 0;
  }

  cache = cache_from_cchandle(e);
  state = as_get(cache->state);
  ASSERT(state);

  if (WAIT_RESPONSE(cache) && state->inpg) {
    ret = state->inpg(e, cache);
  }

  /* Drived by timeout event */
  if (!ret && !WAIT_RESPONSE(cache) && EVER_RETRIED(cache) && state->retry) {
    ret |= state->retry(cache, on_timeout_em);
  } else if (ret == asr_unspec) {
    return 0;
  }
  return 1;
}

void timer_set(config_cache_t *cache, bool enable)
{
  mng_t *mng = get_mng();
  if (!enable) {
    cache->expired = 0;
    return;
  }

  cache->expired = time(NULL) + CONFIG_NO_RSP_TIMEOUT;

  if (!cache->dcd.elems || cache->dcd.feature & LPN_BITOFS) {
    if (mng->cfg->timeout) {
      cache->expired += (get_mng()->cfg->timeout->lpn + 999) / 1000;
      return;
    }
    cache->expired += 120;
    return;
  }

  if (mng->cfg->timeout) {
    cache->expired += (get_mng()->cfg->timeout->normal + 999) / 1000;
    return;
  }
  cache->expired += 5;
  return;
}
