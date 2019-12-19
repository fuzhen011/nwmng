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

#define SAFE_FREE(p) do { if ((p)) { free((p)); (p) = NULL; } } while (0)

#ifndef ASSERT
#define ASSERT(x) do { if (!(x)) { LOGA(""); abort(); } } while (0)
#define ASSERT_MSG(x, fmt, ...) do { if (!(x)) { LOGA(fmt, ##__VA_ARGS__); abort(); } } while (0)
#endif

#ifndef MAX
#define MAX(a, b)                                   ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)                                   ((a) < (b) ? (a) : (b))
#endif

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
#ifdef __cplusplus
}
#endif
#endif //UTILS_H
