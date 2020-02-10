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

#include "stat.h"
#include "mng.h"
#include "err.h"
#include "cfg.h"
#include "gecko_bglib.h"

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
/**
 * @brief - variable parameters get function.
 *
 * @param vap - buffer to store the parameters
 * @param inbuflen - buffer length
 * @param ulen - parameter unit length
 * @param rlen - total parameters length
 *
 * @return @ref{err_t}
 */
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

void cli_print_busy(void);
void cli_print_dev(const node_t *node,
                   const struct gecko_msg_mesh_prov_ddb_get_rsp_t *e);
void cli_print_modelset_done(uint16_t addr, uint8_t type, uint8_t value);
void cli_list_nodes(uint16list_t *ul);
void cli_status(const mng_t *mng);
void cli_print_stat(const stat_t *s);
#ifdef __cplusplus
}
#endif
#endif //CLI_H
