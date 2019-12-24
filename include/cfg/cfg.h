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

#include "ccipc.h"
#include "err.h"

err_t cfg_init(void);
int cfg_proc(void);
void *cfg_mainloop(void *p);

int get_ng_addrs(uint16_t *addrs);

err_t sendto_client(opc_t opc, uint8_t len, const void *buf);
#ifdef __cplusplus
}
#endif
#endif //CFG_H
