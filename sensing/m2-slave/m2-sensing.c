#include <stdbool.h>
#include <string.h>
#include <avr/interrupt.h>
#include "m_general.h"
#include "m_usb.h"
#include "../../spi_int.h"

#define SS    0
#define SLCK  1
#define MOSI  2
#define MISO  3

#define low_byte(r)   ((r) & (0x00FF))
#define high_byte(r)  (((r) & (0xFF00)) >> (8))

volatile bool spi_event;
volatile int state, spi_write_state;
volatile int outgoing, incoming;
volatile int current_fsr;

volatile int fsr[NUM_FSR];
volatile bool adc_completed;
volatile unsigned short next_adc_chan;
int i;


void set_adc_mux(unsigned short chan);


ISR(ADC_vect) {
  set(ADCSRA, ADIF);
  fsr[next_adc_chan] = ADC;
  next_adc_chan = (next_adc_chan + 1)%NUM_FSR;
  set_adc_mux(next_adc_chan);
  set(ADCSRA, ADSC);
  adc_completed = true;
}


ISR(SPI_STC_vect) {
  spi_event = true;
  incoming = SPDR;

  if (state == M2_IDLE && incoming == MBED_FSR_REQ) {
    current_fsr = M2_FSR_0;
    //outgoing = fsr[current_fsr] >> 2;
    outgoing = high_byte(fsr[current_fsr]);
    SPDR = outgoing;
    //current_fsr += 0x01;
    state = M2_SEND_FSR;
  } else if (state == M2_SEND_FSR && incoming == SEND_LO_BYTE) {
    //outgoing = fsr[current_fsr] >> 2;
    outgoing = low_byte(fsr[current_fsr]);
    SPDR = outgoing;
    current_fsr += 0x01;
  } else if (state == M2_SEND_FSR && incoming == SEND_HI_BYTE) {
    outgoing = high_byte(fsr[current_fsr]);
    SPDR = outgoing;
  } else if (state == M2_SEND_FSR && incoming == MBED_FSR_COMP) {
    state = M2_IDLE;
  }
}


int main(void) {
  m_clockdivide(0);
  m_usb_init();
  sei();

  spi_event = false;
  state = M2_IDLE;
  current_fsr = M2_FSR_0;
  for (i = 0; i < NUM_FSR; i++)
    fsr[i] = 0;
  adc_completed = false;
  next_adc_chan = 0;

  set(DDRE, 2);
  set(DDRE, 6);

  set(PORTE, 2);
  clear(PORTE, 6);
  while (!m_usb_isconnected());
  set(PORTE, 6);

  /***  Set Analog Inputs ***/
  // Set Vcc as the analog reference voltage
  clear(ADMUX, REFS1);
  set(ADMUX, REFS0);

  // Set ADC sample frequency to 62.KHz = 2MHz / 32
  set(ADCSRA, ADPS2);
  clear(ADCSRA, ADPS1);
  set(ADCSRA, ADPS0);

  // Disable digital pin circuitry for analog input pins
  set(DIDR0, ADC7D);
  set(DIDR0, ADC6D);
  set(DIDR0, ADC5D);
  set(DIDR0, ADC4D);
  set(DIDR0, ADC1D);
  set(DIDR0, ADC0D);

  // Enable analog conversion complete interrupt
  set(ADCSRA, ADIE);

  //// Set free-running conversion (start new conversion after finished)
  //set(ADCSRA, ADATE);

  //// Select free-running as ADC trigger source
  //clear(ADCSRB, ADTS3);
  //clear(ADCSRB, ADTS2);
  //clear(ADCSRB, ADTS1);
  //clear(ADCSRB, ADTS0);

  // Set conversion for ADC0 (pin F0)
  set_adc_mux(0);

  // Enable ADC unit
  set(ADCSRA, ADEN);

  // Kickstart the first conversion
  set(ADCSRA, ADSC);
  /***/

  /***  Set SPI ***/
  clear(PRR0, PRSPI); // Disable power reduction to allow SPI
  set(DDRB, MISO);    // Enable MISO output for slave mode
  set(SPCR, SPE);     // Enable SPI
  //set(SPCR, SPR1);    // 
  set(SPCR, SPR0);    // Sets divider to /16 --> 16MHz downto 1MHz
  set(SPCR, SPIE);    // Enable SPI interrupts
  /***/
  
  //m_usb_tx_string("Waiting for master.\n");
  
  while (true) {
    if (adc_completed) {
      adc_completed = false;
      //PORTE ^= 0x04;
      m_usb_tx_string("Pad force:\t");
      for (i = 0; i < NUM_FSR; i++) {
        m_usb_tx_char('[');
        m_usb_tx_int(i);
        m_usb_tx_string("] ");
        m_usb_tx_int(fsr[i]);
        m_usb_tx_char('\t');
      }
      m_usb_tx_char('\n');
    }

    if (spi_event) {
      spi_event = false;
      //m_usb_tx_string("Received: ");
      //m_usb_tx_int(incoming);
      //m_usb_tx_string("\tSent: ");
      //m_usb_tx_int(outgoing);
      //m_usb_tx_char('\n');
    }

    //usb_serial_write((uint8_t *)tx_buf, strlen(tx_buf) + 1);
  }

  return 0;
}


// Select single ended channel for ADC conversion
void set_adc_mux(unsigned short chan) {
  clear(ADCSRB, MUX5);
  switch (chan) {
    case 0: // F0
      clear(ADMUX, MUX2);
      clear(ADMUX, MUX1);
      clear(ADMUX, MUX0);
      break;
    case 1: // F1
      clear(ADMUX, MUX2);
      clear(ADMUX, MUX1);
      set(ADMUX, MUX0);
      break;
    case 2: // F4
      set(ADMUX, MUX2);
      clear(ADMUX, MUX1);
      clear(ADMUX, MUX0);
      break;
    case 3: // F5
      set(ADMUX, MUX2);
      clear(ADMUX, MUX1);
      set(ADMUX, MUX0);
      break;
    case 4: // F6
      set(ADMUX, MUX2);
      set(ADMUX, MUX1);
      clear(ADMUX, MUX0);
      break;
    case 5: // F7
      set(ADMUX, MUX2);
      set(ADMUX, MUX1);
      set(ADMUX, MUX0);
      break;
    default: 
      clear(PORTB, 2);
      break; // Error
  }
}
