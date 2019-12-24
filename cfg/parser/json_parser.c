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
#include <json.h>
#include <json_util.h>
#include <glib.h>

#include "cfgdb.h"
#include "generic_parser.h"
#include "json_object.h"
#include "json_parser.h"
#include "logging.h"
#include "utils.h"

/* Defines  *********************************************************** */
#define JSON_ECHO(msg, obj)              \
  do {                                   \
    LOGD("%s Item --- %s\n",             \
         (msg),                          \
         json_object_to_json_string_ext( \
           (obj),                        \
           JSON_C_TO_STRING_PRETTY));    \
  } while (0)

#ifndef JSON_ECHO_DBG
#define JSON_ECHO_DBG 2
#endif

#define json_array_foreach(i, n, obj)       \
  size_t n = json_object_array_length(obj); \
  for (int i = 0; i < n; i++)

#define WHOLE_WORD(x) "\"" x "\""

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
  bool autoflush;
  char *fp;
  json_object *root;
}cfg_general_t;

typedef struct {
  struct {
    cfg_general_t gen;
  }prov;
  struct {
    cfg_general_t gen;
    int subnet_num;
    sbn_t *subnets;
    json_object *backlog;
  }nw;
  struct {
    cfg_general_t gen;
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
/* Used for both template and node */
DECLLOADER1(ttl);
DECLLOADER1(pub);
DECLLOADER1(snb);
DECLLOADER1(txp);
DECLLOADER1(bindings);
DECLLOADER1(sublist);

/* Used only for node */
DECLLOADER1(tmpl);

static const __load_func_t loaders[] = {
  /* Used for both template and node */
  _load_ttl,
  _load_pub,
  _load_snb,
  _load_txp,
  _load_bindings,
  _load_sublist,
  /* Used only for node */
  _load_tmpl,
  /* Used only for provself */
};

static const int tmpl_loader_end = 6;
static const int node_loader_end = 7;

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
  } else if (cfg_fd == PROV_CFG_FILE) {
    return &((provcfg_t *)out)->ttl;
  }
  ASSERT(0);
}

static inline publication_t **ppub_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.pub);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->pub);
  }
  ASSERT(0);
}

static inline txparam_t **ptxp_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.net_txp);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->net_txp);
  } else if (cfg_fd == PROV_CFG_FILE) {
    return &((provcfg_t *)out)->net_txp;
  }

  ASSERT(0);
}

static inline uint8_t **psnb_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.snb);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->snb);
  }
  ASSERT(0);
}

static inline uint16list_t **pbindings_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.bindings);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->bindings);
  }
  ASSERT(0);
}

