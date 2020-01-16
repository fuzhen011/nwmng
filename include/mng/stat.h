/*************************************************************************
    > File Name: stat.h
    > Author: Kevin
    > Created Time: 2020-01-16
    > Description:
 ************************************************************************/

#ifndef STAT_H
#define STAT_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <sys/time.h>
#include <time.h>

enum {
  rc_idle,
  rc_start,
  rc_end,
};

typedef struct {
  int state;
  time_t start;
  time_t end;
} measure_time_t;

union __arb{
  struct {
    unsigned dev_cnt;
    unsigned fail_times;
    measure_time_t time;
  } add;
};

struct __config{
  unsigned dev_cnt;
  unsigned retry_times;
  measure_time_t time;
};

typedef struct {
  union __arb arb;
  struct __config config;
}stat_t;

void stat_reset(void);

void stat_add_start(void);
void stat_add_end(void);
void stat_add_one_dev(void);
void stat_add_failed(void);

void stat_config_start(void);
void stat_config_end(void);
void stat_config_one_dev(void);
void stat_config_retry(void);

#ifdef __cplusplus
}
#endif
#endif //STAT_H
