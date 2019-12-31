/*************************************************************************
    > File Name: climng_startup.h
    > Author: Kevin
    > Created Time: 2019-12-30
    > Description:
 ************************************************************************/

#ifndef CLIMNG_STARTUP_H
#define CLIMNG_STARTUP_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
#include "err.h"
#include "projconfig.h"

/*
 * BITMAP for longjmp to set the return value of setjmp, if the bit is set, the
 * corresponding function in {initfs} array will be called.
 */
#define FULL_RESET  (BITOF(CLI_INIT) | BITOF(NCP_INIT) \
                     | BITOF(SOCK_CONN) | BITOF(GET_PROVCFG))
#define NORMAL_RESET (BITOF(NCP_INIT) | BITOF(GET_PROVCFG))
#define FACTORY_RESET (BITOF(CLR_ALL) | BITOF(NCP_INIT) | BITOF(GET_PROVCFG))

enum {
  CLI_INIT,
  CLR_ALL,
  NCP_INIT,
  SOCK_CONN,
  GET_PROVCFG,
};

typedef struct {
  bool initialized;
  bool enc;
  union {
    char port[FILE_PATH_MAX];
    struct {
      char srv[FILE_PATH_MAX];
      char clt[FILE_PATH_MAX];
    }sock;
  };
}proj_args_t;

typedef err_t (*init_func_t)(void *p);
int offsetof_initfunc(init_func_t fn);
const proj_args_t *getprojargs(void);

void on_sock_disconn(void);
#ifdef __cplusplus
}
#endif
#endif //CLIMNG_STARTUP_H
