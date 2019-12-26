/*************************************************************************
    > File Name: nwk.h
    > Author: Kevin
    > Created Time: 2019-12-23
    > Description:
 ************************************************************************/

#ifndef NWK_H
#define NWK_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "mng.h"

enum {
  nil,
  syncup,
  initialized,
  configured,
  adding_devices_em,
  configuring_devices_em,
  removing_devices_em,
  blacklisting_devices_em
};

err_t nwk_init(void *p);
#ifdef __cplusplus
}
#endif
#endif //NWK_H
