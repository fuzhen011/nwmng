/*************************************************************************
    > File Name: json_parser.c
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <assert.h>
#include <json.h>
#include <glib.h>

#include "cfgdb.h"
#include "generic_parser.h"
#include "json_object.h"
#include "json_parser.h"
#include "logging.h"
#include "utils.h"

/* Defines  *********************************************************** */
#ifndef JSON_ECHO_DBG
#define JSON_ECHO_DBG 2
#endif
/*
 * For each json file, the root holds the pointer to the result of
 * json_object_from_file, whenever it's needed to release the memory allocated
 * from json_object_from_file, call json_object_put with root.
 */
typedef struct {
  uint16_t id;
  json_object *nodes;
}sbn_t;

typedef struct {
  struct {
    char *fp;
    json_object *root;
  }prov;
  struct {
    char *fp;
    json_object *root;
    int subnet_num;
    sbn_t *subnets;
    json_object *backlog;
  }nw;
  struct {
    char *fp;
    json_object *root;
  }tmpl;
}json_cfg_t;

/* Global Variables *************************************************** */
#define DECLLOADER1(name) \
  static err_t _load_##name(json_object * obj, int cfg_fd, void *out)
#define DECLLOADER2(name) \
  static err_t __load_##name(json_object * obj, int cfg_fd, void *out)
#define DECLLOADER3(name) \
  static err_t ___load_##name(json_object * obj, int cfg_fd, void *out)

/* Static Variables *************************************************** */
static json_cfg_t jcfg = { 0 };

/* Static Functions Declaractions ************************************* */
/*
 * out holds the pointer to value, it can be a real value or a pointer
 */
typedef err_t (*__load_func_t)(json_object *obj,
                               int cfg_fd,
                               void *out);
#if 0
typedef struct {
  bool both;
  const char *key;
  __load_func_t loader;
  bool mustfornode;
}key_loader_t;
#endif

DECLLOADER1(ttl);
DECLLOADER1(pub);
DECLLOADER1(snb);
DECLLOADER1(txp);

#if 0
static const key_loader_t keyloaders[] = {
  { 1, STR_TTL, _load_ttl, 0 },
};
static const int klsize = sizeof(keyloaders) / sizeof(key_loader_t);
#endif
static const __load_func_t loaders[] = {
  _load_ttl,
  _load_pub,
  _load_snb,
  _load_txp,
};

/**
 * @defgroup single_key_load
 *
 * Below functions are used to load a single key-value pair in the json file
 * @{ */
static inline err_t uint8_loader(const char *v, uint8_t *out)
{
  ASSERT(v && out);
  if (ec_success != str2uint(v, strlen(v), out, sizeof(uint8_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }
  return ec_success;
}

static inline err_t uint16_loader(const char *v, uint16_t *out)
{
  ASSERT(v && out);
  if (ec_success != str2uint(v, strlen(v), out, sizeof(uint16_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }
  return ec_success;
}
static inline err_t uint32_loader(const char *v, uint32_t *out)
{
  ASSERT(v && out);
  if (ec_success != str2uint(v, strlen(v), out, sizeof(uint32_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }
  return ec_success;
}

static inline err_t uint16list_loader(const char *v, uint16list_t *out)
{
  return ec_success;
}

static inline uint8_t **pttl_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.ttl);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->ttl);
  }
  assert(0);
}

static inline publication_t **ppub_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.pub);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->pub);
  }
  assert(0);
}

static inline txparam_t **ptxp_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.net_txp);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->net_txp);
  }
  assert(0);
}

static inline uint8_t **psnb_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.snb);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->snb);
  }
  assert(0);
}

