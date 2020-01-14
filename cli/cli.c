/*************************************************************************
    > File Name: cli/cli.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>

#include <wordexp.h>

#include <readline/history.h>

#include "projconfig.h"
#include "cli.h"
#include "cfg.h"
#include "mng.h"

#include "startup.h"
#include "utils.h"
#include "err.h"
#include "logging.h"

/* Defines  *********************************************************** */
#define DECLARE_CB(name)  static err_t clicb_##name(int argc, char *argv[])

#define SHELL_NAME RTT_CTRL_TEXT_BRIGHT_BLUE "NWK-Mng$ " RTT_CTRL_RESET
#define RL_HISTORY  ".history"
#define DECLARE_VAGET_FUN(name) static err_t vaget_##name(void *vap, int inbuflen, int *ulen, int *rlen)

#define VAP_LEN 256

#define foreach_cmds(i) for (int i = 0; i < cmd_num; i++)

/* Global Variables *************************************************** */
extern jmp_buf initjmpbuf;
extern pthread_mutex_t qlock;
extern err_t cmd_ret;

/* Static Variables *************************************************** */
DECLARE_VAGET_FUN(addrs);
DECLARE_VAGET_FUN(onoff_lights_addrs);
DECLARE_VAGET_FUN(lightness_lights_addrs);
DECLARE_VAGET_FUN(ctl_lights_addrs);

DECLARE_CB(reset);
DECLARE_CB(help);
DECLARE_CB(quit);
DECLARE_CB(scan);

const command_t commands[] = {
  /* CLI local commands */
  { "reset", "<1/0>", clicb_reset,
    "[Factory] Reset the device" },
  { "q", NULL, clicb_quit,
    "Quit the program" },
  { "help", NULL, clicb_help,
    "Print help" },

  /* Commands need to be handled by mng */
  { "sync", "[1/0]", clicb_sync,
    "Synchronize network configuration" },
  { "list", NULL, clicb_list,
    "List all devices in the database" },
  { "info", "[addr...]", clicb_info,
    "Show the node(s) information in the database",
    NULL, NULL, vaget_addrs },
  { "freemode", "[on/off]", clicb_scan,
    "Turn on/off unprovisioned beacon scanning" },
  { "rmall", NULL, clicb_rmall,
    "Remove all the nodes" },
  { "clrrb", NULL, clicb_rmblclr,
    "Clear the rm_bl fields of all the nodes" },
  { "seqset",
    "[a--/r--/b--/ar-/ab-/ra-/rb-/ba-/br-/arb/abr/rab/rba/bar/bra]",
    clicb_seqset,
    "Set the action loading priority\n"
    "\ta - adding devices\n"
    "\tr - removing devices\n"
    "\tb - blacklisting devices" },

  /* Light Control Commands */
  { "onoff", "[on/off] [addr...]", clicb_onoff,
    "Set the onoff of a light", NULL, NULL, vaget_onoff_lights_addrs },
  { "lightness", "[pecentage] [addr...]", clicb_lightness,
    "Set the lightness of a light", NULL, NULL, vaget_lightness_lights_addrs },
  { "colortemp", "[pecentage] [addr...]", clicb_ct,
    "Set the color temperature of a light", NULL, NULL, vaget_ctl_lights_addrs },
};

const size_t cli_cmd_num = 3;
static const size_t cmd_num = sizeof(commands) / sizeof(command_t);

static char *line = NULL;
static int reset = 0;

/* Static Functions Declaractions ************************************* */
char **shell_completion(const char *text, int start, int end);

#define ADDRULEN  7
static err_t _vaget_lights_addrs(void *vap,
                                 int inbuflen,
                                 int *ulen,
                                 int *rlen,
                                 uint8_t type)
{
  if (!vap || !inbuflen || !ulen || !rlen) {
    return err(ec_param_null);
  }
  char *p = vap;
  uint16list_t *addrs;
  *ulen = ADDRULEN;
  addrs = get_lights_addrs(type);
  if (!addrs) {
    return ec_success;
  }

  if (addrs->len * (*ulen) > inbuflen ) {
    addrs->len = inbuflen / (*ulen);
  }
  *rlen = addrs->len * (*ulen);

  while (addrs->len--) {
    addr_to_buf(addrs->data[addrs->len], &p[addrs->len * (*ulen)]);
  }
  free(addrs->data);
  free(addrs);
  return ec_success;
}

