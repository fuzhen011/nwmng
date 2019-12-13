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

#include <unistd.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "cli/cli.h"

/* Defines  *********************************************************** */
#define RL_HISTORY  ".history"
#define CMD_ENTRY(name, pfncb, doc) { (name), (pfncb), (doc) },
#define DECLARE_CB(name)  int ccb_##name(char *str)

typedef struct {
  const char *name;
  rl_icpfunc_t *fn;
  const char *doc;
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
  CMD_ENTRY("sync", ccb_sync,
            "Synchronize the configuration of the network with the JSON file")
  CMD_ENTRY("reset", ccb_reset,
            "[Factory] Reset the device")
  CMD_ENTRY("q", ccb_quit,
            "Quit the program")
  CMD_ENTRY("help", ccb_help,
            "Print help")
  CMD_ENTRY("list", ccb_list,
            "List all devices in the database")

  CMD_ENTRY("onoff", ccb_onoff,
            "Set the onoff of a light")
  CMD_ENTRY("lightness", ccb_lightness,
            "Set the lightness of a light")
  CMD_ENTRY("colortemp", ccb_ct,
            "Set the color temperature of a light")
};
static const size_t cmd_num = sizeof(commands) / sizeof(command_t);

static int children_num = 0;
static pid_t *children = NULL;
static char *line = NULL;

/* Static Functions Declaractions ************************************* */
char **cmd_cmpl(const char *text, int start, int end);

static void cli_init(void)
{
  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = cmd_cmpl;
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

char **cmd_cmpl(const char *text, int start, int end)
{
  char **matches;
  matches = (char **)NULL;

  /* printf("\ntext[%s][%d:%d]\n", text, start, end); */
  /* If this word is at the start of the line, then it is a command
   *      to complete.  Otherwise it is the name of a file in the current
   *           directory. */
  if (start == 0) {
    matches = rl_completion_matches(text, command_generator);
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
  children = calloc(child_num, sizeof(pid_t));
  memcpy(children, pids, child_num * sizeof(pid_t));
  children_num = child_num;

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

