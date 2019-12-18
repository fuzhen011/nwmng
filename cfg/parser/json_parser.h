/*************************************************************************
    > File Name: json_parser.h
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description:
 ************************************************************************/

#ifndef JSON_PARSER_H
#define JSON_PARSER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "err.h"

err_t json_cfg_open(int cfg_fd,
                    const char *filepath,
                    unsigned int flags,
                    void *out);
void json_close(int cfg_fd);

#ifdef __cplusplus
}
#endif
#endif //JSON_PARSER_H
