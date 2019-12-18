/*************************************************************************
    > File Name: err.h
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

#ifndef ERR_H
#define ERR_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef enum {
  ec_success = 0,
  ec_length_leak = 1,
  ec_param_null = 2,
  ec_not_exist = 3,
  ec_file_ope = 4,
  ec_sock = 5,
  ec_param_invalid = 6,
  ec_format = 7,
  ec_state = 8,

  ec_pub_data_not_set = 9,
  ec_all_try_done_still_fail = 10,
  ec_already_exist = 11,
  ec_timeout = 12,
  ec_not_supported = 13,
  /* Json related error code */
  ec_json_null = 30,
  ec_json_open = 31,
  ec_json_format = 32,
}error_code_t;

/* const char *get_error_str(error_code_t e); */

/**
 * @defgroup err
 *
 *
 * The error code generator design is based on below principles:
 *
 * 1. Maximum number of source files is less than @ref{MAX_FILE_NUM}.
 * 2. No files with the same name, even though for the case that they are
 * located in different directories.
 * 3. Maximum lines for each source file are less than @ref{MAX_LINE_NUM}
 * 4. Maximum number of error codes is less than @ref{MAX_ERROR_CODE_NUM}
 *
 * They error code is 32-bit in length and its layout is as below:
 * | bits   | 31-24     | 23-10 | 9-0        |
 * | fields | file enum | lines | error code |
 *
 * @{ */

#ifndef ERROR_BITS
#define ERROR_BITS 32
#endif

typedef uint32_t err32_t;
typedef uint64_t err64_t;

#if (ERROR_BITS == 64)
typedef err64_t err_t;
#else
typedef err32_t err_t;
#endif

#ifndef MAX_SOURCE_FILES_NUM_BITS
#define MAX_SOURCE_FILES_NUM_BITS 8
#endif
#ifndef MAX_LINES_NUM_BITS
#define MAX_LINES_NUM_BITS 14
#endif
#ifndef MAX_ERROR_CODE_NUM_BITS
#define MAX_ERROR_CODE_NUM_BITS 10
#endif

#if (MAX_SOURCE_FILES_NUM_BITS + MAX_LINES_NUM_BITS + MAX_ERROR_CODE_NUM_BITS) > ERROR_BITS
#error "ERROR EXTENDS 32/64 BITS!!"
#endif

#define ERR_MAX(bits) ((1 << (bits)) - 1)
#define MAX_FILE_NUM  (ERR_MAX(MAX_SOURCE_FILES_NUM_BITS))
#define MAX_LINE_NUM  (ERR_MAX(MAX_LINES_NUM_BITS))
#define MAX_ERROR_CODE_NUM  (ERR_MAX(MAX_ERROR_CODE_NUM_BITS))

void print_err(err_t err,
               int (*pfnc_print)(const char *__restrict __format, ...));

/**
 * @brief __err - assemble the err_t
 *
 * @param file - file name where the error happens
 * @param line - line information where the error happens
 * @param ec - error code
 *
 * @return assembled err_t
 */
err_t __err(const char *file, uint32_t line, err_t ec);

/**
 * @brief get_err_file_name - get the file name where the error happens
 *
 * @param ec - error code
 *
 * @return file name or NULL
 */
const char *get_err_file_name(err_t ec);

/**
 * @brief get_err_line - get the line information where the error happens
 *
 * @param ec - error code
 *
 * @return line number
 */
uint32_t get_err_line(err_t ec);

void eprint(err_t err);
/**
 * @brief errof - Get the error code from err_t input
 *
 * @param e - @ref{err_t}
 *
 * @return @ref{error_code_t}
 */
static inline error_code_t errof(err_t e)
{
  return (error_code_t)(e & MAX_ERROR_CODE_NUM);
}
/* Generate the @ref{err_t} from @ref{error_code_t} */
#define err(e)  (__err(__FILE__, (uint32_t)__LINE__, (e)))

/**
 * @brief elog - the same as eprint, but use logging instead of fprint
 *
 * @param err - @ref{err_t}
 */
void elog(err_t err);
/* #define */
/**  @} */

#ifdef __cplusplus
}
#endif
#endif //ERR_H