static err_t vaget_onoff_lights_addrs(void *vap,
                                      int inbuflen,
                                      int *ulen,
                                      int *rlen)
{
  return _vaget_lights_addrs(vap, inbuflen, ulen, rlen, onoff_support);
}

static err_t vaget_lightness_lights_addrs(void *vap,
                                          int inbuflen,
                                          int *ulen,
                                          int *rlen)
{
  return _vaget_lights_addrs(vap, inbuflen, ulen, rlen, lightness_support);
}

static err_t vaget_ctl_lights_addrs(void *vap,
                                    int inbuflen,
                                    int *ulen,
                                    int *rlen)
{
  return _vaget_lights_addrs(vap, inbuflen, ulen, rlen, ctl_support);
}

static err_t vaget_addrs(void *vap,
                         int inbuflen,
                         int *ulen,
                         int *rlen)
{
  if (!vap || !inbuflen || !ulen || !rlen) {
    return err(ec_param_null);
  }
  char *p = vap;
  uint16list_t *addrs;

  *ulen = ADDRULEN;

  addrs = get_node_addrs();
  if (!addrs) {
    return ec_success;
  }

  if (addrs->len * (*ulen) > inbuflen ) {
    addrs->len = inbuflen / (*ulen);
  }
  *rlen = addrs->len * (*ulen);

  while (addrs->len--) {
    addr_to_buf(addrs->data[addrs->len], &p[addrs->len * (*ulen)]);
  }
  free(addrs->data);
  free(addrs);
  return ec_success;
}

err_t cli_init(void *p)
{
  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = shell_completion;
  setlinebuf(stdout);
  rl_erase_empty_line = 1;
  using_history();
  read_history(RL_HISTORY);
  LOGD("CLI Init Done\n");
  return ec_success;
}

/* Generator function for command completion.  STATE lets us know whether
 * to start from scratch; without any state (i.e. STATE == 0), then we
 * start at the top of the list. */
