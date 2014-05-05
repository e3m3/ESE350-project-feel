#ifndef TI_ANALOG_MUX_H
#define TI_ANALOG_MUX_H

#include "mbed.h"

#define CHANNELS      16


class TIAnalogMux {
  public:
    TIAnalogMux(PinName in, PinName s0, PinName s1, PinName s2, PinName s3,
        PinName en);

    void reset(void);
    void enable(void);
    void disable(void);
    void step(void);

    float get(uint8_t) const;
    float operator[](const size_t) const;

  private:
    void encode_select_and_set(uint8_t);

    AnalogIn adc_input;
    DigitalOut mux_select_0;
    DigitalOut mux_select_1;
    DigitalOut mux_select_2;
    DigitalOut mux_select_3;
    DigitalOut mux_enable;
    float mux_vals[CHANNELS];
    Timer mux_timer;
    uint8_t current_channel;
};

#endif /*TI_ANALOG_MUX_H*/
