/*************************************************************************
    > File Name: logging.c
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "logging.h"
#include "utils.h"
/* Defines  *********************************************************** */
#define LOGBUF_SIZE 256

#define LOGGING_DBG
#ifdef LOGGING_DBG
#define LD(...) printf(__VA_ARGS__)
#else
#define LD(...)
#endif

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static size_t log_len = 0;
static char buf[LOGBUF_SIZE] = { 0 };

/* Static Functions Declaractions ************************************* */

/**
 * @brief get_now_str - fill the @param{str} with time as [Time]
 *
 * @param str - buffer to be filled
 * @param input_len - length of the buffer
 * @param len - real filled length
 *
 * @return @ref{err_t}
 */
static err_t fill_time(char *str,
                       size_t input_len,
                       size_t *len)
{
  time_t t;
  struct tm *tm;
  size_t r;

  if (!str || !len || !input_len) {
    return err(ec_param_null);
  }

  time(&t);
  tm = localtime(&t);

  str[0] = '[';
  r = strftime(str + 1,
               input_len - 1,
               "%F %X",
               tm);
  if (r == 0 || r + 2 > input_len) {
    return err(ec_length_leak);
  }
  str[r + 1] = ']';
  /* The fact -  "%F %X" always takes 21 bytes length */
  if (input_len > r + 2) {
    str[r + 2] = '\0';
  }
  *len = r + 2;

  return ec_success;
}
#define FILE_NAME_LENGTH  10
#define LINE_NAME_LENGTH  5
#define FILE_LINE_LENGTH  (FILE_NAME_LENGTH + LINE_NAME_LENGTH + 3)

static err_t fill_file_line(const char *file_name,
                            unsigned int line,
                            char *str,
                            size_t input_len,
                            size_t offset,
                            size_t *len)
{
  char *p, *n, *posend;
  char tmp[FILE_NAME_LENGTH] = { 0 };
  if (!file_name || !str || !len || !input_len) {
    return err(ec_param_null);
  }
  if (offset + FILE_LINE_LENGTH > input_len) {
    return err(ec_length_leak);
  }
  p = str + offset;

  p[0] = '[';

  n = strrchr(file_name, '/');
  if (!n) {
    n = strrchr(file_name, '\\');
  }
  n = (n ? n + 1 : (char *)file_name);
  posend = strchr(n, '.');

  memcpy(tmp,
         n,
         MIN(FILE_NAME_LENGTH, posend - n));
  snprintf(p + 1,
           input_len - offset - 1,
           "%10.10s:%-5d",
           tmp,
           line);
  p[FILE_LINE_LENGTH - 1] = ']';
  if (offset + FILE_LINE_LENGTH < input_len) {
    p[FILE_LINE_LENGTH] = '\0';
  }
  return ec_success;
}

err_t log_header(const char *file_name,
                 unsigned int line)
{
  err_t e;
  EC(ec_success, fill_time(buf, LOGBUF_SIZE, &log_len));
  EC(ec_success, fill_file_line(file_name,
                                line,
                                buf,
                                LOGBUF_SIZE,
                                log_len,
                                &log_len));
  LD("%s\n", buf);
  return ec_success;
}

int logging_init(const char *fp)
{
  return 1;
}
