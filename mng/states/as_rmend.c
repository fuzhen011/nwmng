/*************************************************************************
    > File Name: as_rmend.c
    > Author: Kevin
    > Created Time: 2020-01-03
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "host_gecko.h"
#include "projconfig.h"
#include "dev_config.h"
#include "utils.h"
#include "logging.h"
#include "generic_parser.h"
#include "gecko_bglib.h"
#include "cli.h"
#include "stat.h"
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
extern const char *state_names[];

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

static void __on_success(config_cache_t *cache);
static void __on_failed(config_cache_t *cache);

int rmend_entry(config_cache_t *cache, func_guard guard)
{
  stat_rm_one_dev();
  if (cache->err_cache.bgcall != 0 || cache->err_cache.bgevt != 0) {
    __on_failed(cache);
  } else {
    /* Node is removed successfully */
    __on_success(cache);
  }
  return asr_suc;
}

static void __on_success(config_cache_t *cache)
{
  uint16_t ret;
  LOGM("Node[0x%04x]: Removed.\n", cache->node->addr);
  bt_shell_printf("Node[0x%04x] Removed\n", cache->node->addr);

  nodeset_errbits(cache->node->addr, 0);
  nodeset_done(cache->node->addr, 0);
  nodeset_func(cache->node->addr, 0);
  nodes_rm(cache->node->addr);
  ret = gecko_cmd_mesh_prov_ddb_delete(*(uuid_128 *)cache->node->uuid)->result;
  if (bg_err_success != ret) {
    LOGBGE("ddb delete", ret);
  }
}

static void __on_failed(config_cache_t *cache)
{
  uint32_t err;
  if (cache->err_cache.bgevt) {
    LOGE("FAILED due to unexpected event received.\n"
         "       State Error Bit Mask- <<0x%08x>>\n"
         "       BGLib Error code - <<0x%04x>>\n",
         cache->err_cache.bgevt,
         cache->err_cache.general);
  }

  if (cache->err_cache.bgcall) {
    if (cache->err_cache.bgcall == 0x80000000) {
      LOGM("IGNORE: User interrupted the procedure.\n");
    } else {
      LOGE("FAILED due to unexpected return value of API calls.\n"
           "       State Error Bit Mask- <<0x%08x>>\n"
           "       BGLib Error code - <<0x%04x>>\n",
           cache->err_cache.bgcall,
           cache->err_cache.general);
    }
  }

  err = (cache->err_cache.bgevt | cache->err_cache.bgcall) & 0x7FFFFFFF;
  nodeset_errbits(cache->node->addr, err);
}
