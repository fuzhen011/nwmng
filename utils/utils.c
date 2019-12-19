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
#define STR_HEX_VALID(x)             \
  ((((x) >= '0') && ((x) <= '9'))    \
   || (((x) >= 'a') && ((x) <= 'f')) \
   || (((x) >= 'A') && ((x) <= 'F')))

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

static inline char __asmbhex(char h, char l)
{
  return h * 16 + l;
}

int cbuf2str(const char src[],
             size_t srcLen,
             uint8_t rev,
             char dest[],
             size_t destLen)
{
  int len;
  const unsigned char *p;

  p = (const unsigned char *)src;
  if (p == NULL) {
    return err(ec_param_invalid);
  }
  len = srcLen;

  if (destLen < len * 2) {
    return err(ec_not_exist);
  }

  for (int i = 0; i < len; i++) {
    if (rev) {
      dest[len * 2 - 1 - i * 2] = hex_value_to_char(*p % 16);
      dest[len * 2 - 2 - i * 2] = hex_value_to_char(*p / 16);
    } else {
      dest[i * 2] = hex_value_to_char(*p / 16);
      dest[i * 2 + 1] = hex_value_to_char(*p % 16);
    }
    p++;
  }

  return ec_success;
}

err_t str2cbuf(const char src[],
               uint8_t rev,
               char dest[],
               size_t destLen)
{
  int len;
  const char *p;
  char ret;

  p = src;
  if (!p) {
    return err(ec_param_invalid);
  }
  len = strlen(p);

  if (destLen < len / 2) {
    return err(ec_length_leak);
  } else if (len % 2) {
    return err(ec_param_invalid);
  }

  for (int i = 0; i < len / 2; i++) {
    if (STR_HEX_VALID(*p) && STR_HEX_VALID(*(p + 1))) {
      ret = __asmbhex(hex_char_to_value(*p), hex_char_to_value(*(p + 1)));
    } else {
      return err(ec_param_invalid);
    }

    if (rev) {
      dest[i] = ret;
    } else {
      dest[len / 2 - 1 - i] = ret;
    }
    p += 2;
  }

  return ec_success;
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
    return err(ec_param_invalid);
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
        return err(ec_format);
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
        return err(ec_format);
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
    return err(ec_param_invalid);
  }

  if (base_type == BASE_DEC) {
    ret = input / base;
    remaining = input % base;
    while (1) {
      if (idx == length) {
        return err(ec_length_leak);
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
        return err(ec_length_leak);
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
    return err(ec_param_invalid);
  }
  return ec_success;
}
