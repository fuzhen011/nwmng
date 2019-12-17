/*************************************************************************
    > File Name: utils.c
    > Author: Kevin
    > Created Time: 2019-12-16
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <string.h>

#include <math.h>

#include "utils.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

char *strdelimit(char *str, char *del, char c)
{
  char *dup;

  if (!str) {
    return NULL;
  }

  dup = strdup(str);
  if (dup[0] == '\0') {
    return dup;
  }

  while (del[0] != '\0') {
    char *rep = dup;

    while ((rep = strchr(rep, del[0])))
      rep[0] = c;

    del++;
  }

  return dup;
}

int strsuffix(const char *str, const char *suffix)
{
  int len;
  int suffix_len;

  if (!str || !suffix) {
    return -1;
  }

  if (str[0] == '\0' && suffix[0] != '\0') {
    return -1;
  }

  if (suffix[0] == '\0' && str[0] != '\0') {
    return -1;
  }

  len = strlen(str);
  suffix_len = strlen(suffix);
  if (len < suffix_len) {
    return -1;
  }

  return strncmp(str + len - suffix_len, suffix, suffix_len);
}

err_t str2uint(const char *input,
               size_t length,
               void *p_ret,
               size_t retlen)
{
  int base = 10, tmp, i;
  uint64_t ret = 0;
  const char *x_ret = NULL;
  if (!retlen || !input || !p_ret || !length) {
    return ec_param_invalid;
  }
  if (!memcmp(input, "0x", 2)) {
    base = 16;
    /* Format 0x---... */
    x_ret  = input + 2;
    length -= 2;
    for (i = length - 1; i >= 0; i--) {
      char tmp_c = x_ret[i];
      if (tmp_c >= '0' && tmp_c <= '9') {
        tmp = tmp_c - '0';
      } else if (tmp_c >= 'a' && tmp_c <= 'f') {
        tmp = tmp_c - 'a' + 10;
      } else if (tmp_c >= 'A' && tmp_c <= 'F') {
        tmp = tmp_c - 'A' + 10;
      } else {
        return ec_format;
      }
      ret += tmp * pow(base, length - 1 - i);
    }
  } else {
    x_ret = input;
    for (i = length - 1; i >= 0; i--) {
      char tmp_c = x_ret[i];
      if (tmp_c >= '0' && tmp_c <= '9') {
        tmp = tmp_c - '0';
      } else {
        return ec_format;
      }
      ret += tmp * pow(base, length - 1 - i);
    }
  }
  if (retlen < 2) {
    *(uint8_t *)p_ret = (uint8_t)ret;
  } else if (retlen < 3) {
    *(uint16_t *)p_ret = (uint16_t)ret;
  } else if (retlen < 5) {
    *(uint32_t *)p_ret = (uint32_t)ret;
  } else {
    *(uint64_t *)p_ret = ret;
  }
  return ec_success;
}

err_t uint2str(uint64_t input,
               uint8_t base_type,
               size_t length,
               char *str)
{
  uint64_t ret = 0;
  uint8_t base = 10, remaining = 0, idx = 0;
  if (!length || !str) {
    return ec_param_invalid;
  }

  if (base_type == BASE_DEC) {
    ret = input / base;
    remaining = input % base;
    while (1) {
      if (idx == length) {
        return ec_length_leak;
      }
      str[length - 1 - idx++] = '0' + remaining;
      if (!ret) {
        memmove(str, str + length - idx, idx);
        break;
      }
      remaining = ret % base;
      ret = ret / base;
    }
  } else if (base_type == BASE_HEX) {
    base = 16;
    ret = input / base;
    remaining = input % base;
    str[0] = '0';
    str[1] = 'x';
    while (1) {
      if (idx > length - 2) {
        return ec_length_leak;
      }
      if (remaining < 10) {
        str[length - 1 - idx++] = '0' + remaining;
      } else {
        str[length - 1 - idx++] = 'a' + remaining - 10;
      }
      if (!ret) {
        memmove(str + 2, str + length - idx, idx);
        break;
      }
      remaining = ret % base;
      ret = ret / base;
    }
  } else {
    return ec_param_invalid;
  }
  return ec_success;
}
