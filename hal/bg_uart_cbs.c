/*************************************************************************
    > File Name: bg_uart_cbs.c
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdlib.h>
#include <errno.h>

#include "bg_uart_cbs.h"
#include "logging.h"
#include "utils.h"
#include "uart.h"
#include "socket_handler.h"

/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static bguart_t bguart = { -1, NULL, NULL, NULL };

/* Static Functions Declaractions ************************************* */
static void on_message_send(uint32_t msg_len, uint8_t* msg_data)
{
  /** Variable for storing function return values. */
  int ret;

  ret = uartTx(msg_len, msg_data);
  if (ret < 0) {
    LOGE("on_message_send() - failed to write ret: %d, errno: %d\n", ret, errno);
    exit(EXIT_FAILURE);
  }
}

void bguart_init(bool enc,
                 char *ser_sockpath,
                 char *client_sockpath)
{
  if (bguart.enc == -1 || (enc ^ (bool)bguart.enc)) {
    bguart.enc = enc;
    if (enc) {
      ASSERT(ser_sockpath && client_sockpath);
      bguart.bglib_input = onMessageReceive;
      bguart.bglib_output = onMessageSend;
      bguart.bglib_peek = messagePeek;
      bguart.ser_sockpath = ser_sockpath;
      bguart.client_sockpath = client_sockpath;
    } else {
      ASSERT(ser_sockpath);
      bguart.bglib_input = uartRx;
      bguart.bglib_output = on_message_send;
      bguart.bglib_peek = uartRxPeek;
      bguart.ser_sockpath = ser_sockpath;
      bguart.client_sockpath = client_sockpath;
    }
  }
}

const bguart_t *get_bguart_impl(void)
{
  return bguart.enc == -1 ? NULL : &bguart;
}
