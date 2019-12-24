/*************************************************************************
    > File Name: opcodes.h
    > Author: Kevin
    > Created Time: 2019-12-23
    > Description:
 ************************************************************************/

#ifndef OPCODES_H
#define OPCODES_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
/*
 * Set xxx to config file, where xxx could be:
 *  - provcfg
 *  - node
 *
 * Sequence:
 * - Set xxx
 * - Set yyy
 * - ...
 * - Exec set [provcfg | node]
 *
 *
 * CPS - Command Prov Set
 * CPG - Command Prov Get
 *
 */
enum {
  CPS_CLRCTL,
  CPS_ADDR,
  CPS_SYNCTIME,
  CPS_NETKEYID,
  CPS_NETKEYDONE,
  CPS_APPKEYID,
  CPS_APPKEYDONE,

  CPG_ALL,

  RSP_PROV_BASIC,
  RSP_PROV_SUBNETS,
  RSP_PROV_TTL,
  RSP_PROV_TXP,
  RSP_PROV_TIMEOUT,

  RPS_CTL_START = 0xe0,
  RSP_EVT,
  RSP_ERR,
  RSP_OK,
  OPC_MAX = 0xff
};

typedef uint8_t opc_t;

#define EVT_CMD_DONE(cmd) (OPC_MAX - CMD_GET_PROV)

#ifdef __cplusplus
}
#endif
#endif //OPCODES_H
