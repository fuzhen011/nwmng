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
#include "err.h"
#include "opcodes.h"
err_t cli_init(void *p);

void on_sock_disconn(void);

int get_children(pid_t **p);

err_t cli_proc_init(int child_num, const pid_t *pids);
int cli_proc(int argc, char *argv[]);
void *cli_mainloop(void *pIn);

/**
 * @brief bt_shell_printf - command to print message on shell without prompt
 *
 * @param fmt - format
 * @param ... - variable inputs
 */
void bt_shell_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif //CLI_H
