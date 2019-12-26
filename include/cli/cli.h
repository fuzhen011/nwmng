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
#include "opcodes.h"
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

typedef int (*ipcevt_hdr_t)(opc_t opc, uint8_t len, const uint8_t *buf,
                            void *out);

void on_sock_disconn(void);

const proj_args_t *getprojargs(void);

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

/**
 * @brief socktocfg - send command to config process and handle the response(s)
 * with provided handler
 *
 * @param opc - command opcode
 * @param len - payload length
 * @param buf - payload
 * @param hdr - handler for handling the response(s)
 *
 * @return @ref{err_t}
 */
err_t socktocfg(opc_t opc, uint8_t len, const void *buf, void *out,
                ipcevt_hdr_t hdr);

/**
 * @brief socktocfg_va - similar function to send command and handle
 * response(s), the difference is the function receive variable fields as
 * payload, with format - pair {len : payload}, ends with 0.
 *
 * @param opc - command opcode
 * @param hdr - handler for handling the response(s)
 * @param ... - variable input with format - pair {len : payload}, ends with 0.
 *
 * @return @ref{err_t}
 */
err_t socktocfg_va(opc_t opc, void *out, ipcevt_hdr_t hdr, ...);

#ifdef __cplusplus
}
#endif
#endif //CLI_H
