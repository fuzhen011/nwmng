/*************************************************************************
    > File Name: as_rmend.c
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

/* Global Variables *************************************************** */
extern const char *stateNames[];

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

static void __on_success(config_cache_t *cache);
static void __on_failed(config_cache_t *cache);

int rmend_entry(config_cache_t *cache, func_guard guard)
{
  /* Alarm SHOULD be set in the main engine */
  if (cache->err_cache.bgcall != 0 || cache->err_cache.bgevt != 0) {
    __on_failed(cache);
  } else {
    /* Node is configured successfully */
    __on_success(cache);
  }

  /* TODO: */
  /* onDeviceDone(actionTBR, cache->node->addr); */
  /* forceGenericReloadActions(); */
  return asr_suc;
}

/* static void __on_success(config_cache_t *cache, networkConfig_t *pconfig) */
/* { */
/* } */
static void __on_success(config_cache_t *cache)
{
  LOGM("Node[%x] Has Been Removed Properly.\n", cache->node->addr);

  cache->node->err = 0;
  /* TODO */
  /* setNodeErrBitsToFile(pconfig->pNodes[cache->node->addr].uuid, 0); */

  cache->node->done = 0;
  /* TODO */
  /* setNodeDoneToFile(pconfig->pNodes[cache->node->addr].uuid, */
                    /* pconfig->pNodes[cache->node->addr].done); */
  /* removeDeviceFromLDDB(typeTBR, cache->node->addr); */
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
  cache->node->err = err;
  /* TODO */
  /* setNodeErrBitsToFile(pconfig->pNodes[cache->node->addr].uuid, err); */
}
