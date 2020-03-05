/*************************************************************************
    > File Name: models.c
    > Author: Kevin
    > Created Time: 2020-01-08
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "projconfig.h"
#include "host_gecko.h"
#include "mng.h"
#include "models.h"
#include "cli.h"
#include "logging.h"
#include "utils.h"
#include "mesh_generic_model_capi_types.h"
#include "mesh_lighting_model_capi_types.h"
#include "mesh_sensor_model_capi_types.h"

/* Defines  *********************************************************** */
#define LC_SERVER_ADDR(addr)  ((addr) + 1)
#define CTL_TEMPERATURE_SERVER_ADDR(addr)  ((addr) + 1)

enum {
  LC_STATE_ONOFF,
  LC_STATE_MODE,
  LC_STATE_OM
};

#ifdef DEMO_EN
#if 0
const char *demo_cmds[] = {
  "onoff on 0xc021",
  "onoff on 0xc022",
  "onoff on 0xc023",
  "onoff on 0xc024",
  "onoff on 0xc025",
  "onoff on 0xc026",
  "onoff on 0xc027",
  "onoff on 0xc028",
  "onoff on 0xc029",
  "onoff on 0xc02a",

  "onoff off 0xc02a",
  "onoff off 0xc029",
  "onoff off 0xc028",
  "onoff off 0xc027",
  "onoff off 0xc026",
  "onoff off 0xc025",
  "onoff off 0xc024",
  "onoff off 0xc023",
  "onoff off 0xc022",
  "onoff off 0xc021",
};
#endif
#if 0
const char *demo_cmds[] = {
  "lightness 1 0xc021",
  "lightness 50 0xc021",
  "lightness 100 0xc021",
  "lightness 1 0xc022",
  "lightness 50 0xc022",
  "lightness 100 0xc022",
  "lightness 1 0xc023",
  "lightness 50 0xc023",
  "lightness 100 0xc023",
  "lightness 1 0xc024",
  "lightness 50 0xc024",
  "lightness 100 0xc024",
  "lightness 1 0xc025",
  "lightness 50 0xc025",
  "lightness 100 0xc025",
  "lightness 1 0xc026",
  "lightness 50 0xc026",
  "lightness 100 0xc026",
  "lightness 1 0xc027",
  "lightness 50 0xc027",
  "lightness 100 0xc027",
  "lightness 1 0xc028",
  "lightness 50 0xc028",
  "lightness 100 0xc028",
  "lightness 1 0xc029",
  "lightness 50 0xc029",
  "lightness 100 0xc029",
  "lightness 1 0xc02a",
  "lightness 50 0xc02a",
  "lightness 100 0xc02a",
  "onoff off 0xc030",
  "lightness 1 0xc030",
  "lightness 50 0xc030",
  "lightness 100 0xc030",
  "onoff off 0xc030",
};
#endif

const char *demo_cmds[] = {
  "lightness 0 0xc030",
  "lightness 40 0xc030",
  "lightness 70 0xc030",
  "lightness 100 0xc030",
  "lightness 70 0xc030",
  "lightness 40 0xc030",
};
static const size_t cmds_len = ARR_LEN(demo_cmds);

#define DEMO_INTERVAL 1
static struct demo{
  bool on;
  int pos;
  time_t expired;
} demo;
#endif

/* Global Variables *************************************************** */
static uint8_t tid = 0;

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
static err_t clicb_perc_set(int argc, char *argv[], uint8_t func);
static err_t clicb_perc_get(int argc, char *argv[], uint8_t func);

