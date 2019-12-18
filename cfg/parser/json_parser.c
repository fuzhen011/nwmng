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

#include "generic_parser.h"
#include "json_parser.h"
#include "logging.h"
#include "utils.h"

/* Defines  *********************************************************** */
typedef struct {
  struct {
    char *fp;
    json_object *root;
  }prov;
  struct {
    char *fp;
    int subnet_num;
    struct {
      uint16_t id;
      json_object *nodes;
    } *sbn;
  }nw;
  struct {
    char *fp;
  }tmpl;
}json_cfg_t;
/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static json_cfg_t jcfg = { 0 };

/* Static Functions Declaractions ************************************* */
static inline char *fp_from_fd(int fd)
{
  return fd == PROV_CFG_FILE ? jcfg.prov.fp
         : fd == NW_NODES_CFG_FILE ? jcfg.nw.fp
         : fd == TEMPLATE_FILE ? jcfg.tmpl.fp : NULL;
}

static err_t new_json_file(int cfg_fd)
{
  /* TODO */
  return ec_success;
}

static err_t json_load_template(void)
{
  /* TODO */
  return ec_success;
}

static err_t json_load_file(int cfg_fd,
                            bool clrctlfls)
{
  /* TODO */
  return ec_success;
}

err_t json_cfg_open(int cfg_fd,
                    const char *filepath,
                    unsigned int flags,
                    void *data)
{
  int tmp;
  err_t ret = ec_success;
  char *fp;

  if (cfg_fd > TEMPLATE_FILE || cfg_fd < PROV_CFG_FILE) {
    return err(ec_param_invalid);
  }
  fp = fp_from_fd(cfg_fd);

  /* Ensure the fp is not NULL */
  if (!(flags & FL_CLR_CTLFS)) {
    if (!filepath) {
      if (!fp) {
        return ec_param_invalid;
      }
    } else {
      if (fp) {
        free((char *)fp);
        fp = NULL;
      }
      fp = malloc(strlen(filepath) + 1);
      strcpy(fp, filepath);
      fp[strlen(filepath)] = '\0';
    }
  } else if (!fp) {
    return ec_param_invalid;
  }
  assert(fp);

  tmp = access(fp, F_OK);

  if (cfg_fd == TEMPLATE_FILE) {
    if (tmp == -1) {
      return ec_not_exist;
    }
  } else {
    if (-1 == tmp) {
      if (!(flags & FL_CREATE)) {
        ret = ec_not_exist;
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

      else {
        if (ec_success != (ret = fjsonConfigOpen((flags & FL_CLEAR) ? true : false))) {
          goto fail;
        }
      }
  if (ec_success != (ret = json_load_file(cfg_fd, !(flags & FL_CLEAR)))) {
    return ret;
  }
  return ec_success;

  fail:
  if (ec_success != ret) {
    jsonConfigDeinit();
    LOGE("JSON[%s] Open failed, err[%d]\n", fp, ret);
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
