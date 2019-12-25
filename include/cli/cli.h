/*************************************************************************
    > File Name: cli.h
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

#ifndef CLI_H
#define CLI_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdlib.h>
#include <stdbool.h>
#include "projconfig.h"
#include "err.h"

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

const proj_args_t *getprojargs(void);

int get_children(pid_t **p);

err_t cli_proc_init(int child_num, const pid_t *pids);
int cli_proc(int argc, char *argv[]);
void *cli_mainloop(void *pIn);

void bt_shell_printf(const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#endif //CLI_H