#ifdef DEMO_EN
#if 0
err_t clicb_demo(int argc, char *argv[])
{
  int onoff = 1;
  if (argc > 1) {
    if (!strcmp(argv[1], "on")) {
      onoff = 1;
    } else if (!strcmp(argv[1], "off")) {
      onoff = 0;
    } else {
      return err(ec_param_invalid);
    }
  }
  if (onoff) {
    demo.on = 1;
    demo.expired = time(NULL) + DEMO_INTERVAL;
    demo.pos = 0;
  } else {
    demo.on = 0;
  }

  return ec_success;
}
#else
err_t clicb_demo(int argc, char *argv[])
{
  if (argc > 1) {
    if (!strcmp(argv[1], "on")) {
      demo_start(1);
    } else if (!strcmp(argv[1], "off")) {
      demo_start(0);
    } else {
      return err(ec_param_invalid);
    }
  }

  return ec_success;
}
#endif

void check_demo(void)
{
  time_t now;
  err_t e;
  wordexp_t w;
  if (!demo.on) {
    return;
  }
  now = time(NULL);
  if (now < demo.expired) {
    return;
  }

  if (wordexp(demo_cmds[demo.pos++], &w, WRDE_NOCMD)) {
    return;
  }
  if (!strcmp("onoff", w.we_wordv[0])) {
    e = clicb_onoff(w.we_wordc, w.we_wordv);
  } else if (!strcmp("lightness", w.we_wordv[0])) {
    e = clicb_lightness(w.we_wordc, w.we_wordv);
  } else {
    ASSERT(0);
  }

  elog(e);

  if (demo.pos == cmds_len) {
    demo.pos = 0;
    /* now += 2; */
  }
  demo.expired = now + DEMO_INTERVAL;
  wordfree(&w);
}
#endif

static inline void parse_dcd(const uint8_t *data, uint8_t len, mng_t *mng)
{
  uint8_t offs = 0, offs1, nums, numv;
  if (mng->dcd.valid) {
    /* TODO: Free first */
  }
  /* No sanity check here */
  mng->dcd.company_id = uint16_from_buf(data);
  offs += 2;
  mng->dcd.product_id = uint16_from_buf(data + offs);
  offs += 2;
  mng->dcd.version_numer = uint16_from_buf(data + offs);
  offs += 2;
  mng->dcd.rpl_size = uint16_from_buf(data + offs);
  offs += 2;
  mng->dcd.feature_bitmask = uint16_from_buf(data + offs);
  offs += 2;

  offs1 = offs;

  /* Allocate memory for elements */
  mng->dcd.element_num = 0;
  while (offs1 < len) {
    offs1 += 2;
    nums = data[offs1++];
    numv = data[offs1++];
    mng->dcd.element_num++;
    offs1 += nums * 2 + numv * 4;
  }
  LOGD("%d Elements\n", mng->dcd.element_num);
  if (!mng->dcd.element_num) {
    return;
  }

  mng->dcd.elements = malloc(mng->dcd.element_num * sizeof(dcd_element_t));

  offs1 = 0;
  while (offs < len) {
    mng->dcd.elements[offs1].location = uint16_from_buf(data + offs);
    offs += 2;
    mng->dcd.elements[offs1].sig_model_num = data[offs++];
    mng->dcd.elements[offs1].vendor_model_num = data[offs++];
    if (mng->dcd.elements[offs1].sig_model_num) {
      mng->dcd.elements[offs1].sigmodels = malloc(sizeof(uint16_t) * mng->dcd.elements[offs1].sig_model_num);
    }
    if (mng->dcd.elements[offs1].vendor_model_num) {
      mng->dcd.elements[offs1].vendor_models = malloc(2 * sizeof(uint16_t) * mng->dcd.elements[offs1].vendor_model_num);
    }
    for (int i = 0; i < mng->dcd.elements[offs1].sig_model_num; i++) {
      mng->dcd.elements[offs1].sigmodels[i] = uint16_from_buf(data + offs);
      offs += 2;
    }
    for (int i = 0; i < mng->dcd.elements[offs1].vendor_model_num; i++) {
      mng->dcd.elements[offs1].vendor_models[i].vendor_id = uint16_from_buf(data + offs);
      offs += 2;
      mng->dcd.elements[offs1].vendor_models[i].model_id = uint16_from_buf(data + offs);
      offs += 2;
    }
    offs1++;
  }
  mng->dcd.valid = true;
}

