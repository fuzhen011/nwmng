/*************************************************************************
    > File Name: shash.h
    > Author: Kevin
    > Created Time: 2019-12-17
    > Description:
 ************************************************************************/

#ifndef SHASH_H
#define SHASH_H
#ifdef __cplusplus
extern "C"
{
#endif
/******************************************************************
 * A simple hash design. There won't be more than one device in the same network
 * using the same address, because of this, it's needless to consider the
 * confliction of hash key.
 * ***************************************************************/
typedef unsigned int (*hash_func_t)(const void *key);

typedef struct {
  hash_func_t hash;
  int base;
  int multi;
  int num;
  void **data;
}htb_t;

#define htb_foreach(iter, htb)        \
  for (iter = htb_next_value(htb, 1); \
       iter != NULL;                  \
       iter = htb_next_value(htb, 0))

htb_t *new_htb(hash_func_t hfn,
               int base);
void htb_destory(htb_t *htb);
int htb_size(htb_t *htb);
void htb_expan(htb_t *htb,
               int reqsize);
int htb_contains(htb_t *htb,
                 const void *key);
int htb_insert(htb_t *htb,
               const void *key,
               const void *value);
void htb_replace(htb_t *htb,
                 const void *key,
                 const void *value);
void *htb_next_value(htb_t * htb, int reset);
void htb_remove(htb_t *htb,
                const void *key);
#ifdef __cplusplus
}
#endif
#endif //SHASH_H
