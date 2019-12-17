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
#include "utils.h"

enum {
  relay_bitoffs,
  proxy_bitoffs,
  friend_bitoffs,
  lowpower_bitoffs
};

enum {
  onoff_support,
  lightness_support,
  ctl_support,
};

typedef struct {
  uint8_t cnt;
  uint16_t intv;
}txparam_t;

typedef struct publication{
  uint16_t addr;
  uint16_t aki;
  uint32_t period;
  uint8_t ttl; /* 0x00 - 0x7f valid, 0x80 - 0xfe prohibited, 0xff use default ttl */
  txparam_t txp;
  /* uint8_t security_credentials_flag; */
} publication_t;

typedef struct {
  uint8_t uuid[16];
  uint16_t addr;
  struct {
    uint8_t ttl;
    txparam_t *net_txp;
    txparam_t *relay_txp;
    /*
     * when setting the features of the node, do below steps:
     * 1. Check if the feature is enabled in dcd, goto step 2 if yes.
     * 2. Check if the feature needs to be set explicitly, goto step 3 if yes.
     * 3. Check the target status (1-enable/0-disable/2-keep) in value
     * 4. Set the feature if it's not "2-keep"
     */
    struct {
      uint8_t dcd_status;
      uint8_t needset;
      uint8_t value;
    }features;

    publication_t pub;
    uint16list_t *bindings;
    uint16list_t *sublist;
  }config;

  struct {
    uint16_t light_supt;
    uint16_t venmod_supt;
  }models;
}node_t;

void cfgdb_deinit(void);

#ifdef __cplusplus
}
#endif
#endif //CFGDB_H