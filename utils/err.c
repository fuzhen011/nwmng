/*************************************************************************
    > File Name: err.c
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/err.h"
#include "utils/utils.h"
#include "logging.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
#include "utils/src_names.c"
static const size_t source_file_cnt = sizeof(source_files) / sizeof(char *);

static const char *get_error_str(error_code_t e)
{
  return "";
}

static int file_name_match(const char *file_path,
                           const char *name)
{
  char *n;

  if (!file_path || !name) {
    return 0;
  }

  n = strrchr(file_path, '/');
  if (!n) {
    n = strrchr(file_path, '\\');
  }

  n = (n ? n + 1 : (char *)file_path);
  return (strstr(n, name) != NULL);
}

static uint32_t get_file_index(const char *file)
{
  int i = 0;

  for (; i < source_file_cnt; i++) {
    if (file_name_match(file, source_files[i])) {
      return i;
    }
  }

  fprintf(stderr,
          "%s needs to be added to source_files\n",
          file);
  ASSERT(0);
}

const char *get_err_file_name(err_t ec)
{
  uint32_t idx = (ec >> (MAX_LINES_NUM_BITS + MAX_ERROR_CODE_NUM_BITS)) & MAX_FILE_NUM;

  if (idx >= source_file_cnt) {
    return NULL;
  }
  return source_files[idx];
}

uint32_t get_err_line(err_t ec)
{
  return ((ec >> (MAX_ERROR_CODE_NUM_BITS)) & MAX_LINE_NUM);
}

err_t __err(const char *file,
            uint32_t line,
            err_t ec)
{
  err_t e = 0;

  if (ec == ec_success || (ec & ~(err_t)MAX_ERROR_CODE_NUM) != 0) { \
    /* If success or already recorded error, just return */
    return ec;
  }
  e = (get_file_index(file) << (MAX_ERROR_CODE_NUM_BITS + MAX_LINES_NUM_BITS))
      | (line << (MAX_ERROR_CODE_NUM_BITS))
      | (ec & MAX_ERROR_CODE_NUM);

  return e;
}

void print_err(err_t err,
               int (*pfnc_print)(const char *__restrict __format, ...))
{
  error_code_t e;
  uint32_t line;
  const char *file;
  if (!pfnc_print) {
    return;
  }

  if (ec_success == (e = errof(err))) {
    return;
  }
  ASSERT(0 != (line = get_err_line(err)));
  ASSERT(NULL != (file = get_err_file_name(err)));

  pfnc_print("[1;33m" "[24;41m" "ERROR" "[0m" "(%04u:%s) happended at %s:%u\n",
             e,
             get_error_str(e),
             file,
             line);
}

void eprint(err_t err)
{
  error_code_t e;
  uint32_t line;
  const char *file;
  if (ec_success == (e = errof(err))) {
    return;
  }
  ASSERT(0 != (line = get_err_line(err)));
  ASSERT(NULL != (file = get_err_file_name(err)));

  fprintf(stderr,
          "(%d:%s) happended at %s:%lu\n",
          e,
          get_error_str(e),
          file,
          (unsigned long)line);
}

void elog(err_t err)
{
  error_code_t e;
  uint32_t line;
  const char *file;
  if (ec_success == (e = errof(err))) {
    return;
  }

  line = get_err_line(err);
  file = get_err_file_name(err);

  if (line == 0 || !file) {
    LOGE("No file:line in err_t[%d].\n", err);
  } else {
    LOGE("(%d:%s) happended at %s:%lu\n",
         e,
         get_error_str(e),
         file,
         (unsigned long)line);
  }
}
