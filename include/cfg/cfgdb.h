/*************************************************************************
    > File Name: cfgdb.h
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

#ifndef CFGDB_H
#define CFGDB_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stdbool.h>
#include <glib.h>

#include <pthread.h>

#include "utils.h"

/* Tree ID */
enum {
  tmpl_em,
  upl_em,
  nodes_em,
  backlog_em,
  max_invalid_em
};

/* Lighting bits */
#define ONOFF_SV_BIT  (1UL << 0)
#define LIGHTNESS_SV_BIT  (1UL << 1)
#define CTL_SV_BIT  (1UL << 2)
/* Sensor bits */
#define SENSOR_SV_BIT  (1UL << 3)

#define KIND_LIGHTING (ONOFF_SV_BIT | LIGHTNESS_SV_BIT | CTL_SV_BIT)
#define KIND_SENSOR (SENSOR_SV_BIT)

typedef struct {
  uint8_t cnt;
  uint16_t intv;
}txparam_t;

typedef struct {
  uint32_t normal;
  uint32_t lpn;
}timeout_t;

typedef struct publication{
  uint16_t addr;
  uint16_t aki;
  uint32_t period;
  uint8_t ttl; /* 0x00 - 0x7f valid, 0x80 - 0xfe prohibited, 0xff use default ttl */
  txparam_t txp;
  /* uint8_t security_credentials_flag; */
} publication_t;

/**
 * @brief features that need to be set
 *
 * when setting the features of the node, do below steps:
 * 1. Check if the feature is enabled in dcd by {dcd_status} if for setting
 * RELAY/PROXY/FRIEND/LPN, others pass.
 * 2. Check target^current to get which features need to be configured
 * 3. Configure the features until target^current=0
 *
 */
typedef struct {
  sbitmap_t dcd_status;
  sbitmap_t target;
  sbitmap_t current;
  txparam_t *relay_txp;
}features_t;

/**
 * @brief - Template structure, only the reference ID is mandatory
 */
typedef struct {
  uint16_t refid;
  uint8_t *ttl;
  uint8_t *snb;
  txparam_t *net_txp;
  publication_t *pub;
  uint16list_t *bindings;
  uint16list_t *sublist;
  features_t features;
} tmpl_t;

typedef struct {
  uint8_t *ttl;
  uint8_t *snb;
  txparam_t *net_txp;
  features_t features;
  publication_t *pub;
  uint16list_t *bindings;
  uint16list_t *sublist;
}mesh_config_t;

/**
 * @brief Node structure, all the configuration of a node will be loaded to the
 * structure, all fields with pointer type are optional to present, the others
 * are mandatory.
 */
typedef struct {
  uint8_t uuid[16];
  uint16_t addr;
  uint8_t done;
  uint8_t rmorbl; /* Remove or blacklist state */
  lbitmap_t err;
  uint8_t *tmpl;
  mesh_config_t config;
  struct {
    /* enum value - see {CTL_SV_BIT} */
    uint8_t func;
    lbitmap_t venmod_supt;
  }models;
}node_t;

/**
 * @brief All the mesh keys used in this program are identified by the @{refid},
 * the reason why not use the real id is the key could be used before it is
 * created, so the real id is unknown at that point.
 */
typedef struct {
  uint16_t refid;
  uint16_t id;
  uint8_t done;
  uint8_t val[16];
}meshkey_t;

typedef struct {
  uint8_t appkey_num;
  /* It's possibly the appkey number is greater than the maximum appkey number
   * supported by the provisioner, {active_appkey_num} holds the number of
   * appkeys which are really created, while {appkey_num} holds the number in
   * config file  */
  uint8_t active_appkey_num;
  meshkey_t netkey;
  meshkey_t appkey[];
}subnet_t;

typedef struct {
  uint16_t addr;
  time_t sync_time;
  uint32_t ivi;
  uint8_t subnet_num;
  subnet_t *subnets;
  uint8_t *ttl;
  txparam_t *net_txp;
  timeout_t *timeout;
}provcfg_t;

typedef struct {
  GTree *templates;
  GTree *unprov_devs;
  GTree *nodes;
  GTree *backlog;
  /* TODO: Below 2 lists are not used yet */
  /* Ideas are to keep them as "set" and add node list to each group entry */
  GList *pubgroups;
  GList *subgroups;
} cfg_devdb_t;

typedef struct {
  bool initialized;
  pthread_rwlock_t lock;
  cfg_devdb_t devdb;
  provcfg_t self;
}cfgdb_t;

/**
 * @brief cfgdb_init - initialized the cfg database, allocate initial memory.
 *
 * @return @ref{err_t}
 */
err_t cfgdb_init(void);

/**
 * @brief cfgdb_deinit - de-initialized the cfg database, free all the
 * allocated memory.
 */
void cfgdb_deinit(void);

/**
 * @brief cfgdb_get_devnum - get the node number in the specified device tree.
 *
 * @param which - tree ID
 *
 * @return - number of nodes in the tree
 */
int cfgdb_get_devnum(int which);
/**
 * @defgroup cfgdb_get
 *
 * CFG database get functions. Input is the key, return the node if found and
 * NULL if not found.
 *
 * @{ */
node_t *cfgdb_node_get(uint16_t addr);
node_t *cfgdb_unprov_dev_get(const uint8_t *uuid);
node_t *cfgdb_backlog_get(const uint8_t *uuid);
tmpl_t *cfgdb_tmpl_get(uint16_t refid);
/**  @} */

/**
 * @defgroup cfgdb_add
 *
 * CFG database add functions. Node should be allocated outside, functions will
 * check if it's already in the database, if yes, compare both pointers, if
 * equal, do nothing, if not, replace the existing one which will be freed
 * afterwards. If not in, add it.
 *
 * @{ */
err_t cfgdb_backlog_add(node_t *n);
err_t cfgdb_unpl_add(node_t *n);
err_t cfgdb_nodes_add(node_t *n);
err_t cfgdb_tmpl_add(tmpl_t *n);
/**  @} */
/**
 * @defgroup cfgdb_remove
 *
 * Remove a node from the specified tree, if destory is ture, the memory will be
 * freed, if destory is false, the memory will be kept.
 *
 * @{ */
err_t cfgdb_backlog_remove(node_t *n, bool destory);
err_t cfgdb_unpl_remove(node_t *n, bool destory);
err_t cfgdb_nodes_remove(node_t *n, bool destory);
void cfgdb_remove_all_upl(void);
void cfgdb_remove_all_nodes(void);
err_t cfgdb_tmpl_remove(tmpl_t *n);
/**  @} */

provcfg_t *get_provcfg(void);

void cfg_load_mnglists(GTraverseFunc func);

/**
 * @brief get_node_addrs - get the list of nodes in the provisioned database,
 * calling function needs to free the returned list if not NULL.
 *
 * @return list of nodes, or NULL if empty
 */
uint16list_t *get_node_addrs(void);
/**
 * @brief get_node_addrs - get the list of light nodes in the provisioned
 * database, calling function needs to free the returned list if not NULL.
 *
 * @param func - bit mask for feature @{ONOFF_SV_BIT}
 *
 * @return list of nodes, or NULL if empty
 */
uint16list_t *get_lights_addrs(uint8_t func);
#ifdef __cplusplus
}
#endif
#endif //CFGDB_H