static err_t _load_pub(json_object *obj,
                       int cfg_fd,
                       void *out)
{
  err_t e = ec_success;
  publication_t **p = ppub_from_fd(cfg_fd, out);
  json_object *o;

  if (!json_object_object_get_ex(obj, STR_PUB, &o)) {
    goto free;
    e = err(e);
  }
  if (!*p) {
    *p = calloc(sizeof(publication_t), 1);
  }

#if (JSON_ECHO_DBG == 0)
  LOGD("pub obj --- %s", json_object_to_json_string(o));
  exit(0);
#endif
  const char *v;
  json_object_object_foreach(o, key, val){
    if (!strcmp(STR_ADDR, key)) {
      v = json_object_get_string(val);
      if (ec_success != (e = uint16_loader(v, &(*p)->addr))) {
        goto free;
      }
    } else if (!strcmp(STR_APPKEY, key)) {
      v = json_object_get_string(val);
      if (ec_success != (e = uint16_loader(v, &(*p)->aki))) {
        goto free;
      }
    } else if (!strcmp(STR_PERIOD, key)) {
      v = json_object_get_string(val);
      if (ec_success != (e = uint32_loader(v, &(*p)->period))) {
        goto free;
      }
    } else if (!strcmp(STR_TTL, key)) {
      v = json_object_get_string(val);
      if (ec_success != (e = uint8_loader(v, &(*p)->ttl))) {
        goto free;
      }
    } else if (!strcmp(STR_TXP, key)) {
      json_object *tmp;
      if (json_object_object_get_ex(val, STR_CNT, &tmp)) {
        v = json_object_get_string(tmp);
        if (ec_success != (e = uint8_loader(v, &(*p)->txp.cnt))) {
          goto free;
        }
      }
      if (json_object_object_get_ex(val, STR_INTV, &tmp)) {
        v = json_object_get_string(tmp);
        if (ec_success != (e = uint16_loader(v, &(*p)->txp.intv))) {
          goto free;
        }
      }
      /* TODO: sanity check, if invalid, memset to 0 */
    }
  }
  return ec_success;

  free:
  free(*p);
  *p = NULL;
  return e;
}

static err_t _load_txp(json_object *obj,
                       int cfg_fd,
                       void *out)
{
  err_t e = ec_success;
  txparam_t **p = ptxp_from_fd(cfg_fd, out);
  json_object *o;

  if (!json_object_object_get_ex(obj, STR_TXP, &o)) {
    goto free;
    e = err(e);
  }
  if (!*p) {
    *p = calloc(sizeof(txparam_t), 1);
  }

#if (JSON_ECHO_DBG == 2)
  LOGD("txp obj --- %s", json_object_to_json_string(o));
#endif
  const char *v;
  json_object *tmp;
  if (json_object_object_get_ex(o, STR_CNT, &tmp)) {
    v = json_object_get_string(tmp);
    if (ec_success != (e = uint8_loader(v, &(*p)->cnt))) {
      goto free;
    }
  }
  if (json_object_object_get_ex(o, STR_INTV, &tmp)) {
    v = json_object_get_string(tmp);
    if (ec_success != (e = uint16_loader(v, &(*p)->intv))) {
      goto free;
    }
  }
  return ec_success;

  free:
  free(*p);
  *p = NULL;
  return e;
}

static err_t _load_ttl(json_object *obj,
                       int cfg_fd,
                       void *out)
{
  err_t e = ec_success;
  uint8_t **p = pttl_from_fd(cfg_fd, out);
  json_object *o;

  if (!json_object_object_get_ex(obj, STR_TTL, &o)) {
    goto free;
    e = err(e);
  }
#if (JSON_ECHO_DBG == 1)
  LOGD("ttl obj --- %s", json_object_to_json_string(o));
  LOGD("\n");
#endif
  const char *v = json_object_get_string(o);
  if (!*p) {
    *p = malloc(sizeof(uint8_t));
  }
  if (ec_success != (e = uint8_loader(v, *p))) {
    goto free;
  }
  return ec_success;

  free:
  free(*p);
  *p = NULL;
  return e;
}

static err_t _load_snb(json_object *obj,
                       int cfg_fd,
                       void *out)
{
  err_t e = ec_success;
  uint8_t **p = psnb_from_fd(cfg_fd, out);
  json_object *o;

  if (!json_object_object_get_ex(obj, STR_SNB, &o)) {
    goto free;
    e = err(e);
  }
#if (JSON_ECHO_DBG == 1)
  LOGD("snb obj --- %s", json_object_to_json_string(o));
  LOGD("\n");
#endif
  const char *v = json_object_get_string(o);
  if (!*p) {
    *p = malloc(sizeof(uint8_t));
  }
  if (ec_success != (e = uint8_loader(v, *p))) {
    goto free;
  }
  return ec_success;

  free:
  free(*p);
  *p = NULL;
  return e;
}

