// settings.cpp ////////////////////////////////////////////////////////////////

#include "settings.h"

#include <Preferences.h>

#include "config.h"

namespace {

Preferences prefs;

// Cached copy of flash state so the hot path never touches NVS.
//
// These are written from the WebSerial handler (AsyncTCP task) and read from
// loop(), so effective_ms holds the duration to actually use rather than being
// derived from two variables at read time. A single aligned word read can never
// see a half-updated pair, so grindMs() needs no lock.
volatile uint32_t effective_ms{DEFAULT_GRIND_MS};
volatile bool     custom_present{false};

}  // namespace

namespace Settings {

void begin() {
  prefs.begin(NVS_NAMESPACE, false);

  if (!prefs.isKey(NVS_KEY_GRIND_MS)) {
    return;
  }

  const uint32_t stored{prefs.getUInt(NVS_KEY_GRIND_MS, DEFAULT_GRIND_MS)};

  // A stored value outside the accepted range means corrupt or stale flash;
  // drop it rather than driving the grinder for an unexpected length of time.
  if (stored < MIN_GRIND_MS || stored > MAX_GRIND_MS) {
    prefs.remove(NVS_KEY_GRIND_MS);
    return;
  }

  effective_ms   = stored;
  custom_present = true;
}

uint32_t grindMs() {
  return effective_ms;
}

bool hasCustom() {
  return custom_present;
}

bool setCustom(uint32_t ms) {
  if (ms < MIN_GRIND_MS || ms > MAX_GRIND_MS) {
    return false;
  }

  // Skip the flash write when nothing would change, to spare NVS wear
  if (custom_present && effective_ms == ms) {
    return true;
  }

  if (prefs.putUInt(NVS_KEY_GRIND_MS, ms) == 0) {
    return false;
  }

  effective_ms   = ms;
  custom_present = true;
  return true;
}

bool clearCustom() {
  if (!custom_present) {
    return false;
  }

  prefs.remove(NVS_KEY_GRIND_MS);
  effective_ms   = DEFAULT_GRIND_MS;
  custom_present = false;
  return true;
}

}  // namespace Settings

// End of file /////////////////////////////////////////////////////////////////
