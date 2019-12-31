/*************************************************************************
    > File Name: mng_ipc.h
    > Author: Kevin
    > Created Time: 2019-12-30
    > Description:
 ************************************************************************/

#ifndef MNG_IPC_H
#define MNG_IPC_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "err.h"
#include "opcodes.h"

typedef int (*ipcevt_hdr_t)(opc_t opc, uint8_t len, const uint8_t *buf,
                            void *out);

err_t conn_socksrv(void *p);
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
#endif //MNG_IPC_H
