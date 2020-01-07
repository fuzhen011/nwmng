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
void cli_print_dev(const node_t *node,
                   const struct gecko_msg_mesh_prov_ddb_get_rsp_t *e)
{
  char buf[TMP_BUF_LEN] = { 0 };
  int ofs = 0;

  ASSERT(e && node);
  bt_shell_printf(DEV_INFO);

  snprintf(buf + ofs, TMP_BUF_LEN - ofs, "%s%s", DEV_PADDING, UUID_INFO);
  ofs += strlen(DEV_PADDING) + strlen(UUID_INFO);

  for (int i = 0; i < 16; i++) {
    if (i == 13) {
      buf[ofs++] = '-';
    }
    snprintf(buf + ofs, TMP_BUF_LEN - ofs, "%02x", node->uuid[i]);
    ofs += 2;
    if (i == 9) {
      buf[ofs++] = '-';
    }
  }
  buf[ofs++] = '\n';
  bt_shell_printf(buf);

  memset(buf, 0, sizeof(buf));
  ofs = 0;
  snprintf(buf + ofs, TMP_BUF_LEN - ofs, "%s%s", DEV_PADDING, DEV_KEY_INFO);
  ofs += strlen(DEV_PADDING) + strlen(DEV_KEY_INFO);
  for (int i = 0; i < 16; i++) {
    snprintf(buf + ofs, TMP_BUF_LEN - ofs, "%02x", e->device_key.data[i]);
    ofs += 2;
  }
  buf[ofs++] = '\n';
  bt_shell_printf(buf);
}
