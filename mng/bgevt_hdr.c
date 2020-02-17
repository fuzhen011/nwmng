/*************************************************************************
    > File Name: bgevt_hdr.c
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "projconfig.h"
#include "bg_uart_cbs.h"
#include "uart.h"
#include "bgevt_hdr.h"
#include "socket_handler.h"
#include "gecko_bglib.h"
#include "logging.h"
#include "mng.h"
#include "nwk.h"
#include "dev_config.h"
#include "startup.h"

/* Defines  *********************************************************** */
BGLIB_DEFINE();

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static volatile int ncp_sync = false;

static bgevt_hdr hdrs[] = {
  dev_add_hdr,
  dev_config_hdr,
  bl_hdr,
  bgevt_dflt_hdr,
  NULL
};

/* Static Functions Declaractions ************************************* */
void conn_ncptarget(void)
{
  /**
   * Initialize BGLIB with our output function for sending messages.
   */
  const bguart_t *u = get_bguart_impl();
  proj_args_t *arg = (proj_args_t *)getprojargs();

  BGLIB_INITIALIZE_NONBLOCK(u->bglib_output, u->bglib_input, u->bglib_peek);
  if (arg->enc) {
    /* if (interface_args_ptr->encrypted) { */
    if (connect_domain_socket_server(arg->sock.srv, arg->sock.clt, 1)) {
      LOGE("Connection to encrypted domain socket unsuccessful. Exiting..\n");
      exit(EXIT_FAILURE);
    }
    LOGD("Turning on Encryption. "
         "All subsequent BGAPI commands and events will be encrypted..\n");
    turnEncryptionOn();
    /* } else { */
    /* if (connect_domain_socket_server(interface_args_ptr->pSerSockPath, CLIENT_UNENCRYPTED_PATH, 0)) { */
    /* printf("Connection to unencrypted domain socket unsuccessful. Exiting..\n"); */
    /* exit(EXIT_FAILURE); */
    /* } */
    /* } */
  } else {
    uartClose();
    if (0 != uartOpen((int8_t *)arg->serial.port, arg->serial.br, 1, 100)) {
      LOGE("Open %s failed. Exiting..\n", arg->serial.port);
      exit(EXIT_FAILURE);
    }
  }
}

void sync_host_and_ncp_target(void)
{
  struct gecko_cmd_packet *p;

  ncp_sync = false;
  LOGM("Syncing NCP Host and Target\n");
  for (int numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
    if (getprojargs()->enc) {
      poll_update(50);
    }
    p = gecko_peek_event();
    if (p) {
      switch (BGLIB_MSG_ID(p->header)) {
        case gecko_evt_system_boot_id:
          LOGM("System Booted - Host and NCP Target Synchronized\n");
          ncp_sync = true;
          break;
        default:
          gecko_cmd_system_reset(0);
          LOGM("Sent reset signal to NCP target\n");
          /*
           * Delay before trying again.
           */
          if (numsec <= MAXSLEEP / 2) {
            sleep(numsec);
          }
          break;
      }
    } else {
      gecko_cmd_system_reset(0);
      LOGM("Sent reset signal to NCP target\n");
      /*
       * Delay before trying again.
       */
      if (numsec <= MAXSLEEP / 2) {
        sleep(numsec);
      }
    }

    if (ncp_sync) {
      return;
    }
  }

  if (!ncp_sync) {
    LOGE("Failed to Synchronize NCP Target\n");
    exit(EXIT_FAILURE);
  }
}

void bgevt_dispenser(void)
{
  bool handled;
  struct gecko_cmd_packet *evt = NULL;
  if (!ncp_sync) {
    sync_host_and_ncp_target();
    return;
  }

  do {
    handled = false;
    if (getprojargs()->enc) {
      poll_update(50);
    }
    evt = gecko_peek_event();
    if (evt) {
      bgevt_hdr *h = hdrs;
      while (*h && !handled) {
        handled = (*h)(evt);
        h++;
      }
      if (!handled) {
        LOGW("NCP Target Event [0x%08x] Not Handled\n", BGLIB_MSG_ID(evt->header));
      }
    }
  } while (evt);
}
