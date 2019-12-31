/*************************************************************************
    > File Name: projconfig.h
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

#ifndef PROJCONFIG_H
#define PROJCONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif
#ifndef SINGLE_PROC
#define SINGLE_PROC
#endif
#define FILE_PATH_MAX 108

#define MAXSLEEP 128
#define GC_EXP_DEV_NUM  128

#if __APPLE__ == 1
#define PROJ_DIR  "/Users/zhfu/work/projs/nwmng/"
#define PORT      "/dev/tty.usbmodem0004400502021"
#else
#define PROJ_DIR  "/home/zhfu/work/projs/nwmng/"
#define PORT      "/dev/ttyACM2"
#endif

#define TMPLATE_FILE_PATH PROJ_DIR "tools/mesh_config/templates.json"
#define SELFCFG_FILE_PATH PROJ_DIR "tools/mesh_config/test1/prov.json"
#define NWNODES_FILE_PATH PROJ_DIR "tools/mesh_config/test1/nwk.json"

#define CLI_LOG_FILE_PATH PROJ_DIR "logs/cli.log"

#ifndef CLIENT_ENCRYPTED_PATH
#define CLIENT_ENCRYPTED_PATH                       "/tmp/client_encrypted"
#endif
#define ENC_SOCK_FILE_PATH                          "/tmp/enc_sock"

/*
 * Socket file path for communication between cli-mng and cfg processes.
 */
#define CC_SOCK_SERV_PATH "/tmp/ccsock_serv"
#define CC_SOCK_CLNT_PATH "/tmp/ccsock_clnt"

/*
 * NOTE: Make sure this value is NOT greater than the Max Prov Sessions
 * definition on the NCP target side
 */
#define MAX_PROV_SESSIONS 3
/*
 * NOTE: Make sure this value is NOT greater than the Max Foundation Client Cmds
 * definition on the NCP target side
 */
#define MAX_CONCURRENT_CONFIG_NODES 6

#ifdef __cplusplus
}
#endif
#endif //PROJCONFIG_H
