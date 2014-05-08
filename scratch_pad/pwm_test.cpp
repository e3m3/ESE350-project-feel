#include "mbed.h"

#define THIRTYHZ 33.0 // ms
#define THESH 0

DigitalOut pwm_out1(p15);
DigitalOut pwm_out2(p16);
DigitalOut pwm_out3(p17);
DigitalOut pwm_out_arr[] = {pwm_out1, pwm_out2, pwm_out3};

Ticker ticker_pwm1, ticker_pwm2, ticker_pwm3;
Ticker ticker_pwm[] = {ticker_pwm1, ticker_pwm2, ticker_pwm3};
Ticker ticker_30Hz;
volatile float pwm_duty_cycles[3];

void ticker_pwm_handler1(void);
void ticker_pwm_handler2(void);
void ticker_pwm_handler3(void);
void ticker_30Hz_handler(void);

void (*ticker_handler_arr[])(void) = {&ticker_pwm_handler1, &ticker_pwm_handler2, &ticker_pwm_handler3};


int main(void) {
  for (int i = 0; i < 3; i++) {
    pwm_duty_cycles[i] = 0.0;
  }
  ticker_30Hz.attach_us(&ticker_30Hz_handler, THIRTYHZ*1000);

  while (true) {
  }

  return 0;
}


void ticker_pwm_handler1(void) {
  ticker_pwm1.detach();
  pwm_out1 = 0;
}


void ticker_pwm_handler2(void) {
  ticker_pwm2.detach();
  pwm_out2 = 0;
}


void ticker_pwm_handler3(void) {
  ticker_pwm3.detach();
  pwm_out3 = 0;
}


void ticker_30Hz_handler(void) {
  for (int i = 0; i < 3; i++) {
    if (pwm_duty_cycles[i] > THESH) {
      ticker_pwm[i].attach_us(ticker_handler_arr[i],
          (unsigned int) (pwm_duty_cycles[i]*((float) THIRTYHZ)));
      pwm_out_arr[i] = 1;
    } else pwm_out_arr[i] = 0;
  }
}
