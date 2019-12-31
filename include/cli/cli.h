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
#include <readline/readline.h>

#include "err.h"

#define __DUMP_PARAMS
#ifdef __DUMP_PARAMS
#define DUMP_PARAMS(argc, argv)                            \
  do {                                                     \
    LOGV("CMD - %s with %d params.\n", argv[0], argc - 1); \
    for (int i = 1; i < (argc); i++)                       \
    { LOGV("\tparam[%d]: %s\n", i, (argv)[i]); }           \
  } while (0)
#else
#define DUMP_PARAMS(argc, argv)
#endif

#define CMD_LENGTH  36
#define print_text(color, fmt, args ...) \
  printf(color fmt COLOR_OFF "\n", ## args)
#define print_cmd_usage(cmd)                                   \
  printf(COLOR_HIGHLIGHT "%s %-*s " COLOR_OFF "%s\n",          \
         (cmd)->name, (int)(CMD_LENGTH - strlen((cmd)->name)), \
         (cmd)->arg ? (cmd)->arg : "",                         \
         (cmd)->doc)

typedef err_t (*cmd_exec_func_t)(int argc, char *argv[]);

typedef err_t (*va_param_get_func_t)(void *vap,
                                     int inbuflen,
                                     int *ulen,
                                     int *rlen);
typedef struct {
  const char *name;
  const char *arg;
  cmd_exec_func_t fn;
  const char *doc;
  rl_compdisp_func_t *disp;
  rl_compentry_func_t *argcmpl;
  va_param_get_func_t vpget;
}command_t;

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
