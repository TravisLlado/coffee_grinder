// settings.h //////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>

// Grind duration stored in ESP32 flash (NVS). When the user has saved a custom
// value it always wins; otherwise the built-in DEFAULT_GRIND_MS is used.
namespace Settings {

  // Open the NVS namespace and load any stored custom duration. Call once from
  // setup(), before any other function here.
  void begin();

  // The duration to actually use: the custom value if one is stored,
  // DEFAULT_GRIND_MS otherwise.
  uint32_t grindMs();

  // True when a custom value is stored in flash.
  bool hasCustom();

  // Store a custom duration and use it from now on. Values outside
  // [MIN_GRIND_MS, MAX_GRIND_MS] are rejected and nothing is written.
  // Returns true when the value was accepted and saved.
  bool setCustom(uint32_t ms);

  // Erase the custom duration and fall back to DEFAULT_GRIND_MS.
  // Returns true if a custom value was present and has been erased.
  bool clearCustom();

}  // namespace Settings

// End of file /////////////////////////////////////////////////////////////////
