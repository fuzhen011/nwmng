/*************************************************************************
    > File Name: cli_print.c
    > Author: Kevin
    > Created Time: 2020-01-07
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>

#include "cli.h"
#include "logging.h"
#include "utils.h"
/* Defines  *********************************************************** */
#define DEV_INFO      "Dev Info:\n"
#define DEV_PADDING   "         "
#define DEV_KEY_INFO  "DevKey --- "
#define DEV_KEY_LSB   "DevKey(LSB for NA import) - "
#define UUID_INFO     "UUID   --- "

#define TMP_BUF_LEN 0xff

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
void cli_print_modelset_done(uint16_t addr, uint8_t type, uint8_t value)
{
  char buf[TMP_BUF_LEN] = { 0 };
  if (type == onoff_support) {
    snprintf(buf, TMP_BUF_LEN, "Set 0x%04x OnOff -> %s\n", addr, value ? "ON" : "OFF");
  } else {
    snprintf(buf, TMP_BUF_LEN, "Set 0x%04x %s -> %d%%\n", addr,
             type == lightness_support ? "Lightness" : "CTL",
             value);
  }
  bt_shell_printf("%s", buf);
}

void cli_print_busy(void)
{
  bt_shell_printf("Device is busy and cannot issue the command.\n");
}

void cli_print_dev(const node_t *node,
                   const struct gecko_msg_mesh_prov_ddb_get_rsp_t *e)
{
  char buf[TMP_BUF_LEN] = { 0 };
  int ofs = 0;

  ASSERT(e && node);
  bt_shell_printf(DEV_INFO);

  snprintf(buf + ofs, TMP_BUF_LEN - ofs, "%s%s", DEV_PADDING, UUID_INFO);
  ofs += strlen(DEV_PADDING) + strlen(UUID_INFO);
  ofs += fmt_uuid(buf + ofs, node->uuid);
  buf[ofs++] = '\n';
  bt_shell_printf(buf);

  memset(buf, 0, sizeof(buf));
  ofs = 0;
  snprintf(buf + ofs, TMP_BUF_LEN - ofs, "%s%s", DEV_PADDING, DEV_KEY_INFO);
  ofs += strlen(DEV_PADDING) + strlen(DEV_KEY_INFO);
  ofs += fmt_key(buf + ofs, e->device_key.data);
  buf[ofs++] = '\n';
  bt_shell_printf(buf);
}

void cli_list_nodes(uint16list_t *ul)
{
  node_t *n;
  char buf[TMP_BUF_LEN] = { 0 };
  int ofs = 0;
  if (!ul) {
    return;
  }
  bt_shell_printf("%d Nodes\n", ul->len);

  while (ul->len) {
    n = cfgdb_node_get(ul->data[ul->len - 1]);
    buf[ofs++] = '\t';
    buf[ofs++] = ' ';
    ofs += fmt_uuid(buf + ofs, n->uuid);
    sprintf(buf + ofs, " --- 0x");
    ofs += strlen(" --- 0x");
    uint16_tostr(n->addr, buf + ofs);
    ofs += 5;
    bt_shell_printf("%s\n", buf);
    ul->len--;
    ofs = 0;
  }
}

const char *states[] = {
  "initialized",
  "configured",
  "starting",
  "adding_devices_em",
  "configuring_devices_em",
  "removing_devices_em",
  "blacklisting_devices_em",
  "stopping",
  "state_reload"
};

void cli_status(const mng_t *mng)
{
  int used = 0;

  for (int i = 0; i < MAX_PROV_SESSIONS; i++) {
    if (mng->cache.add[i].busy) {
      used++;
    }
  }

  bt_shell_printf("State                 = %s\n", states[mng->state]);
  bt_shell_printf("Used adding caches    = %d\n", used);
  bt_shell_printf("Used config/rm caches = %d\n", utils_popcount(mng->cache.config.used));
  bt_shell_printf("Blacklisting          = %s\n", mng->cache.bl.state == bl_idle ? "Idle" : "Busy");
  bt_shell_printf("Action Sequence       = %s\n", mng->status.seq.prios);
  bt_shell_printf("Node(s) to set state  = %d\n", g_list_length(mng->cache.model_set.nodes));
  bt_shell_printf("Free mode             = %s\n", mng->status.free_mode == 2 ? "On" : "Off");
  bt_shell_printf("[%d-%d-%d-%d] to be [added-configured-removed-blacklisted]\n",
                  g_list_length(mng->lists.add),
                  g_list_length(mng->lists.config),
                  g_list_length(mng->lists.rm),
                  g_list_length(mng->lists.bl));
}
