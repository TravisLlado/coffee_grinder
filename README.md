# Coffee Grinder Timer

A very simple ESP32-C3 timer for a coffee grinder. Press the onboard button, the
grinder runs for a fixed number of seconds, then stops.

## How it works

- The onboard **BOOT** button (GPIO 9, active low) is the trigger, with a 30 ms
  debounce.
- **GPIO 4** drives an optocoupler that switches the grinder. It is held low
  through boot so the grinder cannot glitch on at power-up.
- The default grind duration is `DEFAULT_GRIND_MS` in [config.h](src/config.h),
  currently 5 seconds.
- A custom duration can be set over WebSerial. It is saved to the ESP32's flash
  (NVS) and used instead of the default until it is cleared.
- The onboard LED is lit while the grinder is running.

Pressing the button again while a grind is in progress is ignored, so a second
press cannot extend or restart the run. Use `stop` to cut a run short.

## WebSerial

The board joins WiFi and serves WebSerial at `http://coffeegrinder.local/`
(or its IP address, which is printed to the USB serial port at boot).

| Command | Effect |
| --- | --- |
| `status` | Show the current duration, whether it is custom, and the grinder state |
| `set <sec>` | Save a custom duration to flash, e.g. `set 7.5` |
| `clear` | Erase the custom duration and go back to the built-in default |
| `run` | Grind now, same as pressing the button |
| `stop` | Stop grinding immediately |
| `help` | List the commands |

Durations are given in seconds and accepted between 0.1 s and 120 s. Anything
outside that range is rejected and nothing is written to flash.

## Building

Built with [PlatformIO](https://platformio.org/):

```
pio run -t upload
pio device monitor
```

WiFi credentials come from a `credentials.h` that is kept outside this repo and
defines `WIFI_SSID` and `WIFI_PASSWORD`. Point the `CREDENTIALS_INCLUDE`
environment variable at the directory containing it.
