/*************************************************************************
    > File Name: bg_uart_cbs.h
    > Author: Kevin
    > Created Time: 2019-12-20
    > Description:
 ************************************************************************/

#ifndef BG_UART_CBS_H
#define BG_UART_CBS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  /* int enc; */
  void (*bglib_output)(uint32_t len1, uint8_t * data1);
  int32_t (*bglib_input)(uint32_t len1, uint8_t* data1);
  int32_t (*bglib_peek)(void);
  /* char *ser_sockpath; */
  /* char *client_sockpath; */
}bguart_t;

void bguart_init(void);
const bguart_t *get_bguart_impl(void);

#ifdef __cplusplus
}
#endif
#endif //BG_UART_CBS_H
