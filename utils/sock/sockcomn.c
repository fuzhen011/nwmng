/*************************************************************************
    > File Name: sockcomn.c
    > Author: Kevin
    > Created Time: 2019-12-15
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "sockcomn.h"
#include "logging.h"
/* Defines  *********************************************************** */
#define QLEN  10
#define STALE 30  /* client's name can't be older than this (sec) */
#define CLI_PERM  S_IRWXU     /* rwx for user only */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

/*
 * Create a server endpoint of a connection.
 * Returns fd if all OK, <0 on error.
 */
int new_socksv(const char *name)
{
  int fd, len, err, rval;
  struct sockaddr_un un;

  if (strlen(name) >= sizeof(un.sun_path)) {
    errno = ENAMETOOLONG;
    return(-1);
  }

  /* create a UNIX domain stream socket */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    return(-2);
  }

  unlink(name); /* in case it already exists */

  /* fill in socket address structure */
  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  strcpy(un.sun_path, name);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

  /* bind the name to the descriptor */
  if (bind(fd, (struct sockaddr *)&un, len) < 0) {
    rval = -3;
    goto errout;
  }

  if (listen(fd, QLEN) < 0) { /* tell kernel we're a server */
    rval = -4;
    goto errout;
  }
  return(fd);

  errout:
  err = errno;
  close(fd);
  errno = err;
  return(rval);
}

/*
 * Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.
 */
int new_sockcl(const char *name,
               const char *sv_name)
{
  int         fd, len, err, rval;
  struct sockaddr_un  un, sun;
  int         do_unlink = 0;

  if (strlen(name) >= sizeof(un.sun_path)) {
    errno = ENAMETOOLONG;
    return(-1);
  }

  /* create a UNIX domain stream socket */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    return(-1);
  }

  /* fill socket address structure with our address */
  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  strcpy(sun.sun_path, name);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

  unlink(un.sun_path);    /* in case it already exists */
  if (bind(fd, (struct sockaddr *)&un, len) < 0) {
    rval = -2;
    goto errout;
  }
  /* Below needs root */
  if (chmod(un.sun_path, CLI_PERM) < 0) {
    rval = -3;
    do_unlink = 1;
    goto errout;
  }

  /* fill socket address structure with server's address */
  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, sv_name);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(sv_name);
  if (connect(fd, (struct sockaddr *)&sun, len) < 0) {
    rval = -4;
    do_unlink = 1;
    goto errout;
  }
  return(fd);

  errout:
  err = errno;
  close(fd);
  if (do_unlink) {
    unlink(un.sun_path);
  }
  errno = err;
  return(rval);
}

int timed_poll(struct pollfd *pl,
               int timeout,
               void *buf,
               int buflen)
{
  poll(pl, 1, timeout == 0 ? -1 : timeout);

  if (pl->revents == 0) {
    return 0;
  }

  int len = recv(pl->fd, buf, buflen, 0);
  if (len == 0) {
    //socket closed
    return -1;
  } else if (len < 0) {
    /* TODO */
    return 0;
  }
  return len;
}
