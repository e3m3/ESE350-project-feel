#include "mbed.h"
#include "ti_analog_mux.h"

#define ANALOG_MUX_TIME_STEP 20 // us
#define INPUTS  7


TIAnalogMux::TIAnalogMux(PinName in, PinName s0, PinName s1, PinName s2,
    PinName s3, PinName en) : 

    adc_input(in),
    mux_select_0(s0),
    mux_select_1(s1),
    mux_select_2(s2),
    mux_select_3(s3),
    mux_enable(en) {

  reset();
}


void TIAnalogMux::reset(void) {
  mux_enable = 1;
  current_channel = 0;
  encode_select_and_set(current_channel);
  for (uint8_t i = 0; i < CHANNELS; i++)
    mux_vals[i] = 0.0;

  mux_enable = 0;
  //mux_timer.reset();
  //mux_timer.start();
}


void TIAnalogMux::enable(void) {
  current_channel = 0;
  encode_select_and_set(current_channel);
  mux_enable = 0; 
  //mux_timer.reset();
  //mux_timer.start();
}


void TIAnalogMux::disable(void) {
  mux_enable = 1;
  //mux_timer.stop();
}


void TIAnalogMux::step(void) {
  //if (mux_timer.read_us() > ANALOG_MUX_TIME_STEP) {
    //mux_timer.reset();
    mux_vals[current_channel] = adc_input.read(); 
    current_channel = (current_channel + 1)%INPUTS;
    encode_select_and_set(current_channel);
  //}
}


float TIAnalogMux::get(uint8_t channel) const {
  return (channel >= CHANNELS) ? -1.0 : mux_vals[channel];
}


float TIAnalogMux::operator[](const size_t channel) const {
  return get(channel);
}


void TIAnalogMux::encode_select_and_set(uint8_t channel) {
  switch (channel) {
    case 15:
      mux_select_0 = 0;
      mux_select_1 = 0;
      mux_select_2 = 0;
      mux_select_3 = 0;
      break;
    case 14:
      mux_select_0 = 1;
      mux_select_1 = 0;
      mux_select_2 = 0;
      mux_select_3 = 0;
      break;
    case 13:
      mux_select_0 = 0;
      mux_select_1 = 1;
      mux_select_2 = 0;
      mux_select_3 = 0;
      break;
    case 12:
      mux_select_0 = 1;
      mux_select_1 = 1;
      mux_select_2 = 0;
      mux_select_3 = 0;
      break;
    case 11:
      mux_select_0 = 0;
      mux_select_1 = 0;
      mux_select_2 = 1;
      mux_select_3 = 0;
      break;
    case 10:
      mux_select_0 = 1;
      mux_select_1 = 0;
      mux_select_2 = 1;
      mux_select_3 = 0;
      break;
    case 9:
      mux_select_0 = 0;
      mux_select_1 = 1;
      mux_select_2 = 1;
      mux_select_3 = 0;
      break;
    case 8:
      mux_select_0 = 1;
      mux_select_1 = 1;
      mux_select_2 = 1;
      mux_select_3 = 0;
      break;
    case 7:
      mux_select_0 = 0;
      mux_select_1 = 0;
      mux_select_2 = 0;
      mux_select_3 = 1;
      break;
    case 6:
      mux_select_0 = 1;
      mux_select_1 = 0;
      mux_select_2 = 0;
      mux_select_3 = 1;
      break;
    case 5:
      mux_select_0 = 0;
      mux_select_1 = 1;
      mux_select_2 = 0;
      mux_select_3 = 1;
      break;
    case 4:
      mux_select_0 = 1;
      mux_select_1 = 1;
      mux_select_2 = 0;
      mux_select_3 = 1;
      break;
    case 3:
      mux_select_0 = 0;
      mux_select_1 = 0;
      mux_select_2 = 1;
      mux_select_3 = 1;
      break;
    case 2:
      mux_select_0 = 1;
      mux_select_1 = 0;
      mux_select_2 = 1;
      mux_select_3 = 1;
      break;
    case 1:
      mux_select_0 = 0;
      mux_select_1 = 1;
      mux_select_2 = 1;
      mux_select_3 = 1;
      break;
    case 0:
      mux_select_0 = 1;
      mux_select_1 = 1;
      mux_select_2 = 1;
      mux_select_3 = 1;
      break;
    default: break;
  }
}
