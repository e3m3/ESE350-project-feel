#include "mbed.h"
#include "ti_analog_mux.h"
#include <string>



// LEDs you can treat these as variables (led2 = 1 will turn led2 on!)
DigitalOut led1(LED1);

// Timer
Timer timer;
Ticker ticker;

// Serial port for showing RX data.
Serial pc(USBTX, USBRX);

TIAnalogMux mux(p15, p21, p22, p23, p24, p25);

void step(void) { mux.step(); }


//***************** You can start coding here *****************//
int main (void) {
  timer.start();
  mux.reset();
  ticker.attach_us(&step, 160);

  while (true) {
    //mux.step();

    if (timer.read_ms() >= 10) {
      timer.reset();
      led1 = led1^1;
      for (uint8_t i = 0; i < 6; i++)
        pc.printf("[%d] %1.4f\t", (int) i, mux[i]);
      pc.printf("\n");
    }
  }
}
