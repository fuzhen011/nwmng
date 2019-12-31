/*************************************************************************
    > File Name: generic_parser.h
    > Author: Kevin
    > Created Time: 2019-12-18
    > Description:
 ************************************************************************/

#ifndef GENERIC_PARSER_H
#define GENERIC_PARSER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "err.h"
#include "cfg_keys.h"

enum {
  cft_json,
};

/* write type */
enum {
  wrt_clrctl,
  /* Adding a node to backlog */
  wrt_add_node,
  wrt_errbits,
  wrt_node_addr,
  wrt_done,
  /* For prov cfg file */
  wrt_prov_addr,
  wrt_prov_ivi,
  wrt_prov_synctime,
  wrt_prov_netkey_id,
  wrt_prov_netkey_done,
  wrt_prov_appkey_id,
  wrt_prov_appkey_done,
};

/* read type */
enum {
  rdt_node,
};

/* cfg_fd */
enum {
  PROV_CFG_FILE,
  NW_NODES_CFG_FILE,
  TEMPLATE_FILE
};

#define FL_CREATE                       (1UL << 0)
#define FL_TRUNC                        (1UL << 1)
/* Clear all control fields */
#define FL_CLR_CTLFS                    (1UL << 2)

void gp_init(int cfg_filetype, void *init_data);

/*
 * Below typedefs are used for parsering any kind of config file with a
 * key-value structure.
 *
 * All single item in the file can be address by {type + key}, the {key} can be a
 * string or integer or anything else, the type of it depends on the {type}.
 * E.g. if the {type} is to modify the value of a key, the {key} could be the
 * reference ID of the key. For some specific {type}, {key} can be NULL. E.g. the
 * {type} is to read the whole config file, no {key} is needed.
 *
 *
 */
typedef err_t (*gp_init_func_t)(void *data);
typedef err_t (*gp_deinit_func_t)(void);
typedef err_t (*gp_open_func_t)(int cfg_fd,
                                const char *filepath,
                                unsigned int flags);
typedef err_t (*gp_read_func_t)(int cfg_fd,
                                int rdtype,
                                const void *key,
                                void *data);
typedef err_t (*gp_write_func_t)(int cfg_fd,
                                 int wrtype,
                                 const void *key,
                                 void *data);
typedef void (*gp_close_func_t)(int cfg_fd);

typedef err_t (*gp_flush_func_t)(int cfg_fd);

typedef err_t (*gp_free_cfg_func_t)(void **);

/******************************************************************
 * IPC Callbacks
 * ***************************************************************/
err_t prov_clrctl(int len, const char *arg);
err_t prov_get(int len, const char *arg);
err_t provset_addr(int len, const char *arg);
err_t provset_ivi(int len, const char *arg);
err_t provset_synctime(int len, const char *arg);
err_t provset_netkeyid(int len, const char *arg);
err_t provset_netkeydone(int len, const char *arg);
err_t provset_appkeyid(int len, const char *arg);
err_t provset_appkeydone(int len, const char *arg);
err_t _upldev_check(int len, const char *arg);
#ifdef __cplusplus
}
#endif
#endif //GENERIC_PARSER_H
