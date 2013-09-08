#include "TimedButton.h"
#include <inttypes.h>
#include "Arduino.h"

TimedButton::TimedButton(uint8_t pin) {
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
    _pin = pin;
    _begin_time = 0;
}

/* Button was pressed for min_time ms, then released. */
uint8_t TimedButton::pressed(unsigned long min_time) {
    if (_begin_time) {
        unsigned long elapsed = millis() - _begin_time;
        if (elapsed < DEBOUNCE_TIME) return false;
        if (digitalRead(_pin) == LOW) return false;
        _begin_time = 0;
        if (elapsed >= min_time) return true;
        return false;
    }
    if (digitalRead(_pin) != LOW) return false;
    _begin_time = millis();
    return false;
}

/* Button was pressed for min_time ms, possibly still being pressed.
   Needs repeated calls to pressed() or down() to reset state afterwards.
*/
uint8_t TimedButton::down(unsigned long min_time) {
    int pressed = !digitalRead(_pin);
    if (! pressed) {
        _begin_time = 0;
        return false;
    }

    if (! min_time) return true;
    else if (! _begin_time) _begin_time = millis();

    unsigned long elapsed = millis() - _begin_time;
    return elapsed >= min_time;
}