extern int appkey_by_refid(mng_t *mng, uint16_t refid, uint16_t *id);
/**
 * @brief models_init - This function will read the DCD from the ncp, then
 * initialize the client models on it.
 *
 * @param mng -
 *
 * @return
 */
void models_init(mng_t *mng)
{
  uint16_t ret, id;
  struct gecko_msg_mesh_test_get_local_config_rsp_t *r;

  r = gecko_cmd_mesh_test_get_local_config(mesh_node_dcd, 0);

  if (r->result != bg_err_success) {
    LOGBGE("gecko_cmd_mesh_test_get_local_config", r->result);
    exit(1);
  }

  parse_dcd(r->data.data, r->data.len, mng);

  /* Locally bind all models to the given binding list */
  if (mng->cfg->bindings && mng->cfg->bindings->len) {
    for (int i = 0; i < mng->cfg->bindings->len; i++) {
      appkey_by_refid(mng, mng->cfg->bindings->data[i], &id);
      for (int elem = 0; elem < mng->dcd.element_num; elem++) {
        for (int s = 0; s < mng->dcd.elements[elem].sig_model_num; s++) {
          ret = gecko_cmd_mesh_test_bind_local_model_app(elem,
                                                         id,
                                                         0xffff,
                                                         mng->dcd.elements[elem].sigmodels[s])->result;
          if (bg_err_mesh_already_exists == ret) {
            LOGD("Models already bound with given appkey index(es)\n");
            goto already_bound;
          } else if (bg_err_success != ret) {
            LOGE("Bind SIG model %04x failed with %04x\n", mng->dcd.elements[elem].sigmodels[s], ret);
          }
        }
        for (int v = 0; v < mng->dcd.elements[elem].vendor_model_num; v++) {
          ret = gecko_cmd_mesh_test_bind_local_model_app(elem,
                                                         id,
                                                         mng->dcd.elements[elem].vendor_models[v].vendor_id,
                                                         mng->dcd.elements[elem].vendor_models[v].model_id)->result;
          if (bg_err_mesh_already_exists == ret) {
            LOGD("Models already bound\n");
            goto already_bound;
          } else if (bg_err_success != ret) {
            LOGE("Bind Vendor model %04x:%04x failed with %04x\n",
                 mng->dcd.elements[elem].vendor_models[v].vendor_id,
                 mng->dcd.elements[elem].vendor_models[v].model_id,
                 ret);
          }
        }
      }
    }
  }
  LOGD("Models bound with given appkey index(es)\n");

  already_bound:

  /*
   * Initialize all the required model classes
   */
  /* Generic client model */
  if (bg_err_success != (ret = gecko_cmd_mesh_generic_client_init()->result)) {
    LOGBGE("gecko_cmd_mesh_generic_client_init", ret);
  }

  /* Sensor client model */
  if (bg_err_success != (ret = gecko_cmd_mesh_sensor_client_init()->result)) {
    LOGBGE("gecko_cmd_mesh_sensor_client_init", ret);
  }
#if (LC_CLIENT_PRESENT == 1)
  /* LC client model */
  if (bg_err_success != (ret = gecko_cmd_mesh_lc_client_init(LC_ELEM_INDEX)->result)) {
    LOGBGE("gecko_cmd_mesh_lc_client_init", ret);
  }
#endif
#if (SCENE_CLIENT_PRESENT == 1)
  /* Scene client model */
  if (bg_err_success != (ret = gecko_cmd_mesh_scene_client_init(SCENE_ELEM_INDEX)->result)) {
    LOGBGE("gecko_cmd_mesh_scene_client_init", ret);
  }
#endif
}

