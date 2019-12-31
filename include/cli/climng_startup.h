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
#define FULL_RESET  (BITOF(CLI_INIT)    \
                     | BITOF(CFG_INIT)  \
                     | BITOF(NCP_INIT)  \
                     | BITOF(MNG_INIT))
#define NORMAL_RESET (BITOF(NCP_INIT) \
                      | BITOF(MNG_INIT))
#define FACTORY_RESET (BITOF(CLR_ALL)    \
                       | BITOF(NCP_INIT) \
                       | BITOF(MNG_INIT))

enum {
  CLI_INIT,
  CFG_INIT,
  CLR_ALL,
  NCP_INIT,
  MNG_INIT,
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
