/*************************************************************************
    > File Name: shash.c
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdlib.h>
#include <string.h>

#include "shash.h"
#include "logging.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
void *htb_next_value(htb_t *htb, int reset)
{
  static int offset = 0;
  if (!htb) {
    return NULL;
  }

  offset = (reset ? 0 : offset);

  for (; offset < (htb->multi * htb->base); offset++) {
    if (htb->data[offset]) {
      return htb->data[offset++];
    }
  }
  return NULL;
}

htb_t *new_htb(hash_func_t hfn,
               int base)
{
  htb_t *htb;
  htb = calloc(sizeof(htb_t), 1);
  htb->data = calloc(base, sizeof(void *));

  htb->hash = hfn;
  htb->base = base;
  htb->multi = 1;

  return htb;
}

void htb_destory(htb_t *htb)
{
  if (!htb) {
    return;
  }
  void *iter;
  htb_foreach(iter, htb)
  {
    free(iter);
  }
  free(htb);
  htb = NULL;
}

int htb_size(htb_t *htb)
{
  return htb->num;
}

void htb_expan(htb_t *htb,
               int reqsize)
{
  int pre = htb->multi;
  htb->multi = (reqsize + htb->base - 1) / htb->base;
  htb->data = realloc(htb->data, (htb->multi * htb->base) * sizeof(void *));
  memset(htb->data + sizeof(void *) * pre * htb->base,
         0,
         sizeof(void *) * (htb->multi - pre) * htb->base);
}

static inline void *__htb_value(htb_t *htb, const void *key)
{
  unsigned int index = htb->hash(key);
  if (index > htb->multi * htb->base) {
    return NULL;
  }
  return htb->data[index];
}

int htb_contains(htb_t *htb,
                 const void *key)
{
  return __htb_value(htb, key) ? 1 : 0;
}

int htb_insert(htb_t *htb,
               const void *key,
               const void *value)
{
  const void *v;
  unsigned int index = htb->hash(key);

  if (index > htb->multi * htb->base) {
    htb_expan(htb, index);
  }
  v = __htb_value(htb, key);
  if (v && v != value) {
    LOGE("htb insert an existing kv [index - %u]\n", index);
    return -1;
  } else if (v == value) {
    return 0;
  }

  htb->data[index] = (void *)value;
  htb->num++;
  return 0;
}

void htb_replace(htb_t *htb,
                 const void *key,
                 const void *value)
{
  void *v;
  unsigned int index = htb->hash(key);
  int freed = 0;

  if (index > htb->multi * htb->base) {
    htb_expan(htb, index);
  }
  v = __htb_value(htb, key);
  if (v && v != value) {
    free(v);
    freed = 1;
  } else if (v == value) {
    return;
  }

  htb->data[index] = (void *)value;
  if (!freed) {
    htb->num++;
  }
}

void htb_remove(htb_t *htb,
                const void *key)
{
  void *v = __htb_value(htb, key);
  if (v) {
    free(v);
    v = NULL;
  }
}