char *command_generator(const char *text, int state)
{
  static int list_index, len;
  const char *name;

  /* If this is a new word to complete, initialize now.  This includes
   * saving the length of TEXT for efficiency, and initializing the
   * index variable to 0. */
  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  /* Return the next name which partially matches from the command list.
   * */
  for (int i = list_index; i < cmd_num; i++) {
    name = commands[i].name;
    list_index++;

    if (strncasecmp(name, text, len) == 0) {
      return (strdup(name));
    }
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

static int parse_args(char *arg, wordexp_t *w, char *del, int flags)
{
  char *str;

  str = strdelimit(arg, del, '"');

  if (wordexp(str, w, flags)) {
    free(str);
    return -EINVAL;
  }

  /* If argument ends with ... set we_offs bypass strict checks */
  if (w->we_wordc && !strsuffix(w->we_wordv[w->we_wordc - 1], "...")) {
    w->we_offs = 1;
  }

  free(str);
  return 0;
}

static wordexp_t args;

static char *arg_generator(const char *text, int state)
{
  static unsigned int index, len;
  const char *arg;

  if (!state) {
    index = 0;
    len = strlen(text);
  }

  while (index < args.we_wordc) {
    arg = args.we_wordv[index];
    index++;

    if (!strncmp(arg, text, len)) {
      return strdup(arg);
    }
  }

  return NULL;
}

static struct {
  char va_param[VAP_LEN];
  int len;
  int ulen;
  va_param_get_func_t *pvpget;
} vaget;

static char *var_arg_generator(const char *text, int state)
{
  static unsigned int index, len;
  const char *arg;

  if (!state) {
    if (!vaget.pvpget) {
      return NULL;
    }
    index = 0;
    len = strlen(text);
    memset(vaget.va_param, 0, VAP_LEN);
    vaget.len = 0;
    if (ec_success != (*vaget.pvpget)(vaget.va_param, VAP_LEN, &vaget.ulen, &vaget.len)) {
      return NULL;
    }
  }

  while (index < vaget.len / vaget.ulen) {
    arg = &vaget.va_param[index * vaget.ulen];
    index++;

    if (!strncmp(arg, text, len)) {
      return strdup(arg);
    }
  }

  return NULL;
}

static char **args_completion(const command_t *entry,
                              int argc,
                              const char *text)
{
  char **matches = NULL;
  char *str;
  int index;

  index = text[0] == '\0' ? argc - 1 : argc - 2;
  if (index < 0) {
    return NULL;
  }

  if (!entry->arg) {
    goto end;
  }

  str = strdup(entry->arg);

  if (parse_args(str, &args, "<>[]", WRDE_NOCMD)) {
    goto done;
  }

  /* Check if argument is valid */
  if (args.we_offs == 0) {
    /* No variable number args */
    if ((unsigned) index > args.we_wordc - 1) {
      goto done;
    }
  } else {
    if ((unsigned) index >= args.we_wordc - 1) {
      goto var_handler;
    }
  }

  if (!strrchr(args.we_wordv[index], '/')) {
    goto done;
  }

  free(str);
  /* Split values separated by / */
  str = strdelimit(args.we_wordv[index], "/", ' ');

  args.we_offs = 0;
  wordfree(&args);

  if (wordexp(str, &args, WRDE_NOCMD)) {
    goto done;
  }

  rl_completion_display_matches_hook = NULL;
  matches = rl_completion_matches(text, arg_generator);
  goto done;

  var_handler:
  if (entry->vpget) {
    vaget.pvpget = (va_param_get_func_t *)&entry->vpget;
    rl_completion_display_matches_hook = NULL;
    matches = rl_completion_matches(text, var_arg_generator);
  }
  done:
  free(str);
  end:
  if (!matches && text[0] == '\0') {
    bt_shell_printf("Usage: %s %s\n", entry->name,
                    entry->arg ? entry->arg : "");
  }

  args.we_offs = 0;
  wordfree(&args);
  return matches;
}

static char **menu_completion(const char *text,
                              int argc,
                              char *input_cmd)
{
  char **matches = NULL;

  for (int i = 0; i < cmd_num; i++) {
    const command_t *cmd = &commands[i];
    if (strcmp(cmd->name, input_cmd)) {
      continue;
    }

    if (!cmd->argcmpl) {
      matches = args_completion(cmd, argc, text);
      break;
    }

    rl_completion_display_matches_hook = cmd->disp;
    matches = rl_completion_matches(text, cmd->argcmpl);
    break;
  }

  return matches;
}

char **shell_completion(const char *text, int start, int end)
{
  char **matches = NULL;

  if (start == 0) {
    rl_completion_display_matches_hook = NULL;
    matches = rl_completion_matches(text, command_generator);
  } else {
    wordexp_t w;
    if (wordexp(rl_line_buffer, &w, WRDE_NOCMD)) {
      return NULL;
    }

    matches = menu_completion(text, w.we_wordc,
                              w.we_wordv[0]);
    wordfree(&w);
  }
  if (!matches) {
    rl_attempted_completion_over = 1;
  }

  return (matches);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
 * into STRING. */
char *stripwhite(char *string)
{
  register char *s, *t;

  for (s = string; whitespace(*s); s++) {
  }

  if (*s == 0) {
    return (s);
  }

  t = s + strlen(s) - 1;
  while (t > s && whitespace(*t))
    t--;
  *++t = '\0';

  return s;
}

static inline int find_cmd_index(const char *cmd)
{
  int pos;
  for (pos = 0; pos < cmd_num; pos++) {
    if (!strncmp(cmd, commands[pos].name, strlen(commands[pos].name))) {
      return pos;
    }
  }
  return -1;
}

static void output_nspt(const char *cmd)
{
  print_text(COLOR_HIGHLIGHT,
             "Invalid command: %s",
             cmd);
  print_text(COLOR_HIGHLIGHT,
             "\n"
             "Use \"help\" for a list of available commands");
}

void readcmd(void)
{
  char *str;
  int ret;
  wordexp_t w;

  if (line) {
    free(line);
    line = NULL;
  }
  line = readline(SHELL_NAME);
  if (!line) {
    return;
  }
  /* Remove leading and trailing whitespace from the line.
   * Then, if there is anything left, add it to the history list
   * and execute it. */
  str = stripwhite(line);

  if (wordexp(str, &w, WRDE_NOCMD)) {
    return;
  }
  if (!str || str[0] == '\0') {
    goto out;
  }

  ret = find_cmd_index(str);
  if (ret == -1) {
    output_nspt(w.we_wordv[0]);
    goto out;
  }
  DUMP_PARAMS(w.we_wordc, w.we_wordv);
  ret = commands[ret].fn(w.we_wordc, w.we_wordv);
  if (ec_param_invalid == ret) {
    printf(COLOR_HIGHLIGHT "Invalid Parameter(s)\nUsage: " COLOR_OFF);
    print_cmd_usage(&commands[ret]);
    goto out;
  }

  out:
  if (*str) {
    add_history(str);
    write_history(RL_HISTORY);
  }
  wordfree(&w);
}

err_t cli_proc_init(int child_num, const pid_t *pids)
{
  err_t e;
  if (ec_success != (e = logging_init(CLI_LOG_FILE_PATH,
                                      0, /* Not output to stdout */
                                      LOG_MINIMAL_LVL(LVL_VER)))) {
    fprintf(stderr, "LOG INIT ERROR (%x)\n", e);
    return e;
  }
  return e;
}

#if 0
static void rl_handler(char *input)
{
  char *str;
  int ret;
  wordexp_t w;

  /* Remove leading and trailing whitespace from the line.
   * Then, if there is anything left, add it to the history list
   * and execute it. */
  str = stripwhite(input);
  if (wordexp(str, &w, WRDE_NOCMD)) {
    goto done;
  }
  if (!str || str[0] == '\0') {
    goto out;
  }
  ret = find_cmd_index(str);
  if (ret == -1) {
    output_nspt(w.we_wordv[0]);
    goto out;
  }
  DUMP_PARAMS(w.we_wordc, w.we_wordv);
  /* Handle the command - call the callback */
  ret = commands[ret].fn(w.we_wordc, w.we_wordv);
  if (ec_param_invalid == ret) {
    printf(COLOR_HIGHLIGHT "Invalid Parameter(s)\nUsage: " COLOR_OFF);
    print_cmd_usage(&commands[ret]);
  }

  out:
  if (*str) {
    add_history(str);
    write_history(RL_HISTORY);
  }
  wordfree(&w);
  done:
  free(input);
}
void *cli_mainloop(void *pIn)
{
  int ret = 0, rl_saved = 0;
  struct timeval tv = { 0 };
  int stdinfd, maxfd = -1, r;
  fd_set rset, allset;

  rl_readline_name = SHELL_NAME;
  setlinebuf(stdout);
  rl_erase_empty_line = 1;
  rl_callback_handler_install(SHELL_NAME, rl_handler);

  FD_ZERO(&allset);
  stdinfd = fileno(stdin);
  FD_SET(stdinfd, &allset);
  maxfd = MAX(stdinfd, maxfd);

  while (1) {
    rset = allset;   /* rset gets modified each time around */
    if (rl_saved && get_mng()->state <= configured) {
      LOGD("restore\n");
      rl_replace_line("", 0);
      rl_restore_prompt();
      rl_saved = 0;
      rl_redisplay();
    }
    if ((r = select(maxfd + 1, &rset, NULL, NULL, ret ? &tv : NULL)) < 0) {
      LOGE("Select returns [%d], err[%s]\n", r, strerror(errno));
      return (void *)(uintptr_t)err(ec_sock);
    }

    if (get_mng()->state <= configured) {
      if (FD_ISSET(stdinfd, &rset)) {
        LOGD("read\n");
        rl_callback_read_char();
      }
    }
    if (!rl_saved && get_mng()->state > configured) {
      LOGD("save\n");
      rl_save_prompt();
      rl_replace_line("", 0);
      rl_redisplay();
      rl_saved = 1;
    }
    ret = (mng_mainloop(NULL) != NULL);
  }
  return NULL;
}
#else
void *cli_mainloop(void *pin)
{
  err_t e;
  char *str;
  int ret;
  wordexp_t w;

  while (1) {
    if (line) {
      free(line);
      line = NULL;
    }
    if (NULL == (line = readline(SHELL_NAME))) {
      continue;
    }
    /* Remove leading and trailing whitespace from the line.
     * Then, if there is anything left, add it to the history list
     * and execute it. */
    str = stripwhite(line);
    if (wordexp(str, &w, WRDE_NOCMD)) {
      continue;
    }
    if (!str || str[0] == '\0') {
      goto out;
    }
    DUMP_PARAMS(w.we_wordc, w.we_wordv);
    ret = find_cmd_index(str);
    if (ret == -1) {
      output_nspt(w.we_wordv[0]);
      goto out;
    } else if (ret < 3) {
      /* Locally handled */
      e = commands[ret].fn(w.we_wordc, w.we_wordv);
      if (ec_param_invalid == e) {
        printf(COLOR_HIGHLIGHT "Invalid Parameter(s)\nUsage: " COLOR_OFF);
        print_cmd_usage(&commands[ret]);
        goto out;
      }
    } else {
      /* Need the mng component to handle */
      cmd_enq(str, ret);
    }

    out:
    if (*str) {
      add_history(str);
      write_history(RL_HISTORY);
    }
    wordfree(&w);
    if (reset) {
      int r = reset;
      reset = 0;
      longjmp(initjmpbuf, (r == 2) ? FACTORY_RESET : NORMAL_RESET);
    }
  }
  return NULL;
}
#endif

void bt_shell_printf(const char *fmt, ...)
{
  va_list args;
  bool save_input;
  char *saved_line;
  int saved_point;

  /* if (!data.input) */
  /* return; */

  /* if (data.mode) { */
  /* va_start(args, fmt); */
  /* vprintf(fmt, args); */
  /* va_end(args); */
  /* return; */
  /* } */

  save_input = !RL_ISSTATE(RL_STATE_DONE);

  if (save_input) {
    saved_point = rl_point;
    saved_line = rl_copy_text(0, rl_end);
    /* if (!data.saved_prompt) */
    rl_save_prompt();
    rl_replace_line("", 0);
    rl_redisplay();
  }

  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  /* if (data.monitor) { */
  /* va_start(args, fmt); */
  /* bt_log_vprintf(0xffff, data.name, LOG_INFO, fmt, args); */
  /* va_end(args); */
  /* } */

  if (save_input) {
    /* if (!data.saved_prompt) */
    rl_restore_prompt();
    rl_replace_line(saved_line, 0);
    rl_point = saved_point;
    rl_forced_update_display();
    free(saved_line);
  }
}

static err_t clicb_reset(int argc, char *argv[])
{
  int r = 0;
  if (argc > 1) {
    r = atoi(argv[1]);
  }
  bt_shell_printf("%s\n", __FUNCTION__);
  reset = r + 1;
  return 0;
}

static err_t clicb_help(int argc, char *argv[])
{
  print_text(COLOR_HIGHLIGHT, "Available commands:");
  print_text(COLOR_HIGHLIGHT, "-------------------");
  foreach_cmds(i)
  {
    print_cmd_usage(&commands[i]);
  }
  return 0;
}

static err_t clicb_scan(int argc, char *argv[])
{
  int onoff = 1;
  if (argc > 1) {
    if (!strcmp(argv[1], "on")) {
      onoff = 2;
    } else if (!strcmp(argv[1], "off")) {
      onoff = 0;
    } else {
      return err(ec_param_invalid);
    }
  }
  return clm_set_scan(onoff);
}

static err_t clicb_quit(int argc, char *argv[])
{
  bt_shell_printf("%s\n", __FUNCTION__);
  exit(EXIT_SUCCESS);
}
