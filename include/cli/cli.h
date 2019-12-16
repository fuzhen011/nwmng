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
int get_children(pid_t **p);

int cli_proc_init(int child_num, const pid_t *pids);
int cli_proc(void);

void bt_shell_printf(const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#endif //CLI_H
