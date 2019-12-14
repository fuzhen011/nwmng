/*************************************************************************
    > File Name: logging.c
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "logging.h"
#include "utils.h"
/* Defines  *********************************************************** */
#define LOGBUF_SIZE 512

#define LOGGING_DBG
#ifdef LOGGING_DBG
#define LD(...) printf(__VA_ARGS__)
#else
#define LD(...)
#endif

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
typedef struct {
  FILE *fp;
  unsigned int level;
  int tostdout;
  size_t log_len;
  char buf[LOGBUF_SIZE];
}logcfg_t;

static logcfg_t lcfg = {
  NULL,
  LVL_VER,
  0,
  0,
  { 0 },
};

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
  *len = offset + FILE_LINE_LENGTH;
  return ec_success;
}
#define AST_FLAG  "[" RTT_CTRL_TEXT_BRIGHT_RED "AST" RTT_CTRL_RESET "]"
#define ERR_FLAG  "[" RTT_CTRL_BG_BRIGHT_RED "ERR" RTT_CTRL_RESET "]"
#define WRN_FLAG  "[" RTT_CTRL_BG_BRIGHT_YELLOW "WRN" RTT_CTRL_RESET "]"
#define MSG_FLAG  "[" RTT_CTRL_BG_BRIGHT_BLUE "MSG" RTT_CTRL_RESET "]"
#define DBG_FLAG  "[" RTT_CTRL_BG_BRIGHT_GREEN "DBG" RTT_CTRL_RESET "]"
#define VER_FLAG  "[" "VER" "]"

static err_t fill_lvl(int lvl,
                      char *str,
                      size_t input_len,
                      size_t offset,
                      size_t *len)
{
  const char *flag;
  char *p;
  size_t flaglen;

  if (!str || !len || !input_len) {
    return err(ec_param_null);
  }

  switch (lvl) {
    case LVL_AST:
      flag = AST_FLAG;
      flaglen = sizeof(AST_FLAG);
      break;
    case LVL_ERR:
      flag = ERR_FLAG;
      flaglen = sizeof(ERR_FLAG);
      break;
    case LVL_WRN:
      flag = WRN_FLAG;
      flaglen = sizeof(WRN_FLAG);
      break;
    case LVL_MSG:
      flag = MSG_FLAG;
      flaglen = sizeof(MSG_FLAG);
      break;
    case LVL_DBG:
      flag = DBG_FLAG;
      flaglen = sizeof(DBG_FLAG);
      break;
    default:
      flag = VER_FLAG;
      flaglen = sizeof(VER_FLAG);
      break;
  }
  /* LD("%d - %lu\n", lvl, flaglen); */

  if (offset + flaglen + 2 > input_len) {
    return err(ec_length_leak);
  }

  p = str + offset;
  /* sizeof contains the '\0' */
  memcpy(p, flag, flaglen);
  p[flaglen - 1] = ':';
  p[flaglen] = ' ';
  *len = offset + flaglen + 1;
  return ec_success;
}

err_t __log(const char *file_name,
            unsigned int line,
            int lvl,
            const char *fmt,
            ...)
{
  err_t e;
  va_list valist;

  if (lvl >= (int)lcfg.level) {
    return ec_success;
  }

  EC(ec_success, fill_time(lcfg.buf, LOGBUF_SIZE, &lcfg.log_len));
  EC(ec_success, fill_file_line(file_name,
                                line,
                                lcfg.buf,
                                LOGBUF_SIZE,
                                lcfg.log_len,
                                &lcfg.log_len));
  EC(ec_success, fill_lvl(lvl,
                          lcfg.buf,
                          LOGBUF_SIZE,
                          lcfg.log_len,
                          &lcfg.log_len));

  va_start(valist, fmt);
  vsnprintf(lcfg.buf + lcfg.log_len,
            LOGBUF_SIZE - lcfg.log_len,
            fmt,
            valist);
  if (lcfg.fp) {
    fprintf(lcfg.fp, "%s", lcfg.buf);
  }
  if (lcfg.tostdout) {
    printf("%s", lcfg.buf);
  }

  return ec_success;
}

void logging_demo(void)
{
  const char *msg[] = {
    "This is an assert message",
    "This is an error message",
    "This is a  warning message",
    "This is a  message message",
    "This is a  debug message",
    "This is a  verbose message",
  };
  for (int i = LVL_AST; i <= LVL_VER; i++) {
    LOG(i, "%d ------ %s\n", i, msg[i + 1]);
  }
}

err_t logging_init(const char *path,
                   int tostdout,
                   unsigned int lvl_threshold)
{
  int ret;
  if (lcfg.fp) {
    return ec_success;
  }
  ret = access(path, R_OK | W_OK | F_OK);
  if (-1 == ret) {
    if (errno == ENOENT) {
      fprintf(stderr, "%s not exists\n", path);
      return err(ec_not_exist);
    }
    fprintf(stderr, "The current user don 't have the RW permit? Error[%u]\n", errno);
    return err(ec_file_ope);
  }

  lcfg.fp = fopen(path, "a+");
  if (!lcfg.fp) {
    fprintf(stderr, "Cannot open %s\n", path);
    return err(ec_file_ope);
  }
  setlinebuf(lcfg.fp);
  lcfg.tostdout = tostdout;
  lcfg.level = lvl_threshold;

  return ec_success;
}

void logging_deinit(void)
{
  memset(&lcfg, 0, sizeof(logcfg_t));
}
