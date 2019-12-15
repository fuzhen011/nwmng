/*************************************************************************
    > File Name: sockcomn.h
    > Author: Kevin
    > Created Time: 2019-12-15
    > Description:
 ************************************************************************/

#ifndef SOCKCOMN_H
#define SOCKCOMN_H
#ifdef __cplusplus
extern "C"
{
#endif

int new_socksv(const char *name);
int serv_accept(int listenfd, uid_t *uidptr);
int new_sockcl(const char *name,
               const char *sv_name);
int timed_poll(struct pollfd *pl,
               int timeout,
               void *buf,
               int buflen);

#ifdef __cplusplus
}
#endif
#endif //SOCKCOMN_H
