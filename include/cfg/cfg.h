/*************************************************************************
    > File Name: cfg.h
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

#ifndef CFG_H
#define CFG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "err.h"

err_t cfg_init(void *p);
int get_ng_addrs(uint16_t *addrs);

#ifdef __cplusplus
}
#endif
#endif //CFG_H
