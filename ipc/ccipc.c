/*************************************************************************
    > File Name: ccipc.c
    > Author: Kevin
    > Created Time: 2019-12-24
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "projconfig.h"
#include "ccipc.h"
#include "logging.h"

/* Defines  *********************************************************** */
#define SRV_QLEN  0

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */

int connect_retry(int domain, int type, int protocol,
                  const struct sockaddr *addr, socklen_t alen)
{
  int numsec, fd;

  /*
   * Try to connect with exponential backoff.
   */
  for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
    if ((fd = socket(domain, type, protocol)) < 0) {
      return(-1);
    }
    if (connect(fd, addr, alen) == 0) {
      /*
       * Connection accepted.
       */
      return(fd);
    }
    close(fd);

    /*
     * Delay before trying again.
     */
    if (numsec <= MAXSLEEP / 2) {
      sleep(numsec);
    }
  }
  return(-1);
}

int initserver(int type, const struct sockaddr *addr, socklen_t alen,
               int qlen)
{
  int fd, err;
  int reuse = 1;

  if ((fd = socket(addr->sa_family, type, 0)) < 0) {
    return(-1);
  }
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                 sizeof(int)) < 0) {
    goto errout;
  }
  if (bind(fd, addr, alen) < 0) {
    goto errout;
  }
  if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
    if (listen(fd, qlen) < 0) {
      goto errout;
    }
  }
  return(fd);

  errout:
  err = errno;
  close(fd);
  errno = err;
  return(-1);
}

/*
 * Create a server endpoint of a connection.
 * Returns fd if all OK, <0 on error.
 */
int serv_listen(const char *name)
{
  int         fd, len, err, rval;
  struct sockaddr_un  un;

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

  if (listen(fd, SRV_QLEN) < 0) { /* tell kernel we're a server */
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

#define STALE 30  /* client's name can't be older than this (sec) */

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

#define CLI_PERM  S_IRWXU     /* rwx for user only */

/*
 * Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.
 */
int cli_conn(const char *self, const char *srv)
{
  int         fd, len, err, rval;
  struct sockaddr_un  un, sun;
  int         do_unlink = 0, reuse = 1;

  if (strlen(self) >= sizeof(un.sun_path)) {
    errno = ENAMETOOLONG;
    return(-1);
  }

  /* create a UNIX domain stream socket */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    /* LOGE("socket\n"); */
    return(-1);
  }
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
    rval = -5;
    goto errout;
  }

  /* fill socket address structure with our address */
  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  /* sprintf(un.sun_path, "%s%05ld", CLI_PATH, (long)getpid()); */
  /* printf("file is %s\n", un.sun_path); */
  strcpy(un.sun_path, self);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

  unlink(un.sun_path);    /* in case it already exists */
  if (bind(fd, (struct sockaddr *)&un, len) < 0) {
    /* LOGE("bind\n"); */
    rval = -2;
    goto errout;
  }
  if (chmod(un.sun_path, CLI_PERM) < 0) {
    /* LOGE("chmod\n"); */
    rval = -3;
    do_unlink = 1;
    goto errout;
  }

  /* fill socket address structure with server's address */
  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, srv);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(srv);
  if (connect(fd, (struct sockaddr *)&sun, len) < 0) {
    /* LOGE("connect\n"); */
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

err_t sock_send(const sock_status_t *sock,
                opc_t opc, uint8_t len, const void *buf)
{
  err_t e = ec_success;
  int sl = len + 2, pos = 0, n;
  char *p;
  if (sock->fd < 0) {
    return err(ec_state);
  }
  p = malloc(2 + len);
  p[0] = opc;
  p[1] = len;
  if (len) {
    memcpy(p + 2, buf, len);
  }
  while (pos != sl) {
    if (-1 == (n = send(sock->fd, p + pos, sl - pos, 0))) {
      LOGE("send [%s]\n", strerror(errno));
      e = err(ec_sock);
      break;
    }
    pos += n;
  }

  free(p);
  return e;
}
