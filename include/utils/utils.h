/*************************************************************************
    > File Name: utils.h
    > Author: Kevin
    > Created Time: 2019-12-14
    > Description:
 ************************************************************************/

#ifndef UTILS_H
#define UTILS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "err.h"

typedef struct {
  uint8_t len;
  uint16_t data[];
}uint16array_t;

typedef struct {
  uint8_t len;
  uint16_t *data;
}uint16list_t;

typedef uint32_t lbitmap_t;
typedef uint16_t sbitmap_t;
typedef uint8_t  bbitmap_t;

enum {
  BASE_DEC,
  BASE_HEX
};

#define ARR_LEN(x) sizeof((x)) / sizeof((x)[0])

#define SAFE_FREE(p) do { if ((p)) { free((p)); (p) = NULL; } } while (0)

#ifndef ASSERT
#define ASSERT(x) do { if (!(x)) { LOGA(""); abort(); } } while (0)
#define ASSERT_MSG(x, fmt, ...) do { if (!(x)) { LOGA(fmt, ##__VA_ARGS__); abort(); } } while (0)
#endif

#ifndef TODOASSERT
#define TODOASSERT() do { LOGA("TODO at [%s:%lu]\n", __FILE__, __LINE__); abort(); } while (0)
#endif

#ifndef MAX
#define MAX(a, b)                                   ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)                                   ((a) < (b) ? (a) : (b))
#endif

#define BITOF(offs)                                 (1 << (offs))
#define BIT_SET(v, bit)                             ((v) |= (1 << (bit)))
#define BIT_CLR(v, bit)                             ((v) &= ~(1 << (bit)))
#define IS_BIT_SET(v, bit)                          (((v) & (1 << (bit))) ? 1 : 0)

#define BUILD_UINT16(l, h)                          (uint16_t)(((l) & 0xFF) | (((h) & 0xFF) << 8))

/* Expected call - if the return value is not the expected value, return the
 * current function with the returned value of the call */
#define EC(exp_ret, func)                          \
  do {                                             \
    if ((exp_ret) != (e = (func))) { return (e); } \
  } while (0)
#define ECG(exp_ret, func, err)                  \
  do {                                           \
    if ((exp_ret) != (e = (func))) { goto err; } \
  } while (0)

#define CHECK_BGCALL(ret, msg)     \
  do {                             \
    if (bg_err_success != (ret)) { \
      LOGBGE(msg, (ret));          \
      return err(ec_bgrsp);        \
    }                              \
  } while (0)

#define CHECK_BGCALL_VOID(ret, msg) \
  do {                              \
    if (bg_err_success != (ret)) {  \
      LOGBGE(msg, (ret));           \
      return;                       \
    }                               \
  } while (0)

char *strdelimit(char *str, char *del, char c);
int strsuffix(const char *str, const char *suffix);

static inline char hex_value_to_char(char x)
{
  /* Calling function needs to make sure the format 0-9, a-f, A-F */
  if (x >= 0 && x <= 9) {
    return x + '0';
  } else if (x >= 0xa && x <= 0xf) {
    return x = x + 'a' - 10;
  } else {
    return x;
  }
}

static inline char hex_char_to_value(char x)
{
  /* Calling function needs to make sure the format 0-9, a-f, A-F */
  if (((x) >= '0') && ((x) <= '9')) {
    return (x) - '0';
  } else if (((x) >= 'a') && ((x) <= 'f')) {
    return (x) - 'a' + 10;
  } else {
    return (x) - 'A' + 10;
  }
}

static inline void uint32_tostr(uint32_t v, char *p)
{
  p[7] = hex_value_to_char(v & 0x0000000F);
  p[6] = hex_value_to_char((v & 0x000000F0) >> 4);
  p[5] = hex_value_to_char((v & 0x00000F00) >> 8);
  p[4] = hex_value_to_char((v & 0x0000F000) >> 12);
  p[3] = hex_value_to_char((v & 0x000F0000) >> 16);
  p[2] = hex_value_to_char((v & 0x00F00000) >> 20);
  p[1] = hex_value_to_char((v & 0x0F000000) >> 24);
  p[0] = hex_value_to_char((v & 0xF0000000) >> 28);
  p[8] = 0;
}

