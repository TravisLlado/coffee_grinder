/*
 * Coffee Grinder Timer
 *
 * Pressing the onboard BOOT button drives GPIO 4 high for a fixed duration,
 * switching the optocoupler that runs the grinder. The duration defaults to
 * DEFAULT_GRIND_MS and can be overridden over WebSerial; an override is saved
 * to flash and used until it is cleared.
 *
 * Threading: WiFi and the web server already run on their own FreeRTOS tasks
 * (esp_wifi, and AsyncTCP's service task), so nothing here may block. setup()
 * never waits for the network, and loop() never calls delay(). The button and
 * the grinder therefore keep working with the network down or absent.
 *
 * The WebSerial handler runs on the AsyncTCP task, so it never touches the
 * grinder directly - it only raises a request flag that loop() acts on. loop()
 * is the single owner of the grinder, which keeps the timing correct without
 * any locking.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

#include "config.h"
#include "credentials.h"
#include "button.h"
#include "grinder.h"
#include "settings.h"

// Function prototypes /////////////////////////////////////////////////////////

void buttonAction();
void onWebSerialMessage(uint8_t* data, size_t len);
void serviceRequests();
void serviceNetwork();
void printStatus();
void printHelp();
void say(const String& msg);

// Globals /////////////////////////////////////////////////////////////////////

AsyncWebServer server(SERVER_PORT);

Button button(PIN_BUTTON, BUTTON_DEBOUNCE_MS, buttonAction);

// Grinder commands arriving from the WebSerial handler, consumed by loop()
volatile bool runRequested{false};
volatile bool stopRequested{false};

uint32_t lastTickMs{0};
uint32_t lastNetCheckMs{0};
uint32_t lastReconnectMs{0};

bool wifiWasConnected{false};
bool mdnsStarted{false};

// Helpers /////////////////////////////////////////////////////////////////////

// Echo to both the USB serial port and the WebSerial page
void say(const String& msg) {
  Serial.println(msg);
  WebSerial.println(msg);
}

// Format a duration in milliseconds as seconds with two decimals
static String secondsStr(uint32_t ms) {
  return String(ms / 1000.0F, 2) + " s";
}

// Parse a duration argument given in seconds. Returns false when the text is
// missing or is not a number, so "set banana" is rejected rather than read as 0.
static bool parseSeconds(const String& arg, float& seconds) {
  if (arg.length() == 0) {
    return false;
  }

  bool sawDigit{false};
  bool sawDot{false};

  for (size_t i{0}; i < arg.length(); ++i) {
    const char c{arg[i]};
    if (isDigit(c)) {
      sawDigit = true;
    } else if (c == '.' && !sawDot) {
      sawDot = true;
    } else {
      return false;
    }
  }

  if (!sawDigit) {
    return false;
  }

  seconds = arg.toFloat();
  return true;
}

// Button //////////////////////////////////////////////////////////////////////

// Called from button.update(), so already on the loop() task
void buttonAction() {
  const uint32_t ms{Settings::grindMs()};

  if (Grinder::start(ms)) {
    say("Button: grinding for " + secondsStr(ms));
  } else {
    say("Button: already grinding, ignored");
  }
}

// WebSerial ///////////////////////////////////////////////////////////////////

void printStatus() {
  const uint32_t ms{Settings::grindMs()};

  say("Duration: " + secondsStr(ms) +
      (Settings::hasCustom() ? " (custom, saved to flash)" : " (built-in default)"));
  say("Default:  " + secondsStr(DEFAULT_GRIND_MS));
  say(Grinder::isRunning() ? "Grinder:  RUNNING, " + secondsStr(Grinder::remainingMs()) + " left"
                           : String("Grinder:  idle"));
}

void printHelp() {
  say("Commands:");
  say("  status       show the current duration and grinder state");
  say("  set <sec>    save a custom duration to flash, e.g. 'set 7.5'");
  say("  clear        erase the custom duration, back to the default");
  say("  run          grind now, same as pressing the button");
  say("  stop         stop grinding immediately");
  say("  help         this list");
  say("Accepted range: " + secondsStr(MIN_GRIND_MS) + " to " + secondsStr(MAX_GRIND_MS));
}

// Runs on the AsyncTCP task. Must not block and must not touch the grinder.
void onWebSerialMessage(uint8_t* data, size_t len) {
  String cmd;
  for (size_t i{0}; i < len; i++) {
    cmd += (char)data[i];
  }
  cmd.trim();
  cmd.toLowerCase();

  if (cmd.length() == 0) {
    return;
  }

  // Split into a base command and its optional argument
  String baseCmd{cmd};
  String arg;
  const int space{cmd.indexOf(' ')};
  if (space >= 0) {
    baseCmd = cmd.substring(0, space);
    arg     = cmd.substring(space + 1);
    baseCmd.trim();
    arg.trim();
  }

  if (baseCmd == "status" || baseCmd == "?") {
    printStatus();

  } else if (baseCmd == "set") {
    float seconds{0.0F};
    if (!parseSeconds(arg, seconds)) {
      say("Usage: set <seconds>, e.g. 'set 7.5'");
      return;
    }

    const uint32_t ms{(uint32_t)lroundf(seconds * 1000.0F)};
    if (Settings::setCustom(ms)) {
      say("Duration set to " + secondsStr(Settings::grindMs()) + ", saved to flash");
    } else {
      say("Rejected: duration must be between " + secondsStr(MIN_GRIND_MS) +
          " and " + secondsStr(MAX_GRIND_MS));
    }

  } else if (baseCmd == "clear") {
    if (Settings::clearCustom()) {
      say("Custom duration erased, now using the default " + secondsStr(DEFAULT_GRIND_MS));
    } else {
      say("No custom duration stored, already using the default " +
          secondsStr(DEFAULT_GRIND_MS));
    }

  } else if (baseCmd == "run") {
    runRequested = true;

  } else if (baseCmd == "stop") {
    stopRequested = true;

  } else if (baseCmd == "help") {
    printHelp();

  } else {
    say("Unknown command. Type 'help' for commands.");
  }
}

// Deferred work ///////////////////////////////////////////////////////////////

// Apply grinder commands raised by the WebSerial handler. Called from loop(),
// so the grinder only ever changes state on one task.
void serviceRequests() {
  if (stopRequested) {
    stopRequested = false;

    if (Grinder::isRunning()) {
      Grinder::stop();
      say("Stopped");
    } else {
      say("Grinder is already idle");
    }
  }

  if (runRequested) {
    runRequested = false;

    const uint32_t ms{Settings::grindMs()};
    if (Grinder::start(ms)) {
      say("Grinding for " + secondsStr(ms));
    } else {
      say("Already grinding, ignored");
    }
  }
}

// Network /////////////////////////////////////////////////////////////////////

// Watch the WiFi link and react to changes. Never blocks: WiFi.begin() returns
// immediately and the connection completes on the WiFi task.
void serviceNetwork() {
  const bool connected{WiFi.status() == WL_CONNECTED};

  if (connected && !wifiWasConnected) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());

    // mDNS needs a live interface, so it starts here rather than in setup()
    if (!mdnsStarted && MDNS.begin(MDNS_HOSTNAME)) {
      MDNS.addService("http", "tcp", SERVER_PORT);
      mdnsStarted = true;
      Serial.println(String("WebSerial: http://") + MDNS_HOSTNAME + ".local/");
    }
    Serial.println("WebSerial: http://" + WiFi.localIP().toString() + "/");

  } else if (!connected && wifiWasConnected) {
    Serial.println("WiFi lost, retrying in the background");
  }

  // Backstop for the core's own auto-reconnect
  if (!connected && (millis() - lastReconnectMs) >= RECONNECT_INTERVAL_MS) {
    lastReconnectMs = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  wifiWasConnected = connected;
}

// Setup ///////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  // Grinder output first, so the pin is driven low before anything else runs
  Grinder::begin();
  button.begin();
  Settings::begin();

  Serial.println();
  Serial.println("Coffee Grinder Timer");
  Serial.println("Duration: " + secondsStr(Settings::grindMs()) +
                 (Settings::hasCustom() ? " (custom)" : " (default)"));

  // Kick off WiFi and return immediately; the link comes up on its own task
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastReconnectMs = millis();

  // The server binds to any address, so it can start before the link is up
  WebSerial.onMessage(onWebSerialMessage);
  WebSerial.begin(&server, "/");
  server.begin();

  Serial.println("Ready. The button works whether or not WiFi connects.");
}

// Loop ////////////////////////////////////////////////////////////////////////

void loop() {
  const uint32_t now{millis()};

  if ((now - lastTickMs) >= TICK_PERIOD_MS) {
    lastTickMs = now;
    button.update();
    Grinder::update();
    serviceRequests();
  }

  if ((now - lastNetCheckMs) >= NET_CHECK_INTERVAL_MS) {
    lastNetCheckMs = now;
    serviceNetwork();
  }
}

// End of file /////////////////////////////////////////////////////////////////
