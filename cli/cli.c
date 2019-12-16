/*************************************************************************
    > File Name: cli/cli.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include <wordexp.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "cli/cli.h"
#include "utils.h"

/* Defines  *********************************************************** */
#define SHELL_NAME "[1;34m" "nwmng$ " "[0m"
#define RL_HISTORY  ".history"
#define DECLARE_CB(name)  int ccb_##name(char *str)

typedef struct {
  const char *name;
  const char *arg;
  rl_icpfunc_t *fn;
  const char *doc;
  rl_compdisp_func_t *disp;
  rl_compentry_func_t *argcmpl;
}command_t;
/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

DECLARE_CB(sync);
DECLARE_CB(reset);
DECLARE_CB(list);
DECLARE_CB(help);
DECLARE_CB(quit);

DECLARE_CB(ct);
DECLARE_CB(lightness);
DECLARE_CB(onoff);

static const command_t commands[] = {
  { "sync", NULL, ccb_sync,
    "Synchronize the configuration of the network with the JSON file" },
  { "sync1", NULL, ccb_sync },
  { "sync2", NULL, ccb_sync },
  { "sync3", NULL, ccb_sync },
  { "sync4", NULL, ccb_sync },
  { "reset", "<1/0>", ccb_reset,
    "[Factory] Reset the device" },
  { "q", NULL, ccb_quit,
    "Quit the program" },
  { "help", NULL, ccb_help,
    "Print help" },
  { "list", NULL, ccb_list,
    "List all devices in the database" },

  /* Light Control Commands */
  { "onoff", "[on/off] [addr...]", ccb_onoff,
    "Set the onoff of a light" },
  { "lightness", "[pecentage] [addr...]", ccb_lightness,
    "Set the lightness of a light" },
  { "colortemp", "[pecentage] [addr...]", ccb_ct,
    "Set the color temperature of a light" },
};

static const size_t cmd_num = sizeof(commands) / sizeof(command_t);

static int children_num = 0;
static pid_t *children = NULL;
static char *line = NULL;

/* Static Functions Declaractions ************************************* */
char **shell_completion(const char *text, int start, int end);

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
  if ((unsigned) index > args.we_wordc - 1) {
    if (args.we_offs == 0) {
      goto done;
    } else {
      goto done;
    }
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

  done:
  free(str);
  end:
  if (!matches && text[0] == '\0') {
    /* printf("Usage: %s %s\n", entry->cmd, */
    /* entry->arg ? entry->arg : ""); */
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
  printf("Command [%s] NOT SUPPORTED\n", cmd);
}

void readcmd(void)
{
  char *str;
  int ret;
  if (line) {
    free(line);
    line = NULL;
  }
  line = readline("[1;34m" "nwmng$ " "[0m");
  if (!line) {
    return;
  }
  /* Remove leading and trailing whitespace from the line.
   * Then, if there is anything left, add it to the history list
   * and execute it. */
  str = stripwhite(line);

  ret = find_cmd_index(str);
  if (ret == -1) {
    output_nspt(str);
    return;
  }
  ret = commands[ret].fn(str);

  if (*str) {
    add_history(str);
    write_history(RL_HISTORY);
  }
}

int cli_proc_init(int child_num, const pid_t *pids)
{
  if (children) {
    free(children);
  }

  if (child_num) {
    assert(pids);
    children = calloc(child_num, sizeof(pid_t));
    memcpy(children, pids, child_num * sizeof(pid_t));
    children_num = child_num;
  }

  cli_init();
  return 0;
}

int cli_proc(void)
{
  for (;;) {
    readcmd();
  }
}

/******************************************************************
 * Callbacks
 * ***************************************************************/
int ccb_sync(char *str)
{
  sleep(5);
  printf("%s\n", __FUNCTION__);
  return 0;
}

int ccb_reset(char *str)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

int ccb_list(char *str)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

int ccb_help(char *str)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

int ccb_quit(char *str)
{
  printf("%s\n", __FUNCTION__);
  for (int i = 0; i < children_num; i++) {
    kill(children[i], SIGKILL);
  }
  exit(EXIT_SUCCESS);
}

int ccb_ct(char *str)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

int ccb_lightness(char *str)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

int ccb_onoff(char *str)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}
