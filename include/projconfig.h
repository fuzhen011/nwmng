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

#define TMPLATE_FILE_PATH "tools/mesh_config/templates.json"

#ifdef __cplusplus
}
#endif
#endif //PROJCONFIG_H
