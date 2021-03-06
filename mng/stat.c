/*************************************************************************
    > File Name: stat.c
    > Author: Kevin
    > Created Time: 2020-01-16
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stddef.h>
#include <string.h>
#include "stat.h"
#include "logging.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static stat_t stat = { 0 };

/* Static Functions Declaractions ************************************* */
const stat_t *get_stat(void)
{
  return &stat;
}

void stat_reset(void)
{
  memset(&stat, 0, sizeof(stat_t));
}

void stat_add_start(void)
{
  if (stat.add.time.state != rc_idle) {
    return;
  }
  memset(&stat, 0, sizeof(struct __add));
  stat.add.time.state = rc_start;
  stat.add.time.start = time(NULL);
}

void stat_add_end(void)
{
  if (stat.add.time.state == rc_idle) {
    return;
  }
  stat.add.time.state = rc_end;
  stat.add.time.end = time(NULL);
}

void stat_add_one_dev(void)
{
  stat.add.dev_cnt++;
}

void stat_add_failed(void)
{
  stat.add.fail_times++;
}

void stat_config_start(void)
{
  if (stat.config.time.state != rc_idle) {
    return;
  }
  memset(&stat.config, 0, sizeof(struct __config));
  stat.config.time.state = rc_start;
  stat.config.time.start = time(NULL);
}

void stat_config_end(void)
{
  if (stat.config.time.state == rc_idle) {
    return;
  }
  stat.config.time.state = rc_end;
  stat.config.time.end = time(NULL);
}

void stat_config_one_dev(void)
{
  stat.config.dev_cnt++;
}

void stat_config_retry(void)
{
  stat.config.retry_times++;
}

void stat_config_loading_record(const mng_t *mng)
{
  if (!(mng->state == adding_devices_em || mng->state == configuring_devices_em)) {
    return;
  }
  if (MAX_CONCURRENT_CONFIG_NODES == utils_popcount(mng->cache.config.used)) {
    if (stat.config.full_loading.meas.state == rc_start) {
      return;
    }
    stat.config.full_loading.meas.state = rc_start;
    stat.config.full_loading.meas.start = time(NULL);
  } else if (stat.config.full_loading.meas.state == rc_start) {
    stat.config.full_loading.time += (time(NULL) - stat.config.full_loading.meas.start);
    stat.config.full_loading.meas.state = rc_end;
  }
}

void stat_bl_start(void)
{
  if (stat.bl.time.state != rc_idle) {
    return;
  }
  memset(&stat, 0, sizeof(struct __bl));
  stat.bl.time.state = rc_start;
  stat.bl.time.start = time(NULL);
}

void stat_bl_end(void)
{
  if (stat.bl.time.state == rc_idle) {
    return;
  }
  stat.bl.time.state = rc_end;
  stat.bl.time.end = time(NULL);
}

void stat_rm_start(void)
{
  if (stat.rm.time.state != rc_idle) {
    return;
  }
  memset(&stat.rm, 0, sizeof(struct __rm));
  stat.rm.time.state = rc_start;
  stat.rm.time.start = time(NULL);
}

void stat_rm_end(void)
{
  if (stat.rm.time.state == rc_idle) {
    return;
  }
  stat.rm.time.state = rc_end;
  stat.rm.time.end = time(NULL);
}

void stat_rm_one_dev(void)
{
  stat.rm.dev_cnt++;
}

void stat_rm_retry(void)
{
  stat.rm.retry_times++;
}
void stat_print(void)
{
}
