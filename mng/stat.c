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

void stat_reset(void)
{
  memset(&stat, 0, sizeof(stat_t));
}

void stat_add_start(void)
{
  if (stat.arb.add.time.state != rc_idle) {
    LOGW("Adding is recording\n");
    return;
  }
  memset(&stat, 0, sizeof(union __arb));
  stat.arb.add.time.state = rc_start;
  stat.arb.add.time.start = time(NULL);
}

void stat_add_end(void)
{
  if (stat.arb.add.time.state == rc_idle) {
    LOGW("Adding not started yet\n");
    return;
  }
  stat.arb.add.time.state = rc_end;
  stat.arb.add.time.end = time(NULL);
}

void stat_add_one_dev(void)
{
  stat.arb.add.dev_cnt++;
}

void stat_add_failed(void)
{
  stat.arb.add.fail_times++;
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
    LOGW("Adding not started yet\n");
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
