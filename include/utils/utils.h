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
#define EC(exp_ret, func)                             \
  do {                                                \
    if ((exp_ret) != (e = (func))) { return err(e); } \
  } while (0)
#ifdef __cplusplus
}
#endif
#endif //UTILS_H
