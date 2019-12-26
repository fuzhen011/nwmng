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
#include <stdbool.h>
#include "opcodes.h"
#include "err.h"

#define MAXSLEEP 128

typedef struct {
  int fd;
  int listenfd;
}sock_status_t;

int serv_listen(const char *name);
int serv_accept(int listenfd, uid_t *uidptr);
int cli_conn(const char *self, const char *srv);
err_t sock_send(const sock_status_t *sock,
                opc_t opc, uint8_t len, const void *buf);

#ifdef __cplusplus
}
#endif
#endif //CCIPC_H
