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

#define GC_EXP_DEV_NUM  128

#if __APPLE__ == 1
#define PROJ_DIR  "/Users/zhfu/work/projs/nwmng/"
#else
#define PROJ_DIR  "/home/zhfu/work/projs/nwmng/"
#endif

#define TMPLATE_FILE_PATH PROJ_DIR "tools/mesh_config/templates.json"
#define SELFCFG_FILE_PATH PROJ_DIR "tools/mesh_config/test1/prov.json"
#define NWNODES_FILE_PATH PROJ_DIR "tools/mesh_config/test1/nwk.json"

#define LOG_FILE_PATH PROJ_DIR "logs/cli.log"

#ifdef __cplusplus
}
#endif
#endif //PROJCONFIG_H
