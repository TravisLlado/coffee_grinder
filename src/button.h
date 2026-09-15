// button.h ////////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>

// Function pointer type for the press action
typedef void (*ButtonCallback)();

// Active-low button with debouncing. The callback fires on the leading edge of
// an accepted press, so the grinder starts the moment the button goes down.
class Button {
public:
    // debounceMs: the reading must hold steady this long before it is accepted
    // pressCallback: called once per accepted press (may be nullptr)
    Button(uint8_t pin, uint32_t debounceMs, ButtonCallback pressCallback);

    void begin();
    void update();

    bool isPressed() const;

private:
    const uint8_t        button_pin;
    const uint32_t       debounce_ms;
    const ButtonCallback press_callback;

    bool     stable_state{false};      // debounced state: true = pressed
    bool     last_reading{false};      // most recent raw reading
    uint32_t last_change_time{0};      // when last_reading last changed

    bool readPin() const;
};

// End of file /////////////////////////////////////////////////////////////////
