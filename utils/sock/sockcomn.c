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
 * Wait for a client connection to arrive, and accept it.
 * We also obtain the client's user ID from the pathname
 * that it must bind before calling us.
 * Returns new fd if all OK, <0 on error
 */
int serv_accept(int listenfd, uid_t *uidptr)
{
  int         clifd, err, rval;
  socklen_t     len;
  time_t        staletime;
  struct sockaddr_un  un;
  struct stat     statbuf;
  char        *name;

  /* allocate enough space for longest name plus terminating null */
  if ((name = malloc(sizeof(un.sun_path + 1))) == NULL) {
    return(-1);
  }
  len = sizeof(un);
  if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) {
    free(name);
    return(-2);   /* often errno=EINTR, if signal caught */
  }

  /* obtain the client's uid from its calling address */
  len -= offsetof(struct sockaddr_un, sun_path); /* len of pathname */
  memcpy(name, un.sun_path, len);
  name[len] = 0;      /* null terminate */
  if (stat(name, &statbuf) < 0) {
    rval = -3;
    goto errout;
  }

#ifdef  S_ISSOCK  /* not defined for SVR4 */
  if (S_ISSOCK(statbuf.st_mode) == 0) {
    rval = -4;    /* not a socket */
    goto errout;
  }
#endif

  if ((statbuf.st_mode & (S_IRWXG | S_IRWXO))
      || (statbuf.st_mode & S_IRWXU) != S_IRWXU) {
    rval = -5;    /* is not rwx------ */
    goto errout;
  }

  staletime = time(NULL) - STALE;
  if (statbuf.st_atime < staletime
      || statbuf.st_ctime < staletime
      || statbuf.st_mtime < staletime) {
    rval = -6;    /* i-node is too old */
    goto errout;
  }

  if (uidptr != NULL) {
    *uidptr = statbuf.st_uid; /* return uid of caller */
  }
  unlink(name);   /* we're done with pathname now */
  free(name);
  return(clifd);

  errout:
  err = errno;
  close(clifd);
  free(name);
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
