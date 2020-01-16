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

err_t nwk_init(void *p);
int bgevt_dflt_hdr(const struct gecko_cmd_packet *evt);
#ifdef __cplusplus
}
#endif
#endif //NWK_H
