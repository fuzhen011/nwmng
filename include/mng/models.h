/*************************************************************************
    > File Name: models.h
    > Author: Kevin
    > Created Time: 2020-02-26
    > Description: 
 ************************************************************************/

#ifndef MODELS_H
#define MODELS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
#include <stdint.h>
#include "mng.h"

bool models_loop(mng_t *mng);
uint16_t send_onoff(uint16_t addr, uint8_t onoff);
uint16_t send_lightness(uint16_t addr, uint8_t lightness);
uint16_t send_ctl(uint16_t addr, uint8_t ctl);

void demo_run(void);
void demo_start(int en);

int model_evt_hdr(const struct gecko_cmd_packet *evt);
#ifdef __cplusplus
}
#endif
#endif //MODELS_H
