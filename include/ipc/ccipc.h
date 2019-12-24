/*************************************************************************
    > File Name: ccipc.h
    > Author: Kevin
    > Created Time: 2019-12-24
    > Description: 
 ************************************************************************/

#ifndef CCIPC_H
#define CCIPC_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "opcodes.h"
#define MAXSLEEP 128

int serv_listen(const char *name);
int serv_accept(int listenfd, uid_t *uidptr);
int cli_conn(const char *self, const char *srv);

#ifdef __cplusplus
}
#endif
#endif //CCIPC_H
