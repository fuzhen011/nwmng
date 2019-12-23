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

#include "json_parser.h"
#include "generic_parser.h"

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
static gp_t gp = { 0 };

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
