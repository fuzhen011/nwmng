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
/*
 * General project configurations.
 */
#define DEMO_EN
#ifndef STAT
#define STAT
#endif

#define PROJ_VERSION_MAJOR  1
#define PROJ_VERSION_MINOR  0
#define PROJ_VERSION_PATCH  0

#define FILE_PATH_MAX 108
#define MAXSLEEP (60 * 5)

#ifndef SRC_ROOT_DIR
#if __APPLE__ == 1
#define PROJ_DIR  "/Users/zhfu/work/projs/nwmng/"
#else
#define PROJ_DIR  "/home/zhfu/work/projs/nwmng/"
#endif
#else
#define PROJ_DIR SRC_ROOT_DIR
#endif

#if __APPLE__ == 1
/* #define SELFCFG_FILE_PATH PROJ_DIR "tools/mesh_config/home/prov.json" */
/* #define NWNODES_FILE_PATH PROJ_DIR "tools/mesh_config/home/nwk.json" */
#define SELFCFG_FILE_PATH PROJ_DIR "tools/mesh_config/sensor/prov.json"
#define NWNODES_FILE_PATH PROJ_DIR "tools/mesh_config/sensor/nwk.json"
#else
/* #define SELFCFG_FILE_PATH PROJ_DIR "tools/mesh_config/home/prov.json" */
/* #define NWNODES_FILE_PATH PROJ_DIR "tools/mesh_config/home/nwk.json" */
#define SELFCFG_FILE_PATH PROJ_DIR "tools/mesh_config/hcase/prov.json"
#define NWNODES_FILE_PATH PROJ_DIR "tools/mesh_config/hcase/nwk.json"
#endif

#define CONFIG_CACHE_FILE_PATH  PROJ_DIR ".config"
#define TMPLATE_FILE_PATH PROJ_DIR "tools/mesh_config/templates.json"
#define CLI_LOG_FILE_PATH PROJ_DIR "logs/cli.log"

/*
 * NOTE: Make sure this value is NOT greater than the Max Prov Sessions
 * definition on the NCP target side
 */
#define MAX_PROV_SESSIONS 4
/*
 * NOTE: Make sure this value is NOT greater than the Max Foundation Client Cmds
 * definition on the NCP target side
 */
#define MAX_CONCURRENT_CONFIG_NODES 6

/*
 * Typically, each config client bg call will have an event raised no matter
 * because of the status received or timeout occurs, this is the driver of the
 * acc state machine. This timeout is added to safe the situation that there is
 * no event raised by the ncp target (apparently, which is abnormal). The
 * timerout value is an addition to the default timeout value.
 *
 * E.g. for configuring LPN node, if the config client timeout is 20 seconds,
 * the final timeout of this guard will be 20 + CONFIG_NO_RSP_TIMEOUT seconds.
 */
#define CONFIG_NO_RSP_TIMEOUT 3

#define ADD_NO_RSP_TIMEOUT 90

/*
 * If OOM happens in attempt to provision a device, stop scanning and react to
 * unprovisioned beacon event for a while to let the device to recover.
 */
#define OOM_DELAY_TIMEOUT 5

/*
 * Retry times - each config client commands may fail with reasons, retry is
 * implemented, this definitions decide how many times to retry before failure
 */
#define GET_DCD_RETRY_TIMES 5
#define ADD_APP_KEY_RETRY_TIMES 5
#define BIND_APP_KEY_RETRY_TIMES 5
#define SET_PUB_RETRY_TIMES 5
#define ADD_SUB_RETRY_TIMES 5
#define SET_CONFIGS_RETRY_TIMES 5
#define REMOVE_NODE_RETRY_TIMES 3

#ifdef __cplusplus
}
#endif
#endif //PROJCONFIG_H