err_t clicb_onoff(int argc, char *argv[])
{
  mng_t *mng = get_mng();
  err_t e = ec_success;

  if (mng->cache.model_operation.type) {
    cli_print_busy();
    return err(ec_state);
  }
  if (argc < 2) {
    return err(ec_param_invalid);
  }
  if (!strcmp(argv[1], "on")) {
    mng->cache.model_operation.value = 1;
  } else if (!strcmp(argv[1], "off")) {
    mng->cache.model_operation.value = 0;
  } else {
    return err(ec_param_invalid);
  }

  if (argc == 2) {
    uint16list_t *addrs = get_lights_addrs(ONOFF_SV_BIT);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGE("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
  }

  if (mng->cache.model_operation.nodes) {
    mng->cache.model_operation.type = MO_SET;
    mng->cache.model_operation.func = ONOFF_SV_BIT;
  }
  return e;
}

err_t clicb_onoff_get(int argc, char *argv[])
{
  mng_t *mng = get_mng();
  err_t e = ec_success;

  if (mng->cache.model_operation.type) {
    cli_print_busy();
    return err(ec_state);
  }

  if (argc == 1) {
    uint16list_t *addrs = get_lights_addrs(ONOFF_SV_BIT);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGE("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
  }

  if (mng->cache.model_operation.nodes) {
    mng->cache.model_operation.type = MO_GET;
    mng->cache.model_operation.func = ONOFF_SV_BIT;
  }
  return e;
}

err_t clicb_lightness(int argc, char *argv[])
{
  return clicb_perc_set(argc, argv, LIGHTNESS_SV_BIT);
}

err_t clicb_ct(int argc, char *argv[])
{
  return clicb_perc_set(argc, argv, CTL_SV_BIT);
}

err_t clicb_lightness_get(int argc, char *argv[])
{
  return clicb_perc_get(argc, argv, LIGHTNESS_SV_BIT);
}

err_t clicb_ct_get(int argc, char *argv[])
{
  return clicb_perc_get(argc, argv, CTL_SV_BIT);
}

err_t clicb_lcget(int argc, char *argv[])
{
  mng_t *mng = get_mng();
  err_t e = ec_success;

  if (mng->cache.model_operation.type) {
    cli_print_busy();
    return err(ec_state);
  }

  if (argc < 2) {
    return err(ec_param_invalid);
  }

  if (!strcmp(argv[1], "onoff")) {
    mng->cache.model_operation.sub_which = LC_STATE_ONOFF;
  } else if (!strcmp(argv[1], "mode")) {
    mng->cache.model_operation.sub_which = LC_STATE_MODE;
  } else if (!strcmp(argv[1], "om")) {
    mng->cache.model_operation.sub_which = LC_STATE_OM;
  } else {
    return err(ec_param_invalid);
  }

  if (argc == 2) {
    uint16list_t *addrs = get_lights_addrs(LC_SV_BIT);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGE("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
  }

  if (mng->cache.model_operation.nodes) {
    mng->cache.model_operation.type = MO_GET;
    mng->cache.model_operation.func = LC_SV_BIT;
  }

  return ec_success;
}

err_t clicb_lcset(int argc, char *argv[])
{
  mng_t *mng = get_mng();
  err_t e = ec_success;

  if (mng->cache.model_operation.type) {
    cli_print_busy();
    return err(ec_state);
  }

  if (argc < 3) {
    return err(ec_param_invalid);
  }

  if (!strcmp(argv[1], "onoff")) {
    mng->cache.model_operation.sub_which = LC_STATE_ONOFF;
  } else if (!strcmp(argv[1], "mode")) {
    mng->cache.model_operation.sub_which = LC_STATE_MODE;
  } else if (!strcmp(argv[1], "om")) {
    mng->cache.model_operation.sub_which = LC_STATE_OM;
  } else {
    return err(ec_param_invalid);
  }

  if (!strcmp(argv[2], "on")) {
    mng->cache.model_operation.value = 1;
  } else if (!strcmp(argv[2], "off")) {
    mng->cache.model_operation.value = 0;
  } else {
    return err(ec_param_invalid);
  }
  if (argc == 3) {
    uint16list_t *addrs = get_lights_addrs(LC_SV_BIT);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 3; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGE("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
  }

  if (mng->cache.model_operation.nodes) {
    mng->cache.model_operation.type = MO_SET;
    mng->cache.model_operation.func = LC_SV_BIT;
  }
  return ec_success;
}

err_t clicb_lcpropertyget(int argc, char *argv[])
{
  return ec_success;
}

err_t clicb_lcpropertyset(int argc, char *argv[])
{
  return ec_success;
}

static err_t clicb_perc_set(int argc, char *argv[], uint8_t func)
{
  mng_t *mng = get_mng();
  err_t e = ec_success;
  if (mng->cache.model_operation.type) {
    cli_print_busy();
    return err(ec_state);
  }
  if (argc < 2) {
    return err(ec_param_invalid);
  }

  if (ec_success != (e = str2uint(argv[1],
                                  strlen(argv[1]),
                                  &mng->cache.model_operation.value,
                                  sizeof(uint32_t)))) {
    return err(ec_param_invalid);
  }
  if (mng->cache.model_operation.value > 100) {
    return err(ec_param_invalid);
  }

  if (argc == 2) {
    uint16list_t *addrs = get_lights_addrs(func);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGE("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
  }
  if (mng->cache.model_operation.nodes) {
    mng->cache.model_operation.type = MO_SET;
    mng->cache.model_operation.func = func;
  }
  return e;
}

static err_t clicb_perc_get(int argc, char *argv[], uint8_t func)
{
  mng_t *mng = get_mng();
  err_t e = ec_success;
  if (mng->cache.model_operation.type) {
    cli_print_busy();
    return err(ec_state);
  }

  if (argc == 1) {
    uint16list_t *addrs = get_lights_addrs(func);
    if (!addrs) {
      return e;
    }
    for (int i = 0; i < addrs->len; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      *addr = addrs->data[i];
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
    free(addrs->data);
    free(addrs);
  } else {
    for (int i = 2; i < argc; i++) {
      uint16 *addr = malloc(sizeof(uint16_t));
      if (ec_success != str2uint(argv[i], strlen(argv[i]), addr, sizeof(uint16_t))) {
        LOGE("str2uint failed\n");
        free(addr);
        continue;
      }
      mng->cache.model_operation.nodes = g_list_append(mng->cache.model_operation.nodes, addr);
    }
  }
  if (mng->cache.model_operation.nodes) {
    mng->cache.model_operation.type = MO_GET;
    mng->cache.model_operation.func = func;
  }
  return e;
}

uint16_t send_onoff(uint16_t addr, uint8_t onoff)
{
  return gecko_cmd_mesh_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
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

uint16_t onoff_get(uint16_t addr)
{
  return gecko_cmd_mesh_generic_client_get(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                           0,
                                           addr,
                                           0,
                                           mesh_generic_state_on_off)->result;
}

uint16_t send_lightness(uint16_t addr, uint8_t lightness)
{
  uint16_t lvl = lightness * 0xffff / 100;
  return gecko_cmd_mesh_generic_client_set(MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID,
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

uint16_t lightness_get(uint16_t addr)
{
  return gecko_cmd_mesh_generic_client_get(MESH_LIGHTING_LIGHTNESS_CLIENT_MODEL_ID,
                                           0,
                                           addr,
                                           0,
                                           mesh_lighting_state_lightness_actual)->result;
}

uint16_t send_ctl(uint16_t addr, uint8_t ctl)
{
  uint8_t buf[4] = { 0 };
  uint16_t lvl = MESH_LIGHTING_CTL_TEMPERATURE_MIN + (ctl * ctl / 100) * (MESH_LIGHTING_CTL_TEMPERATURE_MAX - MESH_LIGHTING_CTL_TEMPERATURE_MIN) / 100;
  memcpy(buf, (uint8_t *)&lvl, 2);
  return gecko_cmd_mesh_generic_client_set(MESH_LIGHTING_CTL_CLIENT_MODEL_ID,
                                           0,
                                           CTL_TEMPERATURE_SERVER_ADDR(addr),
                                           0, /* TODO: correct this */
                                           tid++,
                                           0,
                                           0,
                                           0,
                                           MESH_GENERIC_CLIENT_REQUEST_CTL_TEMPERATURE,
                                           4,
                                           buf)->result;
}

uint16_t ctl_get(uint16_t addr)
{
  return gecko_cmd_mesh_generic_client_get(MESH_LIGHTING_CTL_CLIENT_MODEL_ID,
                                           0,
                                           CTL_TEMPERATURE_SERVER_ADDR(addr),
                                           0,
                                           mesh_lighting_state_ctl_temperature)->result;
  /* mesh_lighting_state_ctl)->result; */
}

static err_t model_get_handler(uint16_t addr, mng_t *mng, uint16_t *bgerr)
{
  if (mng->cache.model_operation.func == LC_SV_BIT) {
    if (mng->cache.model_operation.sub_which == LC_STATE_ONOFF) {
      *bgerr = gecko_cmd_mesh_lc_client_get_light_onoff(LC_ELEM_INDEX,
                                                        LC_SERVER_ADDR(addr), 0)->result;
    } else if (mng->cache.model_operation.sub_which == LC_STATE_MODE) {
      *bgerr = gecko_cmd_mesh_lc_client_get_mode(LC_ELEM_INDEX,
                                                 LC_SERVER_ADDR(addr), 0)->result;
    } else {
      *bgerr = gecko_cmd_mesh_lc_client_get_om(LC_ELEM_INDEX,
                                               LC_SERVER_ADDR(addr), 0)->result;
    }
  } else if (mng->cache.model_operation.func == ONOFF_SV_BIT) {
    *bgerr = onoff_get(addr);
  } else if (mng->cache.model_operation.func == LIGHTNESS_SV_BIT) {
    *bgerr = lightness_get(addr);
  } else if (mng->cache.model_operation.func == CTL_SV_BIT) {
    *bgerr = ctl_get(addr);
  } else {
    return err(ec_not_supported);
  }
  return (*bgerr == bg_err_success) ? ec_success : err(ec_bgrsp);
}

static err_t model_set_handler(uint16_t addr, mng_t *mng, uint16_t *bgerr)
{
  if (mng->cache.model_operation.func == ONOFF_SV_BIT) {
    *bgerr = send_onoff(addr, mng->cache.model_operation.value);
  } else if (mng->cache.model_operation.func == LIGHTNESS_SV_BIT) {
    *bgerr = send_lightness(addr, mng->cache.model_operation.value);
  } else if (mng->cache.model_operation.func == CTL_SV_BIT) {
    *bgerr = send_ctl(addr, mng->cache.model_operation.value);
  } else if (mng->cache.model_operation.func == LC_SV_BIT) {
    if (mng->cache.model_operation.sub_which == LC_STATE_ONOFF) {
      *bgerr = gecko_cmd_mesh_lc_client_set_light_onoff(LC_ELEM_INDEX,
                                                        LC_SERVER_ADDR(addr),
                                                        0,
                                                        0x2,
                                                        (uint8_t)mng->cache.model_operation.value,
                                                        tid++,
                                                        0,
                                                        0)->result;
    } else if (mng->cache.model_operation.sub_which == LC_STATE_MODE) {
      *bgerr = gecko_cmd_mesh_lc_client_set_mode(LC_ELEM_INDEX,
                                                 LC_SERVER_ADDR(addr),
                                                 0,
                                                 0x2,
                                                 (uint8_t)mng->cache.model_operation.value)->result;
    } else {
      *bgerr = gecko_cmd_mesh_lc_client_set_om(LC_ELEM_INDEX,
                                               LC_SERVER_ADDR(addr),
                                               0,
                                               0x2,
                                               (uint8_t)mng->cache.model_operation.value)->result;
    }
  } else {
    return err(ec_not_supported);
  }
  return (*bgerr == bg_err_success) ? ec_success : err(ec_bgrsp);
}

static err_t model_get_property_handler(uint16_t addr, mng_t *mng, uint16_t *bgerr)
{
  return err(ec_not_supported);
}

static err_t model_set_property_handler(uint16_t addr, mng_t *mng, uint16_t *bgerr)
{
  return err(ec_not_supported);
}

bool models_loop(mng_t *mng)
{
  err_t e;
  uint16_t ret = bg_err_success;

#ifdef DEMO_EN
  check_demo();
#endif
  if (!g_list_length(mng->cache.model_operation.nodes)) {
    return false;
  }

  GList *item = g_list_first(mng->cache.model_operation.nodes);
  ASSERT(item);

  if (mng->cache.model_operation.type == MO_GET) {
    e = model_get_handler(*(uint16_t *)item->data, mng, &ret);
  } else if (mng->cache.model_operation.type == MO_SET) {
    e = model_set_handler(*(uint16_t *)item->data, mng, &ret);
  } else if (mng->cache.model_operation.type == MO_GET_PROPERTY) {
    e = model_get_property_handler(*(uint16_t *)item->data, mng, &ret);
  } else if (mng->cache.model_operation.type == MO_SET_PROPERTY) {
    e = model_set_property_handler(*(uint16_t *)item->data, mng, &ret);
  } else {
    e = err(ec_not_supported);
  }

  if (e == ec_success) {
#if 0
    cli_print_modelset_done(*(uint16_t *)item->data,
                            mng->cache.model_operation.type,
                            mng->cache.model_operation.value);
#endif
  } else if (ret == bg_err_out_of_memory) {
    return true;
  } else {
    elog(e);
    LOGE("Model Operation to Node[0x%04x] Error[0x%04x].\n", *(uint16_t *)item->data, ret);
  }

  mng->cache.model_operation.nodes = g_list_remove_link(mng->cache.model_operation.nodes, item);
  free(item->data);
  g_list_free(item);
  if (!g_list_length(mng->cache.model_operation.nodes)) {
    g_list_free(mng->cache.model_operation.nodes);
    mng->cache.model_operation.nodes = NULL;
    mng->cache.model_operation.type = MO_IDLE;
    mng->cache.model_operation.func = 0;
  }
  return false;
}

void lc_client_evt_hdr(uint32_t evt_id, const struct gecko_cmd_packet *evt)
{
  if (evt_id == gecko_evt_mesh_lc_client_light_onoff_status_id) {
    bt_shell_printf("LC Light Onoff Status\n"
                    "  From: 0x%04x To: 0x%04x\n"
                    "  Present value: %d\n"
                    "  Target  value: %d\n"
                    "  Remaining time: %u\n",
                    evt->data.evt_mesh_lc_client_light_onoff_status.server_address,
                    evt->data.evt_mesh_lc_client_light_onoff_status.destination_address,
                    evt->data.evt_mesh_lc_client_light_onoff_status.present_light_onoff,
                    evt->data.evt_mesh_lc_client_light_onoff_status.target_light_onoff,
                    evt->data.evt_mesh_lc_client_light_onoff_status.remaining_time);
  } else if (evt_id == gecko_evt_mesh_lc_client_mode_status_id) {
    bt_shell_printf("LC Light Mode Status\n"
                    "  From: 0x%04x To: 0x%04x\n"
                    "  Mode: %u\n",
                    evt->data.evt_mesh_lc_client_mode_status.server_address,
                    evt->data.evt_mesh_lc_client_mode_status.destination_address,
                    evt->data.evt_mesh_lc_client_mode_status.mode_status_value);
  } else if (evt_id == gecko_evt_mesh_lc_client_om_status_id) {
    bt_shell_printf("LC Light Occupancy Mode Status\n"
                    "  From: 0x%04x To: 0x%04x\n"
                    "  Mode: %u\n",
                    evt->data.evt_mesh_lc_client_om_status.server_address,
                    evt->data.evt_mesh_lc_client_om_status.destination_address,
                    evt->data.evt_mesh_lc_client_om_status.om_status_value);
  } else if (evt_id == gecko_evt_mesh_lc_client_property_status_id) {
    LOGE("NOT IMPL YET\n");
  }
}

void generic_light_client_evt_hdr(uint32_t evt_id, const struct gecko_cmd_packet *evt)
{
  if (!(evt->data.evt_mesh_generic_client_server_status.type == mesh_generic_state_on_off
        || evt->data.evt_mesh_generic_client_server_status.type == mesh_lighting_state_lightness_actual
        || evt->data.evt_mesh_generic_client_server_status.type == mesh_lighting_state_ctl_temperature)) {
    /* || evt->data.evt_mesh_generic_client_server_status.type == mesh_lighting_state_ctl_lightness_temperature)) { */
    LOGW("type %x missed\n", evt->data.evt_mesh_generic_client_server_status.type);
    return;
  }
  if (evt_id == gecko_evt_mesh_generic_client_server_status_id) {
    bt_shell_printf("Server Status Message:\n"
                    "  Model[0x%04x] on Element[%d]\n"
                    "  From: 0x%04x To 0x%04x\n"
                    "  Relayed %s\n",
                    evt->data.evt_mesh_generic_client_server_status.model_id,
                    evt->data.evt_mesh_generic_client_server_status.elem_index,
                    evt->data.evt_mesh_generic_client_server_status.server_address,
                    evt->data.evt_mesh_generic_client_server_status.client_address,
                    evt->data.evt_mesh_generic_client_server_status.flags & 0x1 ? "Yes" : "No");
  }
  if (evt->data.evt_mesh_generic_client_server_status.type == mesh_generic_state_on_off) {
    bt_shell_printf("  Type: ONOFF Status\n"
                    "    Value: %s\n",
                    evt->data.evt_mesh_generic_client_server_status.parameters.data[0] ? "ON" : "OFF");
  } else if (evt->data.evt_mesh_generic_client_server_status.type == mesh_lighting_state_lightness_actual) {
    bt_shell_printf("  Type: Lightness Status\n"
                    "    Value: %.2f%%\n",
                    uint16_from_buf(evt->data.evt_mesh_generic_client_server_status.parameters.data) * 100 / 65535.0);
  } else if (evt->data.evt_mesh_generic_client_server_status.type == mesh_lighting_state_ctl_temperature) {
    bt_shell_printf("  Type: CTL Status\n"
                    "    Color Temperature: %dK\n"
                    "    Delta UV: 0x%d\n",
                    uint16_from_buf(evt->data.evt_mesh_generic_client_server_status.parameters.data),
                    int16_from_buf(evt->data.evt_mesh_generic_client_server_status.parameters.data + 2));
  }
}

int model_evt_hdr(const struct gecko_cmd_packet *evt)
{
  uint32_t evt_id = BGLIB_MSG_ID(evt->header);

  if ((evt_id & 0x00ff0000) == 0x004c0000) {
    lc_client_evt_hdr(evt_id, evt);
  } else if ((evt_id & 0x00ff0000) == 0x001e0000) {
    LOGD("Generic Light event\n");
    generic_light_client_evt_hdr(evt_id, evt);
  } else {
    return 0;
  }
  return 1;
}