static err_t load_to_tmpl_item(json_object *obj,
                               tmpl_t *tmpl)
{
  err_t e;
#if (JSON_ECHO_DBG == 1)
  LOGD("tmpl item --- %s\n", json_object_to_json_string(obj));
  LOGD("\n");
#endif
  for (int i = 0; i < sizeof(loaders) / sizeof(__load_func_t); i++) {
    e = loaders[i](obj, TEMPLATE_FILE, tmpl);
    if (e != ec_success) {
      LOGE("Load %dth object failed.\n", i);
      elog(e);
    }
  }
  return ec_success;
}

/**  @} */

static inline char **fp_from_fd(int fd)
{
  return fd == PROV_CFG_FILE ? &jcfg.prov.fp
         : fd == NW_NODES_CFG_FILE ? &jcfg.nw.fp
         : fd == TEMPLATE_FILE ? &jcfg.tmpl.fp : NULL;
}

static inline json_object **root_from_fd(int fd)
{
  return fd == PROV_CFG_FILE ? &jcfg.prov.root
         : fd == NW_NODES_CFG_FILE ? &jcfg.nw.root
         : fd == TEMPLATE_FILE ? &jcfg.tmpl.root : NULL;
}

/**
 * @brief open_json_file - close the current root and reload it with the file
 * content, it also fill all the json_object in the struct
 *
 * @param cfg_fd -
 *
 * @return
 */
static err_t open_json_file(int cfg_fd)
{
  err_t e = ec_success;;
  json_object **root = root_from_fd(cfg_fd);
  char **fp;
  fp = fp_from_fd(cfg_fd);
  json_close(cfg_fd);
  if (!*fp) {
    return err(ec_param_invalid);
  }
  *root = json_object_from_file(*fp);
  if (!*root) {
    LOGE("json-c failed to open %s\n", *fp);
    return err(ec_json_open);
  }

  if (cfg_fd == NW_NODES_CFG_FILE) {
    /* Load the subnets, which fills the refid and the json_object nodes */
    json_object_object_foreach(*root, key, val){
      if (!strcmp(STR_SUBNETS, key)) {
        jcfg.nw.subnet_num = json_object_array_length(val);
        jcfg.nw.subnets = calloc(jcfg.nw.subnet_num, sizeof(sbn_t));
        for (int i = 0; i < jcfg.nw.subnet_num; i++) {
          json_object* tmp;
          json_object *n = json_object_array_get_idx(val, i);
          if (json_object_object_get_ex(n, STR_REFID, &tmp)) {
            const char *v = json_object_get_string(tmp);
            if (ec_success != str2uint(v, strlen(v), &jcfg.nw.subnets[i].id, sizeof(uint16_t))) {
              LOGE("STR to UINT error\n");
            }
          }
          if (!json_object_object_get_ex(n, STR_NODES, &jcfg.nw.subnets[i].nodes)) {
            LOGE("No Nodes node in the json file\n");
            e = err(ec_json_format);
            goto out;
          }
        }
      } else if (!strcmp(STR_BACKLOG, key)) {
        jcfg.nw.backlog = val;
      }
    }
  }
  /* else if (cfg_fd == TEMPLATE_FILE) { */
  /* json_object *n = json_object_array_get_idx(val, i); */
  /* } */

  out:
  if (ec_success != e) {
    json_close(cfg_fd);
  } else {
    LOGD("%s file opened\n", *fp);
  }
  return e;
}

static err_t new_json_file(int cfg_fd)
{
  /* TODO */
  return ec_success;
}

#define json_array_foreach(i, n, obj)       \
  size_t n = json_object_array_length(obj); \
  for (int i = 0; i < n; i++)

