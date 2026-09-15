// button.cpp //////////////////////////////////////////////////////////////////

#include "button.h"

// Constructor /////////////////////////////////////////////////////////////////

Button::Button(uint8_t pin, uint32_t debounceMs, ButtonCallback pressCallback)
    : button_pin(pin),
      debounce_ms(debounceMs),
      press_callback(pressCallback) {
}

// Public Methods //////////////////////////////////////////////////////////////

void Button::begin() {
    pinMode(button_pin, INPUT_PULLUP);

    stable_state     = readPin();
    last_reading     = stable_state;
    last_change_time = millis();
}

void Button::update() {
    const bool     reading = readPin();
    const uint32_t now     = millis();

    // Any change restarts the debounce window
    if (reading != last_reading) {
        last_reading     = reading;
        last_change_time = now;
        return;
    }

    // Reading has held steady long enough to be accepted
    if (reading != stable_state && (now - last_change_time) >= debounce_ms) {
        stable_state = reading;

        if (stable_state && press_callback) {
            press_callback();
        }
    }
}

bool Button::isPressed() const {
    return stable_state;
}

// Private Methods /////////////////////////////////////////////////////////////

bool Button::readPin() const {
    return digitalRead(button_pin) == LOW;  // active low
}

// End of file /////////////////////////////////////////////////////////////////
