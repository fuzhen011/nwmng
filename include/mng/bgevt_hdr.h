/*************************************************************************
    > File Name: bgevt_hdr.h
    > Author: Kevin
    > Created Time: 2019-12-23
    > Description: 
 ************************************************************************/

#ifndef BGEVT_HDR_H
#define BGEVT_HDR_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*bgevt_hdr)(const void *evt);

void conn_ncptarget(void);
void sync_host_and_ncp_target(void);
void bgevt_dispenser(void);
#ifdef __cplusplus
}
#endif
#endif //BGEVT_HDR_H