static err_t load_template(void)
{
  json_object *n, *ptmpl;
  err_t e;
  bool add = false;
  if (!jcfg.tmpl.root) {
    return err(ec_json_open);
  }
  if (!json_object_object_get_ex(jcfg.tmpl.root, STR_TEMPLATES, &ptmpl)) {
    return err(ec_json_format);
  }

  json_array_foreach(i, num, ptmpl)
  {
    json_object *tmp;
    n = json_object_array_get_idx(ptmpl, i);
    if (!json_object_object_get_ex(n, STR_REFID, &tmp)) {
      /* No reference ID, ignore it */
      continue;
    }
    const char *v = json_object_get_string(tmp);
    uint16_t refid;
    if (ec_success != str2uint(v, strlen(v), &refid, sizeof(uint16_t))) {
      LOGE("STR to UINT error\n");
      continue;
    }
    tmpl_t *t = cfgdb_tmpl_get(refid);
    if (!t) {
      t = (tmpl_t *)calloc(sizeof(tmpl_t), 1);
      assert(t);
      add = true;
    }
    e = load_to_tmpl_item(n, t);
    elog(e);
    if (add) {
      if (e == ec_success) {
        t->refid = refid;
        EC(ec_success, cfgdb_tmpl_add(t));
      } else {
        free(t);
      }
    }
  }
  return ec_success;
}

static err_t load_json_file(int cfg_fd,
                            bool clrctlfls,
                            void *out)
{
  /* TODO */
  if (cfg_fd == TEMPLATE_FILE) {
    return load_template();
  }
  return ec_success;
}

void json_close(int cfg_fd)
{
  json_object **root = root_from_fd(cfg_fd);
  if (!*root) {
    return;
  }
  json_object_put(*root);
  if (cfg_fd == NW_NODES_CFG_FILE) {
    SAFE_FREE(jcfg.nw.subnets);
  }
  *root = NULL;
}

err_t json_cfg_open(int cfg_fd,
                    const char *filepath,
                    unsigned int flags,
                    void *out)
{
  int tmp;
  err_t ret = ec_success;
  char **fp;

  if (cfg_fd > TEMPLATE_FILE || cfg_fd < PROV_CFG_FILE) {
    return err(ec_param_invalid);
  }
  fp = fp_from_fd(cfg_fd);

  /* Ensure the fp is not NULL */
  if (!(flags & FL_CLR_CTLFS)) {
    if (!filepath) {
      if (!(*fp)) {
        return err(ec_param_invalid);
      }
    } else {
      if (*fp) {
        free(*fp);
        *fp = NULL;
      }
      *fp = malloc(strlen(filepath) + 1);
      strcpy(*fp, filepath);
      (*fp)[strlen(filepath)] = '\0';
    }
  } else if (!(*fp)) {
    return err(ec_param_invalid);
  }
  assert(*fp);

  /*
   * If need to turncate the file or the file doesn't exist, need to create,
   * do it
   */
  tmp = access(*fp, F_OK);
  if (cfg_fd == TEMPLATE_FILE) {
    if (tmp == -1) {
      return err(ec_not_exist);
    }
  } else {
    if (-1 == tmp) {
      if (!(flags & FL_CREATE)) {
        ret = err(ec_not_exist);
        goto fail;
      }
      if (ec_success != (ret = new_json_file(cfg_fd))) {
        goto fail;
      }
    } else {
      if (flags & FL_TRUNC) {
        if (ec_success != (ret = new_json_file(cfg_fd))) {
          goto fail;
        }
      }
    }
  }

  if (ec_success != (ret = open_json_file(cfg_fd))) {
    goto fail;
  }

  if (ec_success != (ret = load_json_file(cfg_fd,
                                          !!(flags & FL_CLR_CTLFS),
                                          out))) {
    goto fail;
  }

  fail:
  if (ec_success != ret) {
    /* TODO: Clean work need? */
    /* jsonConfigDeinit(); */
    LOGE("JSON[%s] Open failed\n", *fp);
    elog(ret);
  }

  return ret;
#if 0

  if (rootPtr) {
    jsonConfigClose();
  }

  if (data) {
    *data = pNetConfig;
  }
  return ec_success;

#endif
}
