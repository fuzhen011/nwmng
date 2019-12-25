/*************************************************************************
    > File Name: mng.h
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

#ifndef MNG_H
#define MNG_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
#include "err.h"

#include "cfgdb.h"

typedef struct {
  int state;
  provcfg_t cfg;
  struct{
    bool scanning;
    bool free_mode;
  }status;
}mng_t;

err_t init_ncp(void *p);

int mng_proc(void);
void *mng_mainloop(void *p);
mng_t *get_mng(void);

#ifdef __cplusplus
}
#endif
#endif //MNG_H
