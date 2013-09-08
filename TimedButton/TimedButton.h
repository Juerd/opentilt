/*
  TimedButton - Simple Arduino library for simple buttons.
  Note: will enable the Arduino's built-in pull-up resistor!

  Juerd Waalboer
*/

#ifndef TimedButton_h
#define TimedButton_h

#define DEBOUNCE_TIME 100

#include <inttypes.h>

class TimedButton {
    public:
        TimedButton(uint8_t pin);
        uint8_t pressed(unsigned long min_time = 0);
        uint8_t down(unsigned long min_time = 0);
    private:
        uint8_t _pin;
        unsigned long _begin_time;
};

#endif
