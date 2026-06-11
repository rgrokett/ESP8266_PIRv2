# ESP8266 PIR Motion Detector

Battery-powered PIR motion alarm for an Adafruit HUZZAH ESP8266. A PIR-driven
reset wakes the ESP8266, which sends one authenticated HTTPS request to IFTTT
Webhooks and immediately returns to indefinite deep sleep.

The bundled PDF documents the original 2017 hardware build. Its software,
certificate fingerprint, and security instructions are historical; use this
README for the current firmware.

## Current behavior

- Validates `maker.ifttt.com` with Amazon Root CA 1 instead of disabling TLS
  verification or pinning a short-lived server certificate.
- Synchronizes UTC time with NTP before certificate validation.
- Follows at most three HTTPS redirects.
- Times out Wi-Fi association, NTP synchronization, TLS, and HTTP reads.
- Checks for a successful 2xx HTTP response.
- Never prints the Wi-Fi password or IFTTT Webhooks key.
- Returns to deep sleep after success or failure to limit battery drain.
- Builds with ESP8266 Arduino core 3.1.2.

The IFTTT certificate chain was checked against Amazon Root CA 1 on
June 11, 2026. The root certificate embedded in
`ESP8266_PIRv2/IFTTTCertificate.h` expires on January 17, 2038. Recheck the
chain if IFTTT changes certificate authorities.

## Requirements

- Adafruit HUZZAH ESP8266 and the PIR/reset circuit described in
  `ESP8266_PIRv2.pdf`
- An IFTTT Webhooks applet
- [Arduino CLI](https://arduino.github.io/arduino-cli/) for command-line builds,
  or Arduino IDE with ESP8266 core 3.1.2

## Configure credentials

Create the ignored local configuration file:

```sh
cp ESP8266_PIRv2/secrets.example.h ESP8266_PIRv2/secrets.h
```

Edit `ESP8266_PIRv2/secrets.h`:

```cpp
constexpr char WIFI_SSID[] = "your-network";
constexpr char WIFI_PASSWORD[] = "your-password";
constexpr char IFTTT_KEY[] = "your-webhooks-key";
constexpr char IFTTT_EVENT[] = "pirtrigger";
```

`secrets.h` is excluded by `.gitignore`. Do not commit it. When that file is
absent, the sketch uses placeholder values from `secrets.example.h`, allowing
CI to verify compilation without real credentials.

To use a static address, uncomment the static IP block in `secrets.h` and
adjust all four addresses for the local network.

## Build

The build script installs ESP8266 core 3.1.2 when necessary and compiles for
the HUZZAH target:

```sh
./scripts/compile.sh
```

Equivalent direct command when the core is already installed:

```sh
arduino-cli compile --fqbn esp8266:esp8266:huzzah ESP8266_PIRv2
```

GitHub Actions runs the same pinned build for every push and pull request.

Upload with Arduino IDE or:

```sh
arduino-cli board list
arduino-cli upload -p /dev/ttyUSB0 \
  --fqbn esp8266:esp8266:huzzah ESP8266_PIRv2
```

Replace `/dev/ttyUSB0` with the detected serial port. Put the HUZZAH into its
bootloader mode before uploading.

## Operation and timeouts

On each reset the firmware:

1. Connects to Wi-Fi, with a 20-second limit.
2. Synchronizes time over NTP, with a 15-second limit.
3. Establishes a CA-validated TLS connection to IFTTT.
4. Sends the configured event and accepts only a 2xx response.
5. Turns Wi-Fi off and enters indefinite deep sleep.

Any failure also leads to deep sleep instead of leaving the radio powered
forever. Diagnostic serial messages identify the failed stage but redact the
IFTTT key.

## Motion cooldown

The old firmware remained awake for 15 seconds after every request. That
consumed unnecessary battery and did not provide durable rate limiting across
resets. This version sleeps immediately.

Set the PIR module's hardware retrigger/delay controls to the desired cooldown,
or add an external one-shot circuit if strict event spacing is required. With
the documented transistor reset circuit, verify that RESET is released before
the next motion event. An indefinite deep sleep has no reliable elapsed-time
clock for enforcing a software cooldown while the ESP8266 is powered down.

## TLS maintenance

TLS validation depends on:

- Working DNS and NTP access
- A correct synchronized clock
- IFTTT continuing to serve a chain trusted by the embedded Amazon root

If TLS begins failing after an IFTTT infrastructure change, inspect the live
chain and replace `IFTTT_ROOT_CA` with the appropriate long-lived root CA.
Never work around certificate errors by calling `setInsecure()`.

## Legacy documentation

The original project article and source remain useful for enclosure and wiring
background:

- <http://www.instructables.com/id/The-Cat-Has-Left-the-Building-ESP8266-PIR-Monitor/>
- <https://github.com/rgrokett/ESP8266_PIR>

The API screens, certificate fingerprint, delays, and code snippets in the PDF
may no longer match current IFTTT or ESP8266 libraries.
