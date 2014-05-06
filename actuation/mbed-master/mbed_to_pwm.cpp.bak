#include "mbed.h"
 
SPI spi(p11, p12, p13);
DigitalOut latchpin(p10);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
 
#define LEDS 24
#define MAX_BRIGHTNESS 4095
 
void latch() {
  latchpin = 1;
  latchpin = 1;
  latchpin = 1;
  latchpin = 1;
  latchpin = 1;
  latchpin = 1;
  latchpin = 0;
}
 
int main() {
  int r, g, b;

  spi.format(12, 0);
  spi.frequency(500 * 1000);
  latchpin = 0;
   
  while (1) {
    for (int h = MAX_BRIGHTNESS; h >= 0; h--) {
      for (int i = LEDS-1; i >= 0; i -= 3) {
        spi.write(h);
        spi.write(h);
        spi.write(h);
      }
      latch();
    }
  }
}
