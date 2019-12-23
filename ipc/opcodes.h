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
 *
 */
enum {
  CMD_GET_PROV,

  OPC_SET_PROV,
  OPC_SET,

  OPC_MAX = 0xff
};

#define EVT_CMD_DONE(cmd) (OPC_MAX - CMD_GET_PROV)

#ifdef __cplusplus
}
#endif
#endif //OPCODES_H