static inline void uint16_tostr(uint16_t v, char *p)
{
  p[3] = hex_value_to_char(v & 0x000F);
  p[2] = hex_value_to_char((v & 0x00F0) >> 4);
  p[1] = hex_value_to_char((v & 0x0F00) >> 8);
  p[0] = hex_value_to_char((v & 0xF000) >> 12);
  p[4] = 0;
}

static inline void uint8_tostr(uint8_t v, char *p)
{
  p[1] = hex_value_to_char(v & 0x0F);
  p[0] = hex_value_to_char((v & 0xF0) >> 4);
  p[2] = 0;
}

err_t str2uint(const char *input,
               size_t length,
               void *p_ret,
               size_t retlen);

err_t uint2str(uint64_t input,
               uint8_t base_type,
               size_t length,
               char *str);

err_t str2cbuf(const char src[],
               uint8_t rev,
               char dest[],
               size_t destLen);
int cbuf2str(const char src[],
             size_t srcLen,
             uint8_t rev,
             char dest[],
             size_t destLen);

static inline void alloc_copy(uint8_t **p,
                              const void *src,
                              size_t len)
{
  if (!p || !len || !src) {
    return;
  }

  *p = (uint8_t *)malloc(len);
  memcpy(*p, src, len);
}

static inline void alloc_copy_u16list(uint16list_t **p,
                                      const uint16list_t *src)
{
  if (!p || !src || !src->len) {
    return;
  }
  *p = (uint16list_t *)malloc(sizeof(uint16list_t));
  (*p)->len = src->len;
  (*p)->data = (uint16_t *)malloc(sizeof(uint16_t) * src->len);
  memcpy((*p)->data, src->data, sizeof(uint16_t) * src->len);
}

static inline void addr_to_buf(uint16_t addr, char *buf)
{
#if 0
  buf[0] = '0';
  buf[1] = 'x';

  buf[2] = hex_value_to_char(addr / 0x1000);
  addr &= 0xfff;
  buf[3] = hex_value_to_char(addr / 0x100);
  addr &= 0xff;
  buf[4] = hex_value_to_char(addr / 0x10);
  addr &= 0xf;
  buf[5] = hex_value_to_char(addr);
#else
  uint2str(addr, BASE_HEX, 6, buf);
#endif
}

static inline void err_exit(const char *pMsg)
{
  perror(pMsg);
  exit(EXIT_FAILURE);
}

static inline void _err_exit(const char *pMsg)
{
  perror(pMsg);
#ifdef __WIN32__
  return;
#else
  _exit(EXIT_FAILURE);
#endif
}

static inline void err_exit_en(int errnum, const char *pMsg)
{
  fprintf(stderr, "%s | error code = %d\n", pMsg, errnum);
  exit(EXIT_FAILURE);
}

#define PTMTX_LOCK(mutex)                           \
  do {                                              \
    int ret;                                        \
    if (0 != (ret = pthread_mutex_lock((mutex)))) { \
      err_exit_en(ret, "pthread_mutex_lock");       \
    }                                               \
  } while (0)

#define PTMTX_UNLOCK(mutex)                           \
  do {                                                \
    int ret;                                          \
    if (0 != (ret = pthread_mutex_unlock((mutex)))) { \
      err_exit_en(ret, "pthread_mutex_unlock");       \
    }                                                 \
  } while (0)

int utils_popcount(uint32_t u);
int utils_ctz(uint32_t u);
int utils_clz(uint32_t u);
int utils_ffs(uint32_t u);
int utils_frz(uint32_t u);

#ifdef __cplusplus
}
#endif
#endif //UTILS_H
