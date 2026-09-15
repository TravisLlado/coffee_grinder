/*
 * Coffee Grinder Timer
 *
 * Pressing the onboard BOOT button drives GPIO 4 high for a fixed duration,
 * switching the optocoupler that runs the grinder. The duration defaults to
 * DEFAULT_GRIND_MS and can be overridden over WebSerial; an override is saved
 * to flash and used until it is cleared.
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
void connectWiFi();
void printStatus();
void printHelp();
void say(const String& msg);

// Globals /////////////////////////////////////////////////////////////////////

AsyncWebServer server(SERVER_PORT);

Button button(PIN_BUTTON, BUTTON_DEBOUNCE_MS, buttonAction);

uint32_t lastTickMs{0};
uint32_t lastWifiCheckMs{0};

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
    const uint32_t ms{Settings::grindMs()};
    if (Grinder::start(ms)) {
      say("Grinding for " + secondsStr(ms));
    } else {
      say("Already grinding, ignored");
    }

  } else if (baseCmd == "stop") {
    if (Grinder::isRunning()) {
      Grinder::stop();
      say("Stopped");
    } else {
      say("Grinder is already idle");
    }

  } else if (baseCmd == "help") {
    printHelp();

  } else {
    say("Unknown command. Type 'help' for commands.");
  }
}

// WiFi ////////////////////////////////////////////////////////////////////////

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(RETRY_DELAY_MS);
    Serial.print(".");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  Serial.println();
  Serial.println("WiFi connected: " + WiFi.localIP().toString());
}

// Setup ///////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  // Grinder output first, so the relay is held off through the rest of boot
  Grinder::begin();
  button.begin();
  Settings::begin();

  Serial.println();
  Serial.println("Coffee Grinder Timer");
  Serial.println("Duration: " + secondsStr(Settings::grindMs()) +
                 (Settings::hasCustom() ? " (custom)" : " (default)"));

  connectWiFi();

  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", SERVER_PORT);
    Serial.println(String("mDNS: http://") + MDNS_HOSTNAME + ".local/");
  } else {
    Serial.println("mDNS failed to start");
  }

  WebSerial.onMessage(onWebSerialMessage);
  WebSerial.begin(&server, "/");
  server.begin();

  Serial.println("WebSerial: http://" + WiFi.localIP().toString() + "/");

  WebSerial.println("Coffee Grinder Timer ready.");
  printStatus();
  WebSerial.println("Type 'help' for commands.");
}

// Loop ////////////////////////////////////////////////////////////////////////

void loop() {
  const uint32_t now{millis()};

  if ((now - lastTickMs) >= TICK_PERIOD_MS) {
    lastTickMs = now;
    button.update();
    Grinder::update();
  }

  // Re-join WiFi if it drops. The button and grinder keep working regardless;
  // only WebSerial needs the network.
  if ((now - lastWifiCheckMs) >= WIFI_CHECK_INTERVAL_MS) {
    lastWifiCheckMs = now;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi lost, reconnecting...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}

// End of file /////////////////////////////////////////////////////////////////
