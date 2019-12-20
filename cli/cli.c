/*************************************************************************
    > File Name: cli/cli.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <wordexp.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "cli/cli.h"
#include "utils.h"
#include "err.h"
#include "logging.h"

/* Defines  *********************************************************** */
#define __DUMP_PARAMS
#ifdef __DUMP_PARAMS
#define DUMP_PARAMS(argc, argv)                            \
  do {                                                     \
    LOGV("CMD - %s with %d params.\n", argv[0], argc - 1); \
    for (int i = 1; i < (argc); i++)                       \
    { LOGV("\tparam[%d]: %s\n", i, (argv)[i]); }           \
  } while (0)
#else
#define DUMP_PARAMS()
#endif
#define DECLARE_CB(name)  static err_t clicb_##name(int argc, char *argv[])

#define SHELL_NAME RTT_CTRL_TEXT_BRIGHT_BLUE "NWmng$ " RTT_CTRL_RESET
#define RL_HISTORY  ".history"
#define DECLARE_VAGET_FUN(name) static err_t vaget_##name(void *vap, int inbuflen, int *ulen, int *rlen)

#define VAP_LEN 256

#define CMD_LENGTH  36
#define print_text(color, fmt, args ...) \
  printf(color fmt COLOR_OFF "\n", ## args)
#define print_cmd_usage(cmd)                                   \
  printf(COLOR_HIGHLIGHT "%s %-*s " COLOR_OFF "%s\n",          \
         (cmd)->name, (int)(CMD_LENGTH - strlen((cmd)->name)), \
         (cmd)->arg ? (cmd)->arg : "",                         \
         (cmd)->doc)

#define foreach_cmds(i) for (int i = 0; i < cmd_num; i++)
typedef err_t (*va_param_get_func_t)(void *vap,
                                     int inbuflen,
                                     int *ulen,
                                     int *rlen);

typedef err_t (*cmd_exec_func_t)(int argc, char *argv[]);

typedef struct {
  const char *name;
  const char *arg;
  cmd_exec_func_t fn;
  const char *doc;
  rl_compdisp_func_t *disp;
  rl_compentry_func_t *argcmpl;
  va_param_get_func_t vpget;
}command_t;

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
DECLARE_VAGET_FUN(addrs);

DECLARE_CB(sync);
DECLARE_CB(reset);
DECLARE_CB(list);
DECLARE_CB(info);
DECLARE_CB(help);
DECLARE_CB(quit);
DECLARE_CB(ct);
DECLARE_CB(lightness);
DECLARE_CB(onoff);

static const command_t commands[] = {
  { "sync", NULL, clicb_sync,
    "Synchronize network configuration" },
  { "reset", "<1/0>", clicb_reset,
    "[Factory] Reset the device" },
  { "q", NULL, clicb_quit,
    "Quit the program" },
  { "help", NULL, clicb_help,
    "Print help" },
  { "list", NULL, clicb_list,
    "List all devices in the database" },
  { "info", "[addr...]", clicb_info,
    "Show the device information in the database",
    NULL, NULL, vaget_addrs },

  /* Light Control Commands */
  { "onoff", "[on/off] [addr...]", clicb_onoff,
    "Set the onoff of a light", NULL, NULL, vaget_addrs },
  { "lightness", "[pecentage] [addr...]", clicb_lightness,
    "Set the lightness of a light", NULL, NULL, vaget_addrs },
  { "colortemp", "[pecentage] [addr...]", clicb_ct,
    "Set the color temperature of a light", NULL, NULL, vaget_addrs },
};

static const size_t cmd_num = sizeof(commands) / sizeof(command_t);

int children_num = 0;
pid_t *children = NULL;
static char *line = NULL;

/* Static Functions Declaractions ************************************* */
char **shell_completion(const char *text, int start, int end);

static int addr_in_cfg(const char *straddr,
                       uint16_t *addr)
{
  /* TODO: socket to CFG to get dev */
  uint16_t addrtmp;
  if (ec_success != str2uint(straddr, strlen(straddr), &addrtmp, sizeof(uint16_t))) {
    LOGD("str2uint failed\n");
    return -1;
  }
  if (addrtmp == 0x1003
      || addrtmp == 0xc005
      || addrtmp == 0x120c
      || addrtmp == 0x100e
      || addrtmp == 0x1 ) {
    *addr = addrtmp;
    return 0;
  }
  return -1;
}

static int dummy_get_addrs(uint16_t *addrs)
{
  addrs[0] = 0x1003;
  addrs[1] = 0xc005;
  addrs[2] = 0x120c;
  addrs[3] = 0x100e;
  addrs[4] = 0x1;
  /* addrs[4] = 0x1003; */
  return 5;
}

#define ADDRULEN  7
static err_t vaget_addrs(void *vap,
                         int inbuflen,
                         int *ulen,
                         int *rlen)
{
  if (!vap || !inbuflen || !ulen || !rlen) {
    return err(ec_param_null);
  }
  char *p = vap;

  *ulen = ADDRULEN;
  uint16_t *addrs = calloc(VAP_LEN / ADDRULEN, sizeof(uint16_t));
  int num = dummy_get_addrs(addrs);
  if (num * (*ulen) > inbuflen ) {
    num = inbuflen / (*ulen);
  }
  *rlen = num * (*ulen);

  while (num--) {
    addr_to_buf(addrs[num], &p[num * (*ulen)]);
  }
  free(addrs);
  return ec_success;
}

static void cli_init(void)
{
  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = shell_completion;
  using_history();
  read_history(RL_HISTORY);
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

  /* if ((unsigned) index > args.we_wordc - 1) { */
  /* if (args.we_offs == 0) { */
  /* goto done; */
  /* } else { */
  /* goto done; */
  /* } */
  /* } */

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
    /* if (!matches) */
    /* matches = menu_completion(data.menu->entries, text, */
    /* w.we_wordc, */
    /* w.we_wordv[0]); */

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

int cli_proc_init(int child_num, const pid_t *pids)
{
  if (children) {
    free(children);
  }

  if (child_num) {
    ASSERT(pids);
    children = calloc(child_num, sizeof(pid_t));
    memcpy(children, pids, child_num * sizeof(pid_t));
    children_num = child_num;
  }

  cli_init();
  return 0;
}

/* static void rl_handler(char *input) */
/* { */
/* printf("111\n"); */
/* } */

int cli_proc(void)
{
  LOGD("CLI started\n");
  for (;;) {
    readcmd();
  }
  /* rl_readline_name = SHELL_NAME; */
  /* setlinebuf(stdout); */
  /* rl_attempted_completion_function = shell_completion; */
  /* rl_erase_empty_line = 1; */
  /* rl_callback_handler_install(NULL, rl_handler); */
  /* using_history(); */
  /* read_history(RL_HISTORY); */
  /* while (1) */
  /* { */
  /* } */
  return 0;
}

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

static err_t clicb_sync(int argc, char *argv[])
{
  sleep(5);
  printf("%s\n", __FUNCTION__);
  return 0;
}

static err_t clicb_reset(int argc, char *argv[])
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

static err_t clicb_list(int argc, char *argv[])
{
  printf("%s\n", __FUNCTION__);
  return err(ec_param_invalid);
}

static err_t clicb_info(int argc, char *argv[])
{
  printf("%s\n", __FUNCTION__);
  return err(ec_param_invalid);
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

static err_t clicb_quit(int argc, char *argv[])
{
  printf("%s\n", __FUNCTION__);
  for (int i = 0; i < children_num; i++) {
    kill(children[i], SIGKILL);
  }
  exit(EXIT_SUCCESS);
}

static err_t clicb_ct(int argc, char *argv[])
{
  err_t e = ec_success;
  unsigned int ct;
  uint16_t *addrs;
  if (argc < 3) {
    return err(ec_param_invalid);
  }

  if (ec_success != (e = str2uint(argv[1],
                                  strlen(argv[1]),
                                  &ct,
                                  sizeof(unsigned int)))) {
    return err(ec_param_invalid);
  }
  if (ct > 100) {
    return err(ec_param_invalid);
  }

  addrs = calloc(argc - 2, sizeof(uint16_t));

  for (int i = 2; i < argc; i++) {
    if (-1 == addr_in_cfg(argv[i], &addrs[i - 2])) {
      LOGW("Onoff invalid addr[%s]\n", argv[i]);
      e = ec_param_invalid;
      break;
    }
  }
  if (e == ec_success) {
    /* TODO */
    /* handle onoff set to addrs */
    LOGD("Sending color temperature [%u] to be handled\n", ct);
    /* IMPL PENDING, now for DEBUG */
    sleep(1);
    printf("Success\n");
    (void)ct;
  }
  free(addrs);
  return e;
}

static err_t clicb_lightness(int argc, char *argv[])
{
  err_t e = ec_success;
  unsigned int lightness;
  uint16_t *addrs;
  if (argc < 3) {
    return err(ec_param_invalid);
  }

  if (ec_success != (e = str2uint(argv[1],
                                  strlen(argv[1]),
                                  &lightness,
                                  sizeof(unsigned int)))) {
    return err(ec_param_invalid);
  }
  if (lightness > 100) {
    return err(ec_param_invalid);
  }
  addrs = calloc(argc - 2, sizeof(uint16_t));

  for (int i = 2; i < argc; i++) {
    if (-1 == addr_in_cfg(argv[i], &addrs[i - 2])) {
      LOGW("Onoff invalid addr[%s]\n", argv[i]);
      e = ec_param_invalid;
      break;
    }
  }
  if (e == ec_success) {
    /* TODO */
    /* handle onoff set to addrs */
    LOGD("Sending lightness [%u] to be handled\n", lightness);
    /* IMPL PENDING, now for DEBUG */
    sleep(1);
    printf("Success\n");
    (void)lightness;
  }
  free(addrs);
  return e;
}

static err_t clicb_onoff(int argc, char *argv[])
{
  err_t e = ec_success;
  int onoff;
  uint16_t *addrs;
  if (argc < 3) {
    return err(ec_param_invalid);
  }
  if (!strcmp(argv[1], "on")) {
    onoff = 1;
  } else if (!strcmp(argv[1], "off")) {
    onoff = 0;
  } else {
    return err(ec_param_invalid);
  }
  addrs = calloc(argc - 2, sizeof(uint16_t));

  for (int i = 2; i < argc; i++) {
    if (-1 == addr_in_cfg(argv[i], &addrs[i - 2])) {
      LOGW("Onoff invalid addr[%s]\n", argv[i]);
      e = ec_param_invalid;
      break;
    }
  }
  if (e == ec_success) {
    /* TODO */
    /* handle onoff set to addrs */
    LOGD("Sending onoff[%s] to be handled\n", onoff ? "on" : "off");
    /* IMPL PENDING, now for DEBUG */
    sleep(1);
    printf("Success\n");
    (void)onoff;
  }
  free(addrs);
  return e;
}
