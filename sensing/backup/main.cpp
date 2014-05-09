#include "mbed.h"
#include "MRF24J40.h"
#include "ti_analog_mux.h"
#include <string>

#define NUM_PADS  7

#ifndef toggle
#define toggle(val) ((val) = (val) ^ 1)
#endif

// RF tranceiver to link with handheld.
MRF24J40 mrf(p11, p12, p13, p14, p21);
TIAnalogMux mux(p15, p24, p23, p26, p25, p27);

// LEDs you can treat these as variables (led2 = 1 will turn led2 on!)
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Timer
Timer timer;
Ticker ticker;

// Serial port for showing RX data.
Serial pc(USBTX, USBRX);

// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

void step(void) { mux.step(); }
int rf_receive(char *data, uint8_t maxLength);
void rf_send(char *data, uint8_t len);


//***************** You can start coding here *****************//
int main (void) {
  uint8_t channel = 12;
  mrf.SetChannel(channel);
  //pc.baud(19200);
  //pc.baud(19200);

  //AnalogIn* pads = (AnalogIn *) malloc(sizeof(AnalogIn)*NUM_PADS);
  //pads[0] = AnalogIn(p15);
  //pads[1] = AnalogIn(p16);
  //pads[2] = AnalogIn(p17);
  //pads[3] = AnalogIn(p18);

  //Start the timer
  timer.start();
  mux.reset();
  ticker.attach_us(&step, 160);

  while (true) {
    if (timer.read_ms() >= 50) {
      timer.reset();
      toggle(led1);
      sprintf(txBuffer, "0,%d", NUM_PADS);
      for (int i = 0; i < NUM_PADS; i++) {
        // Insert the force value at the right offset from the start
        // sprintf(txBuffer + 1 + i*7, ",%1.4f", pads[i].read());
        sprintf(txBuffer + 3 + i*7, ",%1.4f", mux[i]);
      }
      rf_send(txBuffer, strlen(txBuffer) + 1);
      pc.printf("Sent: %s\n", txBuffer);
    }
  }
}


//***************** Do not change these methods (please) *****************//

/**
* Receive data from the MRF24J40.
*
* @param data A pointer to a char array to hold the data
* @param maxLength The max amount of data to read.
*/
int rf_receive(char *data, uint8_t maxLength) {
  uint8_t len = mrf.Receive((uint8_t *)data, maxLength);
  uint8_t header[8] = {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};

  if (len > 10) {
    //Remove the header and footer of the message
    for (uint8_t i = 0; i < len - 2; i++) {
      if (i < 8) {
        //Make sure our header is valid first
        if (data[i] != header[i])
          return 0;
      } else {
        data[i - 8] = data[i];
      }
    }

    //pc.printf("Received: %s length:%d\r\n", data, ((int)len)-10);
  }
  return ((int)len) - 10;
}

/**
* Send data to another MRF24J40.
*
* @param data The string to send
* @param maxLength The length of the data to send.
*                  If you are sending a null-terminated string you can pass strlen(data)+1
*/
void rf_send(char *data, uint8_t len) {
  //We need to prepend the message with a valid ZigBee header
  uint8_t header[8] = {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
  uint8_t *send_buf = (uint8_t *) malloc(sizeof(uint8_t)*(len + 8));

  for(uint8_t i = 0; i < len + 8; i++) {
    //prepend the 8-byte header
    send_buf[i] = (i < 8) ? header[i] : data[i - 8];
  }
  //pc.printf("Sent: %s\r\n", send_buf+8);

  mrf.Send(send_buf, len + 8);
  free(send_buf);
}
