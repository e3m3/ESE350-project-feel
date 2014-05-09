#include "mbed.h"
#include <string>
#include "../../spi_int.h"

#define SPI_PRD       20  // ms
#define PRINT_PRD     100 // ms
#define DEBOUNCE_TSH  500 // ms
#define SPI_WAIT_TIME 16  // us
#define SPI_FREQ      1000000  // MHz


DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

Timer print_timer, reset_m2_dbnc_timer;
Ticker spi_start_ticker;

Serial pc(USBTX, USBRX);
SPI spi(p11, p12, p13);
DigitalOut ss(p14);
InterruptIn reset_m2(p30);

int outgoing, incoming;
int fsr[NUM_FSR];
bool read_fsr;


void SPI_start_handler(void);
void m2_reset_handler(void);
int write_to_spi(int reg, bool no_read);
void read_fsr_from_m2(void);


int main (void) {
  ss = 1;
  spi.format(8, 0);
  spi.frequency(SPI_FREQ);
  spi_start_ticker.attach_us(&SPI_start_handler, SPI_PRD*1000);
  read_fsr = false;
  //reset_m2.rise(&m2_reset_handler);

  print_timer.start();
  reset_m2_dbnc_timer.start();

  while(true) {
    if (read_fsr) read_fsr_from_m2();

    if (print_timer.read_ms() >= PRINT_PRD) {
      print_timer.reset();
      led2 = led2^1;
      for (int i = 0; i < NUM_FSR; i++)
        pc.printf("%d\t", fsr[i]);
      pc.printf("\n");
    }
  }
}


void SPI_start_handler(void) {
  read_fsr = true;
}


//void m2_reset_handler(void) {
//  if (reset_m2_dbnc_timer.read_ms() > DEBOUNCE_TSH) {
//    pc.printf("Reseting\n");
//    reset_m2_dbnc_timer.reset();
//
//    ss = 0;
//    wait_us(SPI_WAIT_TIME);
//    spi.write(MBED_RESET_REQ);
//    wait_us(SPI_WAIT_TIME);
//    //ss = 1;
//  }
//}

int write_to_spi(int reg, bool no_read) {
  int read_val = DUMMY_BYTE;
  outgoing = reg;
  if (no_read)
    spi.write(outgoing);
  else
    read_val = spi.write(outgoing);
  wait_us(SPI_WAIT_TIME);
  return read_val;
}


void read_fsr_from_m2(void) {
  read_fsr = false;
  ss = 0;
  write_to_spi(MBED_FSR_REQ, true);
  //pc.printf("Sent: %d\t Received: %d\n", outgoing, incoming);
  for (int i = 0x00; i < NUM_FSR; i += 0x01) {
    incoming = write_to_spi(SEND_LO_BYTE, false);
    fsr[i] = incoming << 8;
    incoming = write_to_spi(SEND_HI_BYTE, false);
    fsr[i] |= incoming;
  }
  incoming = write_to_spi(MBED_FSR_COMP, false);
  incoming = write_to_spi(DUMMY_BYTE, false);
  ss = 1;
}
