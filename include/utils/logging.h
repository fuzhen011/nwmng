/*************************************************************************
    > File Name: logging.h
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

#ifndef LOGGING_H
#define LOGGING_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "err.h"

int logging_init(const char *fp);

err_t log_header(const char *file_name,
                 unsigned int line);

#define LOG() log_header(__FILE__, __LINE__)

#ifdef __cplusplus
}
#endif
#endif //LOGGING_H
