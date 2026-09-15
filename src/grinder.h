// grinder.h ///////////////////////////////////////////////////////////////////

#pragma once

#include <Arduino.h>

// Drives the optocoupler on PIN_GRINDER for a fixed duration. Non-blocking:
// start() returns immediately and update() releases the pin when time is up.
namespace Grinder {

  // Put the pin in a known-off state. Call once from setup().
  void begin();

  // Run the grinder for durationMs. Ignored while a run is already in progress,
  // so a second button press cannot extend or restart the grind.
  // Returns true if the run was started.
  bool start(uint32_t durationMs);

  // Call every loop; switches the grinder off once the duration has elapsed.
  void update();

  // Switch the grinder off immediately, abandoning any run in progress.
  void stop();

  bool isRunning();

  // Milliseconds left in the current run, 0 when idle.
  uint32_t remainingMs();

}  // namespace Grinder

// End of file /////////////////////////////////////////////////////////////////
