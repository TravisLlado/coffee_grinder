// settings.cpp ////////////////////////////////////////////////////////////////

#include "settings.h"

#include <Preferences.h>

#include "config.h"

namespace {

Preferences prefs;

// Cached copy of flash state so the hot path never touches NVS
uint32_t custom_ms{0};
bool     custom_present{false};

}  // namespace

namespace Settings {

void begin() {
  prefs.begin(NVS_NAMESPACE, false);

  custom_present = prefs.isKey(NVS_KEY_GRIND_MS);
  custom_ms      = custom_present ? prefs.getUInt(NVS_KEY_GRIND_MS, DEFAULT_GRIND_MS) : 0;

  // A stored value outside the accepted range means corrupt or stale flash;
  // drop it rather than driving the grinder for an unexpected length of time.
  if (custom_present && (custom_ms < MIN_GRIND_MS || custom_ms > MAX_GRIND_MS)) {
    prefs.remove(NVS_KEY_GRIND_MS);
    custom_present = false;
    custom_ms      = 0;
  }
}

uint32_t grindMs() {
  return custom_present ? custom_ms : DEFAULT_GRIND_MS;
}

bool hasCustom() {
  return custom_present;
}

bool setCustom(uint32_t ms) {
  if (ms < MIN_GRIND_MS || ms > MAX_GRIND_MS) {
    return false;
  }

  // Skip the flash write when nothing would change, to spare NVS wear
  if (custom_present && custom_ms == ms) {
    return true;
  }

  if (prefs.putUInt(NVS_KEY_GRIND_MS, ms) == 0) {
    return false;
  }

  custom_ms      = ms;
  custom_present = true;
  return true;
}

bool clearCustom() {
  if (!custom_present) {
    return false;
  }

  prefs.remove(NVS_KEY_GRIND_MS);
  custom_present = false;
  custom_ms      = 0;
  return true;
}

}  // namespace Settings

// End of file /////////////////////////////////////////////////////////////////
