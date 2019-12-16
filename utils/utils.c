/*************************************************************************
    > File Name: utils.c
    > Author: Kevin
    > Created Time: 2019-12-16
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <string.h>

#include "utils.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

char *strdelimit(char *str, char *del, char c)
{
  char *dup;

  if (!str) {
    return NULL;
  }

  dup = strdup(str);
  if (dup[0] == '\0') {
    return dup;
  }

  while (del[0] != '\0') {
    char *rep = dup;

    while ((rep = strchr(rep, del[0])))
      rep[0] = c;

    del++;
  }

  return dup;
}

int strsuffix(const char *str, const char *suffix)
{
	int len;
	int suffix_len;

	if (!str || !suffix)
		return -1;

	if (str[0] == '\0' && suffix[0] != '\0')
		return -1;

	if (suffix[0] == '\0' && str[0] != '\0')
		return -1;

	len = strlen(str);
	suffix_len = strlen(suffix);
	if (len < suffix_len)
		return -1;

	return strncmp(str + len - suffix_len, suffix, suffix_len);
}
