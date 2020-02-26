/*************************************************************************
    > File Name: demo.c
    > Author: Kevin
    > Created Time: 2020-01-23
    > Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include "mng.h"
#include "models.h"
/* Defines  *********************************************************** */

#define DEMO_INTERVAL 1

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
static uint8_t ls[10] = { 0 };

static struct {
  time_t expired;
  int pos;
} demo;

/* Static Functions Declaractions ************************************* */

static inline void __roll(uint8_t *l)
{
  *l += 10;
  *l %= 100;
}

static inline void roll(void)
{
  for (int i = 0; i < demo.pos; i++) {
    __roll(ls + i);
  }
  if (demo.pos < 10) {
    demo.pos++;
  }
}

const char *addrst[] = {
  "0xc021",
  "0xc022",
  "0xc023",
  "0xc024",
  "0xc025",
  "0xc026",
  "0xc027",
  "0xc028",
  "0xc029",
  "0xc02a",
};

const char *lnst[] = {
  "10",
  "20",
  "30",
  "40",
  "50",
  "60",
  "70",
  "80",
  "90",
  "100",
};

void demo_start(int en)
{
  if (en) {
    demo.expired = time(NULL) + DEMO_INTERVAL;
  } else {
    demo.expired = 0;
    demo.pos = 0;
  }
}

void demo_run(void)
{
  char p1[] = "lightness";
  char p2[10] = { 0 }, p3[10] = { 0 };
  char *p[3] = { 0 };

  time_t now = time(NULL);
  err_t e;

  if ( demo.expired == 0 || now < demo.expired) {
    return;
  }

  roll();

  for (int i = 0; i < demo.pos; i++) {
    memset(p2, 0, 10);
    memcpy(p2, lnst[ls[i] / 10], strlen(lnst[ls[i] / 10]));
    memset(p3, 0, 10);
    memcpy(p3, addrst[i], strlen(addrst[i]));
    p[0] = p1;
    p[1] = p2;
    p[2] = p3;
    e = clicb_lightness(3, p);
    elog(e);
    models_loop(get_mng());
  }

  demo.expired += DEMO_INTERVAL;
}
