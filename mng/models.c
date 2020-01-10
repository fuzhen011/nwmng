/*************************************************************************
    > File Name: models.c
    > Author: Kevin
    > Created Time: 2020-01-08
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "host_gecko.h"
#include "mng.h"
#include "cli.h"
#include "logging.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */
static uint8_t tid = 0;

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static err_t clicb_perc_set(int argc, char *argv[], uint8_t type);

err_t clicb_onoff(int argc, char *argv[])
{
  mng_t *mng = get_mng();
  err_t e = ec_success;

  if (mng->cache.model_set.type) {
    cli_print_busy();
    return err(ec_state);
  }
  if (argc < 2) {
    return err(ec_param_invalid);
  }
  if (!strcmp(argv[1], "on")) {
    mng->cache.model_set.value = 1;
  } else if (!strcmp(argv[1], "off")) {
    mng->cache.model_set.value = 0;
  } else {
    return err(ec_param_invalid);
  }

  if (argc == 2) {
    uint16list_t *addrs = get_lights_addrs(onoff_support);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_set.nodes = g_list_append(mng->cache.model_set.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGD("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_set.nodes = g_list_append(mng->cache.model_set.nodes, addr);
    }
  }

  if (mng->cache.model_set.nodes) {
    mng->cache.model_set.type = onoff_support;
  }
  return e;
}

err_t clicb_lightness(int argc, char *argv[])
{
  return clicb_perc_set(argc, argv, lightness_support);
}

err_t clicb_ct(int argc, char *argv[])
{
  return clicb_perc_set(argc, argv, ctl_support);
}

static err_t clicb_perc_set(int argc, char *argv[], uint8_t type)
{
  mng_t *mng = get_mng();
  err_t e = ec_success;
  if (mng->cache.model_set.type) {
    cli_print_busy();
    return err(ec_state);
  }
  if (argc < 2) {
    return err(ec_param_invalid);
  }

  if (ec_success != (e = str2uint(argv[1], strlen(argv[1]), &mng->cache.model_set.value,
                                  sizeof(uint8_t)))) {
    return err(ec_param_invalid);
  }
  if (mng->cache.model_set.value > 100) {
    return err(ec_param_invalid);
  }

  if (argc == 2) {
    uint16list_t *addrs = get_lights_addrs(onoff_support);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_set.nodes = g_list_append(mng->cache.model_set.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGD("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_set.nodes = g_list_append(mng->cache.model_set.nodes, addr);
    }
  }
  if (mng->cache.model_set.nodes) {
    mng->cache.model_set.type = type;
  }
  return e;
}

static uint16_t send_onoff(uint16_t addr, uint8_t onoff)
{
  return gecko_cmd_mesh_generic_client_set(0x1001,
                                           0,
                                           addr,
                                           0, /* TODO: correct this */
                                           tid++,
                                           0,
                                           0,
                                           0,
                                           MESH_GENERIC_CLIENT_REQUEST_ON_OFF,
                                           1,
                                           &onoff)->result;
}

static uint16_t send_lightness(uint16_t addr, uint8_t lightness)
{
  uint16_t lvl = lightness * 0xffff / 100;
  return gecko_cmd_mesh_generic_client_set(0x1302,
                                           0,
                                           addr,
                                           0, /* TODO: correct this */
                                           tid++,
                                           0,
                                           0,
                                           0,
                                           MESH_GENERIC_CLIENT_REQUEST_LIGHTNESS_ACTUAL,
                                           2,
                                           (uint8_t *)&lvl)->result;
}
// Minimum color temperature 800K
#define TEMPERATURE_MIN      0x0320
// Maximum color temperature 20000K
#define TEMPERATURE_MAX      0x4e20

static uint16_t send_ctl(uint16_t addr, uint8_t ctl)
{
  uint8_t buf[4] = { 0 };
  uint16_t lvl = TEMPERATURE_MIN + (ctl * ctl / 100) * (TEMPERATURE_MAX - TEMPERATURE_MIN) / 100;
  memcpy(buf, (uint8_t *)&lvl, 2);
  return gecko_cmd_mesh_generic_client_set(0x1305,
                                           0,
                                           addr,
                                           0, /* TODO: correct this */
                                           tid++,
                                           0,
                                           0,
                                           0,
                                           MESH_GENERIC_CLIENT_REQUEST_CTL_TEMPERATURE,
                                           4,
                                           buf)->result;
}

bool models_loop(mng_t *mng)
{
  uint16_t ret;

  if (!g_list_length(mng->cache.model_set.nodes)) {
    return false;
  }

  GList *item = g_list_first(mng->cache.model_set.nodes);
  if (!item) {
    ASSERT(0);
  }

  if (mng->cache.model_set.type == onoff_support) {
    ret = send_onoff(*(uint16_t *)item->data, mng->cache.model_set.value);
  } else if (mng->cache.model_set.type == lightness_support) {
    ret = send_lightness(*(uint16_t *)item->data, mng->cache.model_set.value);
  } else {
    ret = send_ctl(*(uint16_t *)item->data, mng->cache.model_set.value);
  }

  if (ret != bg_err_success) {
    if (ret == bg_err_out_of_memory) {
      return true;
    }
    LOGE("Model set[%d] error[%x].\n", ret);
  } else {
    cli_print_modelset_done(*(uint16_t *)item->data,
                            mng->cache.model_set.type,
                            mng->cache.model_set.value);
  }

  mng->cache.model_set.nodes = g_list_remove_link(mng->cache.model_set.nodes, item);
  free(item->data);
  g_list_free(item);
  if (!g_list_length(mng->cache.model_set.nodes)) {
    g_list_free(mng->cache.model_set.nodes);
    mng->cache.model_set.nodes = NULL;
    mng->cache.model_set.type = unknown;
  }
  return false;
}
