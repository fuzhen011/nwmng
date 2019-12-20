/*************************************************************************
    > File Name: mng/mng.c
    > Author: Kevin
    > Created Time: 2019-12-13
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "hal/bg_uart_cbs.h"
#include "host_gecko.h"

#include "projconfig.h"
/* #include <sys/prctl.h> */
#include "logging.h"
#include "utils.h"
#include "bg_uart_cbs.h"
#include "gecko_bglib.h"
#include "socket_handler.h"
#include "uart.h"
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */

/* Static Functions Declaractions ************************************* */
void __mng_exit(void)
{
  /* gecko_cmd_system_reset(0); */
  LOGD("MNG Exit\n");
}

int mng_proc(void)
{
  /* prctl(PR_SET_PDEATHSIG, SIGKILL); */
  atexit(__mng_exit);
  for (;;) {
    sleep(1);
  }
}

void conn_ncptarget(void)
{
  /**
   * Initialize BGLIB with our output function for sending messages.
   */
  const bguart_t *u = get_bguart_impl();
  BGLIB_INITIALIZE_NONBLOCK(u->bglib_output, u->bglib_input, u->bglib_peek);
  if (u->enc) {
    /* if (interface_args_ptr->encrypted) { */
    if (connect_domain_socket_server(u->ser_sockpath, u->client_sockpath, 1)) {
      LOGE("Connection to encrypted domain socket unsuccessful. Exiting..\n");
      exit(EXIT_FAILURE);
    }
    LOGM("Turning on Encryption. "
         "All subsequent BGAPI commands and events will be encrypted..\n");
    turnEncryptionOn();
    /* } else { */
    /* if (connect_domain_socket_server(interface_args_ptr->pSerSockPath, CLIENT_UNENCRYPTED_PATH, 0)) { */
    /* printf("Connection to unencrypted domain socket unsuccessful. Exiting..\n"); */
    /* exit(EXIT_FAILURE); */
    /* } */
    /* } */
  } else {
    if (0 != uartOpen((int8_t *)u->ser_sockpath, 115200, 1, 100)) {
      LOGE("Open %s failed. Exiting..\n", u->ser_sockpath);
      exit(EXIT_FAILURE);
    }
  }
}

#if 0
void bglib_interface_init_sync(const interfaceArgs_t *p)
{
  hardASSERT(p);

  interface_args_ptr = (interfaceArgs_t *)p;
  hostTargetSyncUp = false;
  openConnectionToNcpTarget();
  /* memset(filters, 0, sizeof(filter_t) * MAX_FILTERS); */
  initialized = true;

  sync_host_and_ncp_target();
}
#endif

static volatile int ncp_sync = false;

static void sync_host_and_ncp_target(void)
{
  struct gecko_cmd_packet *p;

  ncp_sync = false;
  LOGM("Syncing up NCP Host and Target");
  do {
    printf("."); fflush(stdout);

    if (get_bguart_impl()->enc) {
      poll_update(50);
    }
    p = gecko_peek_event();
    if (p) {
      switch (BGLIB_MSG_ID(p->header)) {
        case gecko_evt_system_boot_id:
        {
          LOGM("System Booted. NCP Target and Host Sync-ed Up\n");
          ncp_sync = true;
        }
        break;
        default:
          sleep(1);
          break;
      }
    } else {
      gecko_cmd_system_reset(0);
      LOGD("Sent reset signal to NCP target\n");
      sleep(1);
    }

    if (ncp_sync) {
      break;
    }
  } while (1);
}

err_t mng_init(bool enc)
{
  bguart_init(false, PORT, NULL);
  conn_ncptarget();
  sync_host_and_ncp_target();
  atexit(__mng_exit);
  return ec_success;
}

void *mng_mainloop(void *p)
{
  while (1) {
    sleep(1);
  }
}
