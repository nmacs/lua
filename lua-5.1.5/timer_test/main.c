#include "ltimer.h"
#include <assert.h>

static int hit = 0;
static int hit_sum = 0;

static void timer_callback_test_1(void *data)
{
  hit = (int)data;
  hit_sum += hit;
}

/*
 * 0 - 63                (64)
 * 64 - 4095             (64*64)
 * 4096 - 262143         (64*64*64)
 * 262144 - 16777215     (64*64*64*64)
 * 16777216 - 1073741823 (64*64*64*64*64)
 */

static int test_1()
{
  struct timer_list timer1;
  struct timer_list timer2;
  struct timer_list timer3;
  clock_t t;
  int r;

  init_timers(100);
  set_timer(&timer1, 110, timer_callback_test_1, 0);
  add_timer(&timer1);
  del_timer(&timer1);

  t = get_next_timeout(100);
  assert(t == ((1 << (30)) - 1));

  set_timer(&timer1, 110, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(101);
  assert(t == 9);

  t = get_next_timeout(120);
  assert(t == 0);

  t = get_next_timeout(99);
  assert(t == 11);

  del_timer(&timer1);

  set_timer(&timer1, 100+64, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == 63);
  del_timer(&timer1);

  set_timer(&timer1, 200+64, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == 127);
  del_timer(&timer1);

  set_timer(&timer1, 100 + 64*64+100, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == 4095);
  del_timer(&timer1);

  set_timer(&timer1, 100 + 64*64+64*64+100, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == (4096+4095));
  del_timer(&timer1);

  set_timer(&timer1, 100 + 64*64*64+100, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == (64*64*64 - 1));
  del_timer(&timer1);

  set_timer(&timer1, 100 + 64*64*64, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == (64*64*64 - 1));
  del_timer(&timer1);

  set_timer(&timer1, 100 + 64*64*64*64, timer_callback_test_1, 0);
  add_timer(&timer1);
  t = get_next_timeout(100);
  assert(t == (64*64*64*64 - 1));
  del_timer(&timer1);

  set_timer(&timer1, 100 + 64*64*64*64*64, timer_callback_test_1, 0);
  r = add_timer(&timer1);
  assert(r != 0);

  process_timers(100);
  process_timers(200);

  set_timer(&timer1, 200+10, timer_callback_test_1, 0);
  add_timer(&timer1);

  t = get_next_timeout(200);
  assert(t == 10);

  process_timers(300);

  init_timers(0);
  hit = 0;
  set_timer(&timer1, 0, timer_callback_test_1, (void*)2);
  add_timer(&timer1);
  process_timers(0);
  assert(hit == 2);

  hit = 0;
  set_timer(&timer1, 0, timer_callback_test_1, (void*)2);
  add_timer(&timer1);
  process_timers(0);
  assert(hit == 2);

  init_timers(0);
  set_timer(&timer1, 64, timer_callback_test_1, (void*)1234);
  add_timer(&timer1);
  process_timers(64);
  assert(hit == 1234);

  init_timers(0);
  set_timer(&timer1, 0, timer_callback_test_1, (void*)12345);
  add_timer(&timer1);
  process_timers(1);
  assert(hit == 12345);

  init_timers(0);
  set_timer(&timer1, 0, timer_callback_test_1, (void*)12346);
  add_timer(&timer1);
  process_timers(63);
  assert(hit == 12346);

  init_timers(0);
  set_timer(&timer1, 0, timer_callback_test_1, (void*)12346);
  add_timer(&timer1);
  process_timers(1 << 30);
  assert(hit == 12346);

  init_timers(0);
  hit_sum = 0;
  set_timer(&timer1, 63, timer_callback_test_1, (void*)1234);
  add_timer(&timer1);
  set_timer(&timer2, 64, timer_callback_test_1, (void*)1234);
  add_timer(&timer2);
  process_timers(64);
  assert(hit_sum == 1234*2);

  init_timers(0);
  hit_sum = 0;
  set_timer(&timer1, 62, timer_callback_test_1, (void*)1234);
  add_timer(&timer1);
  set_timer(&timer2, 63, timer_callback_test_1, (void*)1234);
  add_timer(&timer2);
  set_timer(&timer3, 64, timer_callback_test_1, (void*)1234);
  add_timer(&timer3);
  process_timers(64);
  assert(hit_sum == 1234*3);

  init_timers(0);
  hit = hit_sum = 0;
  process_timers(60);
  process_timers(0);
  set_timer(&timer1, 68, timer_callback_test_1, (void*)1);
  add_timer(&timer1);
  t = get_next_timeout(60);
  assert(t == 4);
  t = get_next_timeout(0);
  assert(t == 64);

  init_timers(0);
  hit = hit_sum = 0;
  process_timers(63);
  set_timer(&timer1, 63, timer_callback_test_1, (void*)2);
  add_timer(&timer1);
  set_timer(&timer2, 64, timer_callback_test_1, (void*)2);
  add_timer(&timer2);
  process_timers(4096);
  assert(hit_sum == 4);

  init_timers(0);
  hit = hit_sum = 0;
  set_timer(&timer1, 4, 0, 0);
  add_timer(&timer1);
  process_timers(4);

  return 1;
}

int main(int argc, char *argv[])
{
  assert(test_1());
  return 0;
}
