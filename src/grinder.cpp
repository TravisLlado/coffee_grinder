// grinder.cpp /////////////////////////////////////////////////////////////////

#include "grinder.h"

#include "config.h"

namespace {

bool     running{false};
uint32_t start_time{0};
uint32_t duration_ms{0};

void setOutput(bool on) {
  digitalWrite(PIN_GRINDER, on ? HIGH : LOW);  // optocoupler is active high
  digitalWrite(PIN_LED, on ? LED_ON : LED_OFF);
}

}  // namespace

namespace Grinder {

void begin() {
  // Drive the pin low before enabling the output, so switching the pin to
  // OUTPUT cannot glitch the grinder on at boot.
  digitalWrite(PIN_GRINDER, LOW);
  pinMode(PIN_GRINDER, OUTPUT);
  digitalWrite(PIN_GRINDER, LOW);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF);

  running     = false;
  start_time  = 0;
  duration_ms = 0;
}

bool start(uint32_t durationMs) {
  if (running || durationMs == 0) {
    return false;
  }

  start_time  = millis();
  duration_ms = durationMs;
  running     = true;
  setOutput(true);
  return true;
}

void update() {
  if (!running) {
    return;
  }

  if ((millis() - start_time) >= duration_ms) {
    stop();
  }
}

void stop() {
  setOutput(false);
  running     = false;
  duration_ms = 0;
}

bool isRunning() {
  return running;
}

uint32_t remainingMs() {
  if (!running) {
    return 0;
  }

  const uint32_t elapsed = millis() - start_time;
  return (elapsed >= duration_ms) ? 0 : (duration_ms - elapsed);
}

}  // namespace Grinder

// End of file /////////////////////////////////////////////////////////////////
