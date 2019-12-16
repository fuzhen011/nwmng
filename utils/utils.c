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
