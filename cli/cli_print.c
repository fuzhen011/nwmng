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
  if (type == ONOFF_SV_BIT) {
    snprintf(buf, TMP_BUF_LEN, "Set 0x%04x OnOff -> %s\n", addr, value ? "ON" : "OFF");
  } else {
    snprintf(buf, TMP_BUF_LEN, "Set 0x%04x %s -> %d%%\n", addr,
             type == LIGHTNESS_SV_BIT ? "Lightness" : "CTL",
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
  "nil",
  "initialized",
  "Idle",
  "starting",
  "Adding Devices",
  "Configuring Nodes",
  "Removing Nodes",
  "Blacklisting Nodes",
  "stopping",
  "state_reload"
};

const char *loglvls[] = { "Assert", "Error", "Warning", "Message", "Debug", "Verbose" };

void cli_status(const mng_t *mng)
{
  int used = 0;
  int loglvl;

  for (int i = 0; i < MAX_PROV_SESSIONS; i++) {
    if (mng->cache.add[i].busy) {
      used++;
    }
  }

  loglvl = get_logging_lvl_threshold();

  bt_shell_printf("State                 = %s\n", states[mng->state]);
  bt_shell_printf("Used adding    caches = %d\n", used);
  bt_shell_printf("Used config/rm caches = %d\n", utils_popcount(mng->cache.config.used));
  bt_shell_printf("Blacklisting          = %s\n", mng->cache.bl.state == bl_idle ? "Idle" : "Busy");
  bt_shell_printf("Action Sequence       = %s\n", mng->status.seq.prios);
  bt_shell_printf("Node(s) to set state  = %d\n", g_list_length(mng->cache.model_set.nodes));
  bt_shell_printf("Free mode             = %s\n", mng->status.free_mode == 2 ? "On" : "Off");
  bt_shell_printf("Logging Threshold     = %s\n", loglvls[loglvl + 1]);
  bt_shell_printf("[%d-%d-%d-%d] to be [added-configured-removed-blacklisted]\n",
                  g_list_length(mng->lists.add),
                  g_list_length(mng->lists.config),
                  g_list_length(mng->lists.rm),
                  g_list_length(mng->lists.bl));
}

void cli_print_stat(const stat_t *s)
{
  unsigned t, h, m;
  if (s->add.time.state == rc_end) {
    /* Add valid, output it */
    t = s->add.time.end - s->add.time.start;
    h = t / 3600;
    t %= 3600;
    m = t / 60;
    t %= 60;

    bt_shell_printf("  Adding devices summary:\n"
                    "    number          : %u\n"
                    "    failures        : %u\n"
                    "    time            : %u:%u:%u\n",
                    s->add.dev_cnt,
                    s->add.fail_times,
                    h, m, t
                    );
  }

  if (s->bl.time.state == rc_end) {
    /* Blacklist valid, output it */
    t = s->bl.time.end - s->bl.time.start;
    h = t / 3600;
    t %= 3600;
    m = t / 60;
    t %= 60;

    bt_shell_printf("  Blacklisting devices summary:\n"
                    /* "    number  : %u\n" */
                    /* "    failures: %u\n" */
                    "    time            : %u:%u:%u\n",
                    /* s->bl.dev_cnt, */
                    /* s->bl.fail_times, */
                    h, m, t
                    );
  }

  if (s->rm.time.state == rc_end) {
    /* Removing valid, output it */
    t = s->rm.time.end - s->rm.time.start;
    h = t / 3600;
    t %= 3600;
    m = t / 60;
    t %= 60;

    bt_shell_printf("  Removing devices summary:\n"
                    "    number          : %u\n"
                    "    retry times     : %u\n"
                    "    time            : %u:%u:%u\n",
                    s->rm.dev_cnt,
                    s->rm.retry_times,
                    h, m, t
                    );
  }

  if (s->config.time.state == rc_end) {
    /* Config valid, output it */
    int l = -1;
    float full_loading_perc;

    t = s->config.time.end - s->config.time.start;

    if (s->config.full_loading.meas.state == rc_end) {
      /* Full Loading data valid */
      l = s->config.full_loading.time;
      full_loading_perc = (float)l * 100.0 / (float)t;
    }

    h = t / 3600;
    t %= 3600;
    m = t / 60;
    t %= 60;

    bt_shell_printf("  Config devices summary:\n"
                    "    number          : %u\n"
                    "    retry times     : %u\n"
                    "    time            : %u:%u:%u\n",
                    s->config.dev_cnt,
                    s->config.retry_times,
                    h, m, t
                    );
    if (l != -1) {
      bt_shell_printf("    full loading % : %.1f%%\n",
                      full_loading_perc
                      );
    }
  }
}
