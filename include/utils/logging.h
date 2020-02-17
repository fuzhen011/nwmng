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
#include <stdbool.h>
#include "err.h"

#define COLOR_OFF "\x1B[0m"
#define COLOR_BLACK "\x1B[0;30m"
#define COLOR_RED "\x1B[0;31m"
#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_YELLOW  "\x1B[0;33m"
#define COLOR_BLUE  "\x1B[0;34m"
#define COLOR_MAGENTA "\x1B[0;35m"
#define COLOR_CYAN  "\x1B[0;36m"
#define COLOR_WHITE "\x1B[0;37m"
#define COLOR_WHITE_BG  "\x1B[0;47;30m"
#define COLOR_HIGHLIGHT "\x1B[1;39m"

#define COLOR_RED_BOLD    "\x1B[1;31m"
#define COLOR_GREEN_BOLD  "\x1B[1;32m"
#define COLOR_BLUE_BOLD   "\x1B[1;34m"
#define COLOR_MAGENTA_BOLD  "\x1B[1;35m"
/******************************************************************
 * Copied from RTT
 * ***************************************************************/
#define RTT_CTRL_RESET                "[0m"         // Reset to default colors
#define RTT_CTRL_CLEAR                "[2J"         // Clear screen, reposition cursor to top left

#define RTT_CTRL_TEXT_BLACK           "[2;30m"
#define RTT_CTRL_TEXT_RED             "[2;31m"
#define RTT_CTRL_TEXT_GREEN           "[2;32m"
#define RTT_CTRL_TEXT_YELLOW          "[2;33m"
#define RTT_CTRL_TEXT_BLUE            "[2;34m"
#define RTT_CTRL_TEXT_MAGENTA         "[2;35m"
#define RTT_CTRL_TEXT_CYAN            "[2;36m"
#define RTT_CTRL_TEXT_WHITE           "[2;37m"

#define RTT_CTRL_TEXT_BRIGHT_BLACK    "[1;30m"
#define RTT_CTRL_TEXT_BRIGHT_RED      "[1;31m"
#define RTT_CTRL_TEXT_BRIGHT_GREEN    "[1;32m"
#define RTT_CTRL_TEXT_BRIGHT_YELLOW   "[1;33m"
#define RTT_CTRL_TEXT_BRIGHT_BLUE     "[1;34m"
#define RTT_CTRL_TEXT_BRIGHT_MAGENTA  "[1;35m"
#define RTT_CTRL_TEXT_BRIGHT_CYAN     "[1;36m"
#define RTT_CTRL_TEXT_BRIGHT_WHITE    "[1;37m"

#define RTT_CTRL_BG_BLACK             "[24;40m"
#define RTT_CTRL_BG_RED               "[24;41m"
#define RTT_CTRL_BG_GREEN             "[24;42m"
#define RTT_CTRL_BG_YELLOW            "[24;43m"
#define RTT_CTRL_BG_BLUE              "[24;44m"
#define RTT_CTRL_BG_MAGENTA           "[24;45m"
#define RTT_CTRL_BG_CYAN              "[24;46m"
#define RTT_CTRL_BG_WHITE             "[24;47m"

#define RTT_CTRL_BG_BRIGHT_BLACK      "[4;40m"
#define RTT_CTRL_BG_BRIGHT_RED        "[4;41m"
#define RTT_CTRL_BG_BRIGHT_GREEN      "[4;42m"
#define RTT_CTRL_BG_BRIGHT_YELLOW     "[4;43m"
#define RTT_CTRL_BG_BRIGHT_BLUE       "[4;44m"
#define RTT_CTRL_BG_BRIGHT_MAGENTA    "[4;45m"
#define RTT_CTRL_BG_BRIGHT_CYAN       "[4;46m"
#define RTT_CTRL_BG_BRIGHT_WHITE      "[4;47m"

#define AST_FLAG  "[" RTT_CTRL_TEXT_BRIGHT_RED "AST" RTT_CTRL_RESET "]"
#define ERR_FLAG  "[" RTT_CTRL_BG_BRIGHT_RED "ERR" RTT_CTRL_RESET "]"
#define WRN_FLAG  "[" RTT_CTRL_BG_BRIGHT_YELLOW "WRN" RTT_CTRL_RESET "]"
#define MSG_FLAG  "[" RTT_CTRL_BG_BRIGHT_BLUE "MSG" RTT_CTRL_RESET "]"
#define DBG_FLAG  "[" RTT_CTRL_BG_BRIGHT_GREEN "DBG" RTT_CTRL_RESET "]"
#define VER_FLAG  "[" "VER" "]"

/* LVL_AST cannot be masked */
typedef enum {
  LVL_AST = -1,
  LVL_ERR,
  LVL_WRN,
  LVL_MSG,
  LVL_DBG,
  LVL_VER
}log_lvl_t;

/**
 * @brief logging_init - initialization of logging
 *
 * @param path - which file the log is writen to
 * @param tostdout - set to 1 also writes to stdout
 * @param lvl_threshold - set the logging level threshold, the logging with
 * level **LESS THAN OR EQUAL** the threshold will be writen to file.
 *
 * @return @ref{err_t}
 */
err_t logging_init(const char *path,
                   bool tostdout,
                   unsigned int lvl_threshold);

/**
 * @brief logging_deinit - de-initialization of logging
 */
void logging_deinit(void);

/**
 * @brief logging_demo - output the demonstration of logging to stdout
 */
void logging_demo(void);

err_t __log(const char *file_name,
            unsigned int line,
            int lvl,
            const char *fmt,
            ...);

void log_n(void);
#define LOG(lvl, fmt, ...) __log(__FILE__, __LINE__, (lvl), (fmt), ##__VA_ARGS__)
#define LOGN() log_n()

/*
 * Below 7 LOGx macros are used for logging data in specific level.
 */
#define LOGA(fmt, ...) \
  do { LOG(LVL_AST, (fmt), ##__VA_ARGS__); abort(); } while (0)
#define LOGE(fmt, ...) LOG(LVL_ERR, (fmt), ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG(LVL_WRN, (fmt), ##__VA_ARGS__)
#define LOGM(fmt, ...) LOG(LVL_MSG, (fmt), ##__VA_ARGS__)
#define LOGD(fmt, ...) LOG(LVL_DBG, (fmt), ##__VA_ARGS__)
#define LOGV(fmt, ...) LOG(LVL_VER, (fmt), ##__VA_ARGS__)

#define LOGBGE(what, err) LOGE(what " returns Error[0x%04x]\n", (err))

void set_logging_tostdout(int enable);
int get_logging_tostdout(void);
void set_logging_lvl_threshold(log_lvl_t lvl);
int get_logging_lvl_threshold(void);

#ifdef __cplusplus
}
#endif
#endif //LOGGING_H
