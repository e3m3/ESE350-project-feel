#include <stdbool.h>
#include <avr/interrupt.h>
#include "m_general.h"
#include "m_usb.h"

#define NUM_PADS    4


/*** Global Variables ***/
volatile int pad_force[NUM_PADS];
volatile bool adc_completed;
volatile unsigned short next_adc_chan;
int i;
/***/

/*** Function Prototypes ***/
void set_adc_mux(unsigned short chan);
/***/

ISR(ADC_vect) {
  set(ADCSRA, ADIF);
  pad_force[next_adc_chan] = ADC;
  next_adc_chan = (next_adc_chan + 1)%NUM_PADS;
  set_adc_mux(next_adc_chan);
  set(ADCSRA, ADSC);
  adc_completed = true;
}

int main(void) {
  m_clockdivide(3);
  sei();
  m_usb_init();

  /*** Initilize globals ***/
  for (i = 0; i < NUM_PADS; i++)
    pad_force[i] = 0;
  adc_completed = false;
  next_adc_chan = 0;
  /***/

  /*** Initilize digital IO ports ***/
  DDRE |= 0x44; // Enable green and red LEDs (pin E2, E6) 

  PORTE = 0x40; // Set red LED E6 while waiting for USB comm
  while(!m_usb_isconnected());
  PORTE = 0x00; // Clear red LED E6
  /***/

  /***  Set Analog Inputs ***/
  // Set Vcc as the analog reference voltage
  clear(ADMUX, REFS1);
  set(ADMUX, REFS0);

  // Set ADC sample frequency to 62.KHz = 2MHz / 32
  set(ADCSRA, ADPS2);
  clear(ADCSRA, ADPS1);
  set(ADCSRA, ADPS0);

  // Disable digital pin circuitry for analog input pins
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

  while (true) {
    if (adc_completed) {
      adc_completed = false;
      //PORTE ^= 0x04;
      m_usb_tx_string("Pad force:\t");
      for (i = 0; i < NUM_PADS; i++) {
        m_usb_tx_char('[');
        m_usb_tx_int(i);
        m_usb_tx_string("] ");
        m_usb_tx_int(pad_force[i]);
        m_usb_tx_char('\t');
      }
      m_usb_tx_char('\n');
    }
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
    default: break; // Error
  }
}