static inline uint16list_t **psublist_from_fd(int cfg_fd, void *out)
{
  if (cfg_fd == NW_NODES_CFG_FILE) {
    return (&((node_t *)out)->config.sublist);
  } else if (cfg_fd == TEMPLATE_FILE) {
    return (&((tmpl_t *)out)->sublist);
  }
  ASSERT(0);
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

#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Pub", o);
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

#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("TxP", o);
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

static err_t _load_timeout(json_object *obj,
                           int cfg_fd,
                           void *out)
{
  err_t e = ec_success;
  json_object *o;
  provcfg_t *prov = (provcfg_t *)out;

  if (cfg_fd != PROV_CFG_FILE) {
    e = err(ec_param_invalid);
    goto free;
  }

  if (!json_object_object_get_ex(obj, STR_TIMEOUT, &o)) {
    goto free;
    e = err(e);
  }
  if (!prov->timeout) {
    prov->timeout = calloc(1, sizeof(timeout_t));
  }

#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Timeout", o);
#endif
  const char *v;
  json_object *tmp;
  if (json_object_object_get_ex(o, STR_TIMEOUT_NORMAL, &tmp)) {
    v = json_object_get_string(tmp);
    if (ec_success != (e = uint32_loader(v, &prov->timeout->normal))) {
      goto free;
    }
  }
  if (json_object_object_get_ex(o, STR_TIMEOUT_LPN, &tmp)) {
    v = json_object_get_string(tmp);
    if (ec_success != (e = uint32_loader(v, &prov->timeout->lpn))) {
      goto free;
    }
  }
  return ec_success;

  free:
  if (prov->timeout) {
    free(prov->timeout);
    prov->timeout = NULL;
  }
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
  JSON_ECHO("TTL", o);
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
  JSON_ECHO("SNB", o);
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

static err_t __load_uint16list(json_object *o,
                               uint16list_t **p)
{
  err_t e;
  if (json_type_array != json_object_get_type(o)) {
    e =  err(ec_format);
    goto free;
  }
  if (!*p) {
    *p = calloc(sizeof(uint16list_t), 1);
  }

  const char *v;
  int len = json_object_array_length(o);
  if (!(*p)->data) {
    (*p)->len = len;
    (*p)->data = calloc(len, sizeof(uint16_t));
  } else if ((*p)->len != len) {
    (*p)->len = len;
    (*p)->data = realloc((*p)->data, len * sizeof(uint16_t));
    memset((*p)->data, 0, len * sizeof(uint16_t));
  }

  json_array_foreach(i, n, o)
  {
    json_object *tmp;
    tmp = json_object_array_get_idx(o, i);
    v = json_object_get_string(tmp);
    if (ec_success != (e = uint16_loader(v, &(*p)->data[i]))) {
      goto free;
    }
  }
  return ec_success;

  free:
  if (*p) {
    if ((*p)->data) {
      free((*p)->data);
    }
    free(*p);
    *p = NULL;
  }
  return e;
}

static err_t _load_bindings(json_object *obj,
                            int cfg_fd,
                            void *out)
{
  err_t e = ec_success;
  uint16list_t **p = pbindings_from_fd(cfg_fd, out);
  json_object *o;

  if (!json_object_object_get_ex(obj, STR_BIND, &o)) {
    goto free;
    e = err(e);
  }
#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Bindings", o);
#endif
  if (ec_success != (e = __load_uint16list(o, p))) {
    return e;
  }

  return ec_success;

  free:
  if (*p) {
    if ((*p)->data) {
      free((*p)->data);
    }
    free(*p);
    *p = NULL;
  }
  return e;
}

static err_t _load_sublist(json_object *obj,
                           int cfg_fd,
                           void *out)
{
  err_t e = ec_success;
  uint16list_t **p = psublist_from_fd(cfg_fd, out);
  json_object *o;

  if (!json_object_object_get_ex(obj, STR_SUB, &o)) {
    goto free;
    e = err(e);
  }
#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Sublist", o);
#endif
  if (ec_success != (e = __load_uint16list(o, p))) {
    return e;
  }
  return ec_success;

  free:
  if (*p) {
    if ((*p)->data) {
      free((*p)->data);
    }
    free(*p);
    *p = NULL;
  }
  return e;
}

/**
 * @brief copy_tmpl_to_node - copy the configuration in template to the node
 * only if the field in node is not set.
 *
 * @param t - tmplate
 * @param n - node
 */
static void copy_tmpl_to_node(const tmpl_t *t,
                              node_t *n)
{
  ASSERT(t && n);
  if (t->ttl && !n->config.ttl) {
    alloc_copy(&n->config.ttl, t->ttl, sizeof(uint8_t));
  }
  if (t->sublist && !n->config.sublist) {
    alloc_copy_u16list(&n->config.sublist, t->sublist);
  }
  if (t->snb && !n->config.snb) {
    alloc_copy(&n->config.snb, t->snb, sizeof(uint8_t));
  }
  if (t->relay_txp && !n->config.features.relay_txp) {
    alloc_copy((uint8_t **)&n->config.features.relay_txp, t->relay_txp, sizeof(txparam_t));
  }
  if (t->pub && !n->config.pub) {
    alloc_copy((uint8_t **)&n->config.pub, t->pub, sizeof(publication_t));
  }
  if (t->net_txp && !n->config.net_txp) {
    alloc_copy((uint8_t **)&n->config.net_txp, t->net_txp, sizeof(txparam_t));
  }
  if (t->bindings && !n->config.bindings) {
    alloc_copy_u16list(&n->config.bindings, t->bindings);
  }
}

static err_t _load_tmpl(json_object *obj,
                        int cfg_fd,
                        void *out)
{
  uint16_t tmplid = 0;
  json_object *tmp;
  const char *tmplid_str;
  tmpl_t *t;

  ASSERT(cfg_fd == NW_NODES_CFG_FILE);

  json_object_object_get_ex(obj, STR_TMPL, &tmp);
  tmplid_str = json_object_get_string(tmp);

  if (ec_success != str2uint(tmplid_str, strlen(tmplid_str), &tmplid, sizeof(uint8_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }

  t = cfgdb_tmpl_get(tmplid);
  if (!t) {
    return ec_success;
  }
  copy_tmpl_to_node(t, out);
  return ec_success;
}

static err_t load_to_tmpl_item(json_object *obj,
                               tmpl_t *tmpl)
{
  err_t e;
#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Template", obj);
#endif
  for (int i = 0; i < tmpl_loader_end; i++) {
    e = loaders[i](obj, TEMPLATE_FILE, tmpl);
    if (e != ec_success) {
      LOGE("Load %dth object failed.\n", i);
      elog(e);
    }
  }
  return ec_success;
}

static err_t load_to_node_item(json_object *obj,
                               node_t *node)
{
  err_t e;
#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Node", obj);
#endif
  for (int i = 0; i < node_loader_end; i++) {
    e = loaders[i](obj, NW_NODES_CFG_FILE, node);
    if (e != ec_success) {
      LOGE("Load %dth object failed.\n", i);
      elog(e);
    }
  }
  return ec_success;
}

/**  @} */

static inline cfg_general_t *gen_from_fd(int fd)
{
  return fd == PROV_CFG_FILE ? &jcfg.prov.gen
         : fd == NW_NODES_CFG_FILE ? &jcfg.nw.gen
         : fd == TEMPLATE_FILE ? &jcfg.tmpl.gen : NULL;
}

/**
 * @brief open_json_file - close the current root and reload it with the file
 * content, it also fill all the json_object in the struct
 *
 * @param cfg_fd -
 *
 * @return
 */
static err_t open_json_file(int cfg_fd, bool autoflush)
{
  err_t e = ec_success;;
  cfg_general_t *gen;

  gen = gen_from_fd(cfg_fd);
  json_cfg_close(cfg_fd);
  if (!gen) {
    return err(ec_param_invalid);
  }
  gen->root = json_object_from_file(gen->fp);
  gen->autoflush = autoflush;

  if (cfg_fd == NW_NODES_CFG_FILE) {
    /* Load the subnets, which fills the refid and the json_object nodes */
    json_object_object_foreach(gen->root, key, val){
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

  out:
  if (ec_success != e) {
    json_cfg_close(cfg_fd);
  } else {
    LOGD("%s file opened\n", gen->fp);
  }
  return e;
}

static err_t new_json_file(int cfg_fd)
{
  /* TODO */
  return ec_success;
}

static err_t load_template(void)
{
  json_object *n, *ptmpl;
  err_t e;
  bool add = false;
  if (!jcfg.tmpl.gen.root) {
    return err(ec_json_open);
  }
  if (!json_object_object_get_ex(jcfg.tmpl.gen.root, STR_TEMPLATES, &ptmpl)) {
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
      ASSERT(t);
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

static const char *mandatory_fields[] = {
  WHOLE_WORD(STR_UUID),
  WHOLE_WORD(STR_ADDR),
  WHOLE_WORD(STR_RMORBL),
  WHOLE_WORD(STR_DONE),
  WHOLE_WORD(STR_ERRBITS),
  NULL
};

static bool _node_valid_check(json_object *obj)
{
  ASSERT(obj);
  const char *v = json_object_get_string(obj);
  for (const char **c = mandatory_fields;
       *c != NULL;
       c++) {
    if (!strstr(v, *c)) {
      return false;
    }
  }
  return true;
}

static err_t load_key(json_object *obj,
                      meshkey_t *key)
{
  json_object *n;
  const char *v;

#if (JSON_ECHO_DBG == 1)
  JSON_ECHO("Key", obj);
#endif
  json_object_object_get_ex(obj, STR_REFID, &n);
  v = json_object_get_string(n);
  if (ec_success != str2uint(v,
                             strlen(v),
                             &key->refid,
                             sizeof(uint16_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }
  json_object_object_get_ex(obj, STR_ID, &n);
  v = json_object_get_string(n);
  if (ec_success != str2uint(v,
                             strlen(v),
                             &key->id,
                             sizeof(uint16_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }
  json_object_object_get_ex(obj, STR_VALUE, &n);
  v = json_object_get_string(n);
  if (ec_success != str2cbuf(v, 0, (char *)key->val, 16)) {
    LOGE("STR to CBUF error\n");
    return err(ec_json_format);
  }

  json_object_object_get_ex(obj, STR_DONE, &n);
  v = json_object_get_string(n);
  if (ec_success != str2uint(v,
                             strlen(v),
                             &key->done,
                             sizeof(uint8_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }
  return ec_success;
}

static err_t load_provself(void)
{
  /* NOTE: Only first subnet will be loaded */
  json_object *n, *primary_subnet, *appkeys;
  err_t e;
  provcfg_t *provcfg = get_provcfg();
  int appkey_num;
  const char *v;

  if (!jcfg.prov.gen.root) {
    return err(ec_json_open);
  }

  json_object_object_get_ex(jcfg.prov.gen.root, STR_ADDR, &n);
  v = json_object_get_string(n);
  if (ec_success != str2uint(v, strlen(v), &provcfg->addr, sizeof(uint16_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }

  json_object_object_get_ex(jcfg.prov.gen.root, STR_SYNC_TIME, &n);
  v = json_object_get_string(n);
  if (ec_success != str2uint(v, strlen(v), &provcfg->sync_time, sizeof(uint32_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }

  json_object_object_get_ex(jcfg.prov.gen.root, STR_IVI, &n);
  v = json_object_get_string(n);
  if (ec_success != str2uint(v, strlen(v), &provcfg->ivi, sizeof(uint32_t))) {
    LOGE("STR to UINT error\n");
    return err(ec_json_format);
  }

  _load_ttl(jcfg.prov.gen.root, PROV_CFG_FILE, provcfg);
  _load_txp(jcfg.prov.gen.root, PROV_CFG_FILE, provcfg);
  _load_timeout(jcfg.prov.gen.root, PROV_CFG_FILE, provcfg);

  /* Load Primary Subnet */
  if (!json_object_object_get_ex(jcfg.prov.gen.root, STR_SUBNETS, &n)) {
    return err(ec_json_format);
  }
  if (json_object_array_length(n)) {
    /* Turncate to only load the first one */
    provcfg->subnet_num = 1;
  }

  primary_subnet = json_object_array_get_idx(n, 0);
  json_object_object_get_ex(primary_subnet, STR_APPKEY, &appkeys);
  appkey_num = json_object_array_length(appkeys);
  if (provcfg->subnets && provcfg->subnets->appkey_num < appkey_num) {
    provcfg->subnets = realloc(provcfg->subnets,
                               sizeof(subnet_t) + appkey_num * sizeof(meshkey_t));
    memset(provcfg->subnets, 0, sizeof(subnet_t) + appkey_num * sizeof(meshkey_t));
  } else if (!provcfg->subnets) {
    provcfg->subnets = calloc(1, sizeof(subnet_t) + appkey_num * sizeof(meshkey_t));
  }
  provcfg->subnets[0].appkey_num = appkey_num;

  if (ec_success != (e = load_key(primary_subnet, &provcfg->subnets[0].netkey))) {
    goto free;
  }

  json_array_foreach(i, num, appkeys)
  {
    json_object *tmp;
    tmp = json_object_array_get_idx(appkeys, i);
    if (ec_success != (e = load_key(tmp, &provcfg->subnets[0].appkey[i]))) {
      goto free;
    }
  }
  free:
  if (ec_success != e) {
    if (provcfg->subnets) {
      free(provcfg->subnets);
      provcfg->subnet_num = 0;
    }
  }
  return e;
}

static err_t load_nodes(void)
{
  /* NOTE: Only first subnet will be loaded */
  json_object *n, *pnode;
  err_t e;
  bool add = false;
  if (!jcfg.nw.subnets
      || !jcfg.nw.subnets[0].nodes
      || !jcfg.nw.gen.root) {
    return err(ec_json_open);
  }
  pnode = jcfg.nw.subnets[0].nodes;

  json_array_foreach(i, num, pnode)
  {
    json_object *tmp;
    n = json_object_array_get_idx(pnode, i);
    if (!_node_valid_check(n)) {
      LOGE("Node[%d] invalid, pass.\n", i);
      continue;
    }

    if (!json_object_object_get_ex(n, STR_UUID, &tmp)) {
      /* No reference ID, ignore it */
      continue;
    }
    /*
     * Check if it's already provisioned to know which hash table to find the
     * node
     */
    const char *addr_str, *uuid_str, *rmbl_str, *done_str, *err_str;
    uint8_t uuid[16] = { 0 };
    uint32_t errbits;
    uint16_t addr;
    uint8_t rmbl, done;
    node_t *t;

    json_object_object_get_ex(n, STR_ADDR, &tmp);
    addr_str = json_object_get_string(tmp);

    json_object_object_get_ex(n, STR_UUID, &tmp);
    uuid_str = json_object_get_string(tmp);

    json_object_object_get_ex(n, STR_RMORBL, &tmp);
    rmbl_str = json_object_get_string(tmp);

    json_object_object_get_ex(n, STR_DONE, &tmp);
    done_str = json_object_get_string(tmp);

    json_object_object_get_ex(n, STR_ERRBITS, &tmp);
    err_str = json_object_get_string(tmp);

    if (ec_success != str2cbuf(uuid_str, 0, (char *)uuid, 16)) {
      LOGE("STR to CBUF error\n");
      continue;
    }
    if (ec_success != str2uint(err_str, strlen(err_str), &errbits, sizeof(uint32_t))) {
      LOGE("STR to UINT error\n");
      continue;
    }
    if (ec_success != str2uint(addr_str, strlen(addr_str), &addr, sizeof(uint16_t))) {
      LOGE("STR to UINT error\n");
      continue;
    }
    if (ec_success != str2uint(rmbl_str, strlen(rmbl_str), &rmbl, sizeof(uint8_t))) {
      LOGE("STR to UINT error\n");
      continue;
    }
    if (ec_success != str2uint(done_str, strlen(done_str), &done, sizeof(uint8_t))) {
      LOGE("STR to UINT error\n");
      continue;
    }

    if (addr) {
      t = cfgdb_node_get(addr);
    } else {
      t = cfgdb_unprov_dev_get((const uint8_t *)uuid);
    }
    if (!t) {
      t = (node_t *)calloc(sizeof(node_t), 1);
      ASSERT(t);
      add = true;
    }

    e = load_to_node_item(n, t);
    elog(e);

    if (add) {
      if (e == ec_success) {
        t->addr = addr;
        memcpy(t->uuid, uuid, 16);
        t->done = done;
        t->rmorbl = rmbl;
        t->err = errbits;
        EC(ec_success, cfgdb_add(t));
      } else {
        free(t);
      }
    }
  }
  return ec_success;
}

static err_t load_json_file(int cfg_fd,
                            bool clrctlfls)
{
  /* TODO */
  if (cfg_fd == TEMPLATE_FILE) {
    return load_template();
  } else if (cfg_fd == NW_NODES_CFG_FILE) {
    return load_nodes();
  } else if (cfg_fd == PROV_CFG_FILE) {
    return load_provself();
  }
  return err(ec_param_invalid);
}

void json_cfg_close(int cfg_fd)
{
  cfg_general_t *gen = gen_from_fd(cfg_fd);
  if (!gen->root) {
    return;
  }
  json_object_put(gen->root);
  if (cfg_fd == NW_NODES_CFG_FILE) {
    SAFE_FREE(jcfg.nw.subnets);
  }
  gen->root = NULL;
  LOGM("%s file closed.\n", gen->fp);
}

err_t json_cfg_open(int cfg_fd,
                    const char *filepath,
                    unsigned int flags,
                    void *out)
{
  int tmp;
  err_t ret = ec_success;
  cfg_general_t *gen;

  if (cfg_fd > TEMPLATE_FILE || cfg_fd < PROV_CFG_FILE) {
    return err(ec_param_invalid);
  }
  gen = gen_from_fd(cfg_fd);

  /* Ensure the fp is not NULL */
  if (!(flags & FL_CLR_CTLFS)) {
    if (!filepath) {
      if (!(gen->fp)) {
        return err(ec_param_invalid);
      }
    } else {
      if (gen->fp) {
        free(gen->fp);
        gen->fp = NULL;
      }
      gen->fp = malloc(strlen(filepath) + 1);
      strcpy(gen->fp, filepath);
      gen->fp[strlen(filepath)] = '\0';
    }
  } else if (!gen->fp) {
    return err(ec_param_invalid);
  }
  ASSERT(gen->fp);

  /*
   * If need to turncate the file or the file doesn't exist, need to create,
   * do it
   */
  tmp = access(gen->fp, F_OK);
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

  if (ec_success != (ret = open_json_file(cfg_fd, 1))) {
    goto fail;
  }

  if (ec_success != (ret = load_json_file(cfg_fd,
                                          !!(flags & FL_CLR_CTLFS)))) {
    goto fail;
  }

  fail:
  if (ec_success != ret) {
    /* TODO: Clean work need? */
    /* jsonConfigDeinit(); */
    LOGE("JSON[%s] Open failed\n", gen->fp);
    elog(ret);
  }

  return ret;
#if 0
  if (rootPtr) {
    jsonConfigClose();
  }
#endif
}

err_t json_cfg_flush(int cfg_fd)
{
  cfg_general_t *gen;
  if (cfg_fd > TEMPLATE_FILE || cfg_fd < PROV_CFG_FILE) {
    return err(ec_param_invalid);
  }
  gen = gen_from_fd(cfg_fd);
  if (!gen || !gen->root || !gen->fp) {
    return err(ec_json_null);
  }

  if (-1 == json_object_to_file_ext(gen->fp, gen->root, JSON_C_TO_STRING_PRETTY)) {
#if __APPLE__ == 1
    LOGE("json file save error, reason[%s]\n",
         json_util_get_last_err());
#else
    LOGE("json file save error, reason[Not available in this platform]\n");
#endif
    return err(ec_json_save);
  }
  return ec_success;
}
/*
 * What fields can be modified?
 *
 * - In node scope [key]
 *   - Error bitmap [Err]
 *   - Address [Address]
 *   - Remove or blacklist [RM_Blacklist]
 *   - Done [Done]
 * - In Self config scope
 *   - Address
 *   - Sync time
 *   - IVI
 *   - Id*
 *   - Value*
 *   - Done*
 *   - Pub groups
 *   - Sub groups
 * Where * means it could duplicate in multiple fields, in this case, it can be
 * used for both network key and application key, so it's insufficient to
 * address them without additional field.
 *
 * How to address a specific key-value pair in the json file?
 * With the pair [cfg_fd, wrtype(opcode), key, value]
 *
 * - In node scope:
 *   1> find the node by UUID
 *   2> find the key-value pair by the key
 *   3> modify the value
 *
 * - In Self config scope, since there are overlap keys used in network and
 *   application keys, additional field key(s) is needed:
 *   1> Check if field key is provided? Yes means to modify the field in
 *   subnets, no means modify the field in current level.
 *
 *   - Modify the current level
 *     - Find the key-value pair by key and modify the value
 *
 *   - Searching down still
 *     - if the additional field key IS NOT provided, then iterate all the
 *       network key by matching the RefId, then modify the value
 *     - if the additional field key IS provided, then iterate all the
 *       application key by matching the RefId, then modify the value
 *
 * So x parameters are needed to modify the target value in general.
 *
 * - field_key - used to address the field in array for searching key-value
 *   pair, e.g. UUID for searching node, refId for network key.
 * - level_indicator - used to indicate how many levels need to go into to find
 *   to match the field key, now only used for appkey
 * - key - used to address the key-value pair in above field.
 * - value - new value to set.
 *
 *
 */

/* NOTE: It's the caller's responsbility to make sure no same UUID being added
 * for multiple times */
static void __kv_replace(json_object *obj,
                         const void *key,
                         void *value)
{
  json_object_object_del(obj, key);
  json_object_object_add(obj, key, json_object_new_string((char *)value));
}

static json_object *find_node(const uint8_t *uuid,
                              bool reload)
{
  if (!uuid) {
    return NULL;
  }
  if (reload) {
    load_json_file(NW_NODES_CFG_FILE, 0);
  }
  if (!jcfg.nw.subnets[0].nodes) {
    return NULL;
  }

  json_array_foreach(i, n, jcfg.nw.subnets[0].nodes){
    json_object *tmp, *n;
    const char *v;
    uint8_t uuid_buf[16];
    n = json_object_array_get_idx(jcfg.nw.subnets[0].nodes, i);
    json_object_object_get_ex(n, STR_UUID, &tmp);
    v = json_object_get_string(tmp);
    if (ec_success != str2cbuf(v, 0, (char *)uuid_buf, 16)) {
      LOGE("STR to CBUF error\n");
      continue;
    }
    if (!memcmp(uuid, uuid_buf, 16)) {
      return n;
    }
  }
  return NULL;
}

static err_t modify_node_field(const uint8_t *uuid,
                               const void *key,
                               char *value)
{
  json_object *obj;
  ASSERT(uuid && key && value);
  obj = find_node(uuid, 0);
  if (!obj) {
    return err(ec_not_exist);
  }
  __kv_replace(obj, key, value);
  if (jcfg.nw.gen.autoflush) {
    return json_cfg_flush(NW_NODES_CFG_FILE);
  }
  return ec_success;
}

static err_t _backlog_node_add(const uint8_t *uuid)
{
  err_t e;
  char uuid_str[33] = { 0 };
  node_t *n = NULL;
  json_object *obj;

  if (ec_success != (e = cbuf2str((char *)uuid, 16, 0, uuid_str, 33))) {
    return e;
  }
  obj = json_object_new_object();

  json_object_object_add(obj, STR_UUID, json_object_new_string(uuid_str));
  json_object_object_add(obj, STR_ADDR, json_object_new_string("0x0000"));
  json_object_object_add(obj, STR_ERRBITS, json_object_new_string("0x00000000"));
  json_object_object_add(obj, STR_TMPL, json_object_new_string("0x00"));
  json_object_object_add(obj, STR_RMORBL, json_object_new_string("0x00"));
  json_object_object_add(obj, STR_DONE, json_object_new_string("0x00"));

  json_object_array_add(jcfg.nw.backlog, obj);

  n = calloc(1, sizeof(node_t));
  memcpy(n->uuid, uuid, 16);
  if (ec_success != (e = cfgdb_add(n))) {
    free(n);
    goto fail;
  }

  if (jcfg.nw.gen.autoflush) {
    if (ec_success != (e = json_cfg_flush(NW_NODES_CFG_FILE))) {
      goto fail;
    }
  }

  fail:
  if (e != ec_success) {
    cfgdb_remove(n, 1);
    json_object_put(obj);
  }
  return ec_success;
}

static err_t set_node_addr(const void *key,
                           void *data)
{
  /* Key is uuid and data is the address */
  char buf[7] = { 0 };

  if (!key || !data) {
    return err(ec_param_invalid);
  }
  buf[0] = '0';
  buf[1] = 'x';
  uint16_tostr(*(uint16_t *)data, buf + 2);
  return modify_node_field(key, STR_ADDR, buf);
}

static err_t set_node_errbits(const void *key,
                              void *data)
{
  /* Key is uuid and data is the errbits */
  char buf[11] = { 0 };

  if (!key || !data) {
    return err(ec_param_invalid);
  }
  buf[0] = '0';
  buf[1] = 'x';
  uint32_tostr(*(uint32_t *)data, buf + 2);
  return modify_node_field(key, STR_ERRBITS, buf);
}

static err_t set_node_done(const void *key,
                           void *data)
{
  /* Key is uuid and data is the done */
  char buf[5] = { 0 };

  if (!key || !data) {
    return err(ec_param_invalid);
  }
  buf[0] = '0';
  buf[1] = 'x';
  uint8_tostr(*(uint8_t *)data, buf + 2);
  return modify_node_field(key, STR_DONE, buf);
}

static err_t write_nodes(int wrtype,
                         const void *key,
                         void *data)
{
  switch (wrtype) {
    case wrt_add_node:
      return _backlog_node_add(data);
      break;
    case wrt_errbits:
      return set_node_errbits(key, data);
      break;
    case wrt_node_addr:
      set_node_addr(key, data);
      break;
    case wrt_done:
      set_node_done(key, data);
      break;
    default:
      return err(ec_param_invalid);
  }
  return ec_success;
}

static err_t modify_provself_field(const uint8_t *uuid,
                                   const void *key,
                                   char *value)
{
}

static inline void __provself_clrctl(provcfg_t *pc)
{
  pc->addr = 0;
  pc->ivi = 0;
  pc->sync_time = time(NULL);
  pc->subnets[0].netkey.done = 0;
  pc->subnets[0].netkey.id = 0;
  for (int i = 0; i < pc->subnets[0].active_appkey_num; i++) {
    pc->subnets[0].appkey[i].done = 0;
    pc->subnets[0].appkey[i].id = 0;
  }
}

static inline void __provself_setaddr(provcfg_t *pc, void *data)
{
  pc->addr = *(uint16_t *)data;
}

static inline void __provself_setsynctime(provcfg_t *pc, void *data)
{
  pc->sync_time = *(uint32_t *)data;
}

static inline void __provself_setnetkeyid(provcfg_t *pc, void *data)
{
  pc->subnets[0].netkey.id = *(uint16_t *)data;
}

static inline void __provself_setnetkeydone(provcfg_t *pc, void *data)
{
  pc->subnets[0].netkey.done = *(uint8_t *)data;
}

static err_t write_provself(int wrtype,
                            const void *key,
                            void *data)
{
  provcfg_t *provcfg = get_provcfg();
  switch (wrtype) {
    case wrt_clrctl:
      __provself_clrctl(provcfg);
      break;
    case wrt_prov_addr:
      __provself_setaddr(provcfg, data);
      break;
    case wrt_prov_synctime:
      __provself_setsynctime(provcfg, data);
      break;
    case wrt_prov_netkey_id:
      __provself_setnetkeyid(provcfg, data);
      break;
    case wrt_prov_netkey_done:
      __provself_setnetkeydone(provcfg, data);
      break;
    default:
      return err(ec_param_invalid);
  }
  if (jcfg.prov.gen.autoflush) {
    return json_cfg_flush(PROV_CFG_FILE);
  }
  return ec_success;
}

static err_t write_template(int wrtype,
                            const void *key,
                            void *data)
{
  return err(ec_not_supported);
}

err_t json_cfg_write(int cfg_fd,
                     int wrtype,
                     const void *key,
                     void *data)
{
  cfg_general_t *gen;
  if (cfg_fd > TEMPLATE_FILE || cfg_fd < PROV_CFG_FILE) {
    return err(ec_param_invalid);
  }
  gen = gen_from_fd(cfg_fd);
  if (!gen || !gen->root || !gen->fp) {
    return err(ec_json_null);
  }

  if (cfg_fd == NW_NODES_CFG_FILE) {
    return write_nodes(wrtype, key, data);
  } else if (cfg_fd == PROV_CFG_FILE) {
    return write_provself(wrtype, key, data);
  }
  return write_template(wrtype, key, data);
}

err_t json_cfg_read(int cfg_fd,
                    int rdtype,
                    const void *key,
                    void *data)
{
  return ec_success;
}
