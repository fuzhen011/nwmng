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
 * Simple IPC protocol design, there are only 2 types of messages:
 * - Command
 *   - Set
 *   - Get
 * - Response
 *
 * Currently, there are only 2 config files can be set/gotten:
 *  - provcfg
 *  - node
 *
 * All command-response communication pair should end with **RSP_OK**.
 *
 * Set command sequence:
 * Set -> [Response...] -> [RSP_OK]
 *
 * Get command sequence:
 * Get -> [Response...] -> [RSP_OK]
 *
 * Autonomous event sequence:
 * Response...
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

  RSP_ERR = 253,
  RSP_OK = 254,
  OPC_MAX = 255 
};

typedef uint8_t opc_t;

#define EVT_CMD_DONE(cmd) (OPC_MAX - CMD_GET_PROV)

#ifdef __cplusplus
}
#endif
#endif //OPCODES_H
