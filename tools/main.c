/*************************************************************************
    > File Name: main.c
    > Author: Kevin
    > Created Time: xxxx-xx-xx
    >Description:
 ************************************************************************/

/* Includes *********************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <dispatch/dispatch.h>
/* Defines  *********************************************************** */

/* Global Variables *************************************************** */

/* Static Variables *************************************************** */
int a = 3, b = 4;
int i = 0;
dispatch_queue_t queue;
dispatch_source_t timer1, timer2;

/* Static Functions Declaractions ************************************* */
void sigtrap(int sig)
{
  dispatch_source_cancel(timer1);
  dispatch_source_cancel(timer2);
  printf("CTRL-C received, exiting program\n");
  exit(EXIT_SUCCESS);
}

void vector1(dispatch_source_t timer, int *in)
{
  printf("a: %d\n", i);
  i++;
  *in = i;
  if (i >= 20) {
    dispatch_source_cancel(timer);
  }
}
void vector2(dispatch_source_t timer, int *in)
{
  printf("b: %d\n", i);
  *in = i;
  i++;
  if (i >= 20) {                        //at 20 count cancel the
    dispatch_source_cancel(timer);
  }
}

int main(int argc, const char* argv[])
{
  signal(SIGINT, &sigtrap);       //catch the cntl-c
  queue = dispatch_queue_create("timerQueue", 0);

  // Create dispatch timer source
  timer1 = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                                  0, 0, queue);
  timer2 =
    dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0,
                           0, queue);

  // Set block for dispatch source when catched events
  dispatch_source_set_event_handler(timer1, ^{ vector1(timer1, &a); });
  dispatch_source_set_event_handler(timer2, ^{ vector2(timer2, &b); });

  // Set block for dispatch source when cancel source
  dispatch_source_set_cancel_handler(timer1, ^{
    dispatch_release(timer1);
    dispatch_release(queue);
    printf("End, ab = [%d%d]\n", a, b);
    printf("1end\n");
    exit(0);
  });
  dispatch_source_set_cancel_handler(timer2, ^{
    dispatch_release(timer2);
    dispatch_release(queue);
    printf("End, ab = [%d%d]\n", a, b);
    printf("2end\n");
    exit(0);
  });

  dispatch_time_t start = dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC); // after 1 sec

  // Set timer
  dispatch_source_set_timer(timer1, start, NSEC_PER_SEC / 5, 0); // 0.2 sec
  dispatch_source_set_timer(timer2, start, NSEC_PER_SEC / 2, 0); // 0.5 sec
  printf("start\n");
  dispatch_resume(timer1);
  dispatch_resume(timer2);
  while (1) {
    ;;
  }
  return
    0;
}
