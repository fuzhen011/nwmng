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

err_t mng_init(bool enc);
int mng_proc(void);
void *mng_mainloop(void *p);

#ifdef __cplusplus
}
#endif
#endif //MNG_H
