// config.h ////////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>

// Hardware ////////////////////////////////////////////////////////////////////

// Onboard BOOT/bootselect button (GPIO 9 on ESP32-C3), active low.
constexpr uint8_t PIN_BUTTON {9U};

// Drives the optocoupler that switches the grinder, active high.
constexpr uint8_t PIN_GRINDER {4U};

// Onboard LED (GPIO 8 on Adafruit Qt Py ESP32-C3), active low.
constexpr uint8_t PIN_LED {8U};

// LED logical state - active low: ON = LOW, OFF = HIGH
enum LedState : uint8_t {
  LED_ON  = LOW,
  LED_OFF = HIGH,
};

// Grind timing ////////////////////////////////////////////////////////////////

// Built-in default grind duration, used whenever no custom value is stored.
constexpr uint32_t DEFAULT_GRIND_MS {5000U};

// Bounds accepted when the user sets a custom duration over WebSerial.
constexpr uint32_t MIN_GRIND_MS {100U};
constexpr uint32_t MAX_GRIND_MS {120000U};

// Button //////////////////////////////////////////////////////////////////////

// Reading must be stable this long before a press or release is accepted.
constexpr uint32_t BUTTON_DEBOUNCE_MS {30U};

// Flash storage ///////////////////////////////////////////////////////////////

// NVS namespace and key holding the custom grind duration, in milliseconds.
constexpr const char* NVS_NAMESPACE {"grinder"};
constexpr const char* NVS_KEY_GRIND_MS {"grind_ms"};

// Network /////////////////////////////////////////////////////////////////////

// WebSerial lives at http://<MDNS_HOSTNAME>.local/
constexpr const char* MDNS_HOSTNAME {"coffeegrinder"};
constexpr uint16_t    SERVER_PORT {80};

// How often (ms) the main loop services the network state machine.
constexpr uint32_t NET_CHECK_INTERVAL_MS {500};

// Minimum gap (ms) between WiFi re-join attempts while disconnected.
constexpr uint32_t RECONNECT_INTERVAL_MS {10000};

// Program /////////////////////////////////////////////////////////////////////

constexpr uint32_t SERIAL_BAUD_RATE {115200};
constexpr uint32_t TICK_PERIOD_MS {5U};

// End of file /////////////////////////////////////////////////////////////////
