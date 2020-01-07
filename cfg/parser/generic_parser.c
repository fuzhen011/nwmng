/*************************************************************************
    > File Name: generic_parser.c
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "projconfig.h"
#include "json_parser.h"
#include "generic_parser.h"
#include "cfgdb.h"
#include "cfg.h"
#include "mng.h"

/* Defines  *********************************************************** */
typedef struct {
  bool initialized;
  gp_init_func_t init;
  gp_deinit_func_t deinit;
  gp_open_func_t open;
  gp_read_func_t read;
  gp_write_func_t write;
  gp_close_func_t close;
  gp_flush_func_t flush;
}gp_t;

/* Global Variables *************************************************** */
#define CHECK_STATE(ret) \
  do { if (!gp.initialized) { return err((ret)); } } while (0)
#define CHECK_NULL_RET() \
  do { if (!gp.initialized) { return NULL; } } while (0)
#define CHECK_VOID_RET() \
  do { if (!gp.initialized) { return; } } while (0)

/* Static Variables *************************************************** */
gp_t gp = { 0 };

/* Static Functions Declaractions ************************************* */

void gp_init(int cfg_filetype, void *init_data)
{
  if (cfg_filetype != cft_json) {
    return;
  }
  if (gp.initialized) {
    return;
  }
  /* gp.init = NULL; */
  /* gp.deinit = NULL; */
  gp.open = json_cfg_open;
  gp.write = json_cfg_write;
  gp.read = json_cfg_read;
  gp.close = json_cfg_close;
  gp.flush = json_cfg_flush;

  if (gp.init) {
    gp.init(init_data);
  }
  gp.initialized = true;
}

void gp_deinit(void)
{
  memset(&gp, 0, sizeof(gp_t));
}

static inline err_t prov_clrctl(void)
{
  return gp.write(PROV_CFG_FILE, wrt_clrctl, NULL, NULL);
}

static inline err_t nodes_clrctl(void)
{
  return gp.write(NW_NODES_CFG_FILE, wrt_clrctl, NULL, NULL);
}

err_t cfg_clrctl(void)
{
  err_t e;
  EC(ec_success, prov_clrctl());
  EC(ec_success, nodes_clrctl());
  return e;
}

err_t provset_addr(const uint16_t *addr)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_addr, NULL, (void *)addr);
}

err_t provset_ivi(const uint32_t *ivi)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_ivi, NULL, (void *)ivi);
}

err_t provset_synctime(int len, const char *arg)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_synctime, NULL, (void *)arg);
}

err_t provset_netkeyid(const uint16_t *id)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_netkey_id, NULL, (void *)id);
}

err_t provset_netkeydone(const uint8_t *done)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_netkey_done, NULL, (void *)done);
}

err_t provset_appkeyid(const uint16_t *refid, const uint16_t *id)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_appkey_id, (void *)refid, (void *)id);
}

err_t provset_appkeydone(const uint16_t *refid, const uint8_t *done)
{
  return gp.write(PROV_CFG_FILE, wrt_prov_appkey_done, refid, (void *)done);
}

err_t backlog_dev(const uint8_t *uuid)
{
  return gp.write(NW_NODES_CFG_FILE, wrt_add_node, NULL, (void *)uuid);
}

int file_modified(int cfg_fd)
{
  uint8_t r;
  err_t e = gp.read(cfg_fd, rdt_modified, NULL, &r);
  if (e != ec_success) {
    elog(e);
    return -1;
  }
  return r;
}

err_t load_cfg_file(int cfg_fd, bool force_reload)
{
  err_t e;
  if (cfg_fd > TEMPLATE_FILE || cfg_fd < PROV_CFG_FILE) {
    return err(ec_param_invalid);
  }
  const char *fp = (cfg_fd == TEMPLATE_FILE ? TMPLATE_FILE_PATH
                    : cfg_fd == NW_NODES_CFG_FILE ? NWNODES_FILE_PATH : SELFCFG_FILE_PATH);
  e = gp.open(cfg_fd, fp, force_reload ? FL_FORCE_RELOAD : 0);
  if (ec_success != e) {
    elog(e);
    return e;
  }
  if (cfg_fd == NW_NODES_CFG_FILE) {
    mng_load_lists();
  }
  gp.flush(cfg_fd); /* Update the synctime value */
  return ec_success;
}

err_t upl_nodeset_addr(const uint8_t *uuid, uint16_t addr)
{
  err_t e;
  node_t *n;
  e = gp.write(NW_NODES_CFG_FILE, wrt_node_addr, uuid, (void *)&addr);
  elog(e);

  n = cfgdb_unprov_dev_get(uuid);
  cfgdb_unpl_remove(n, 0);
  n->addr = addr;
  cfgdb_nodes_add(n);
  return e;
}

err_t nodes_rm(uint16_t addr)
{
  err_t e;
  node_t *n;
  n = cfgdb_node_get(addr);
  cfgdb_nodes_remove(n, 0);
  n->addr = 0;
  cfgdb_unpl_add(n);
  e = gp.write(NW_NODES_CFG_FILE, wrt_node_addr, n->uuid, (void *)&n->addr);
  elog(e);
  return e;
}

err_t nodeset_errbits(uint16_t addr, lbitmap_t err)
{
  err_t e;
  node_t *n;
  n = cfgdb_node_get(addr);
  n->err = err;

  e = gp.write(NW_NODES_CFG_FILE, wrt_errbits, (void *)n->uuid, (void *)&err);
  elog(e);
  return e;
}

err_t nodeset_done(uint16_t addr, uint8_t done)
{
  err_t e;
  node_t *n;
  n = cfgdb_node_get(addr);
  n->done = done;

  e = gp.write(NW_NODES_CFG_FILE, wrt_done, (void *)n->uuid, (void *)&done);
  elog(e);
  return e;
}

const char *nodeget_cfgstr(uint16_t addr)
{
  err_t e;
  const char *v;
  node_t *n = cfgdb_node_get(addr);
  e = gp.read(NW_NODES_CFG_FILE, rdt_node_str, n->uuid, (void *)&v);
  elog(e);
  return v;
}
