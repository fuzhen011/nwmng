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

struct __add{
  unsigned dev_cnt;
  unsigned fail_times;
  measure_time_t time;
};

struct __rm{
  unsigned dev_cnt;
  unsigned retry_times;
  measure_time_t time;
};

struct __bl{
  measure_time_t time;
};

struct __config{
  unsigned dev_cnt;
  unsigned retry_times;
  measure_time_t time;
};

typedef struct {
  struct __add add;
  struct __rm rm;
  struct __bl bl;
  struct __config config;
}stat_t;

const stat_t *get_stat(void);
void stat_reset(void);

void stat_add_start(void);
void stat_add_end(void);
void stat_add_one_dev(void);
void stat_add_failed(void);

void stat_config_start(void);
void stat_config_end(void);
void stat_config_one_dev(void);
void stat_config_retry(void);

void stat_bl_start(void);
void stat_bl_end(void);

void stat_rm_start(void);
void stat_rm_end(void);
void stat_rm_one_dev(void);
void stat_rm_retry(void);
#ifdef __cplusplus
}
#endif
#endif //STAT_H
