/*
 * ESP8266 PIR motion detector with deep sleep and IFTTT Webhooks.
 *
 * The ESP8266 boots when the PIR circuit resets it, sends one authenticated
 * HTTPS request, and immediately returns to deep sleep.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <time.h>

#include "HTTPSRedirect.h"
#include "IFTTTCertificate.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#warning "Using placeholder credentials from secrets.example.h"
#endif

namespace {

constexpr char IFTTT_HOST[] = "maker.ifttt.com";
constexpr uint16_t HTTPS_PORT = 443;
constexpr uint8_t LED_PIN = 0;
constexpr uint32_t WIFI_TIMEOUT_MS = 20000;
constexpr uint32_t NTP_TIMEOUT_MS = 15000;
constexpr uint32_t HTTP_TIMEOUT_MS = 15000;
constexpr unsigned int MAX_REDIRECTS = 3;
constexpr time_t MIN_VALID_TIME = 1704067200;  // 2024-01-01 00:00:00 UTC

BearSSL::X509List iftttRootCa(IFTTT_ROOT_CA);

void blink(uint8_t count) {
  for (uint8_t i = 0; i < count; ++i) {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
  }
}

void sleepUntilMotion() {
  Serial.println(F("Entering deep sleep"));
  Serial.flush();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1);
  ESP.deepSleep(0);
  while (true) {
    delay(1000);
  }
}

bool connectWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

#if defined(USE_STATIC_IP)
  if (!WiFi.config(STATIC_IP, STATIC_GATEWAY, STATIC_SUBNET, STATIC_DNS)) {
    Serial.println(F("Static IP configuration failed"));
    return false;
  }
#endif

  Serial.print(F("Connecting to Wi-Fi"));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < WIFI_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Wi-Fi connection timed out"));
    return false;
  }

  Serial.print(F("Wi-Fi connected, IP: "));
  Serial.println(WiFi.localIP());
  return true;
}

bool synchronizeClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print(F("Synchronizing clock"));

  const uint32_t startedAt = millis();
  time_t now = time(nullptr);
  while (now < MIN_VALID_TIME && millis() - startedAt < NTP_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
    now = time(nullptr);
  }
  Serial.println();

  if (now < MIN_VALID_TIME) {
    Serial.println(F("Clock synchronization timed out"));
    return false;
  }

  Serial.println(F("Clock synchronized"));
  return true;
}

bool sendEvent() {
  HTTPSRedirect client(HTTPS_PORT);
  client.setTrustAnchors(&iftttRootCa);
  client.setTimeout(HTTP_TIMEOUT_MS);
  client.setMaxRedirects(MAX_REDIRECTS);

  Serial.print(F("Connecting securely to "));
  Serial.println(IFTTT_HOST);
  if (!client.connect(IFTTT_HOST, HTTPS_PORT)) {
    char error[128];
    client.getLastSSLError(error, sizeof(error));
    Serial.print(F("TLS connection failed: "));
    Serial.println(error);
    return false;
  }

  const String url =
      String(F("/trigger/")) + IFTTT_EVENT + F("/with/key/") + IFTTT_KEY;
  Serial.print(F("Sending IFTTT event: "));
  Serial.println(IFTTT_EVENT);

  if (!client.GET(url, IFTTT_HOST)) {
    Serial.print(F("IFTTT request failed, HTTP status: "));
    Serial.println(client.getStatusCode());
    return false;
  }

  const int statusCode = client.getStatusCode();
  if (statusCode < 200 || statusCode >= 300) {
    Serial.print(F("IFTTT returned HTTP status: "));
    Serial.println(statusCode);
    return false;
  }

  Serial.print(F("IFTTT event accepted, HTTP status: "));
  Serial.println(statusCode);
  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  if (!connectWiFi()) {
    sleepUntilMotion();
  }

  blink(4);

  if (!synchronizeClock()) {
    sleepUntilMotion();
  }

  sendEvent();
  sleepUntilMotion();
}

void loop() {
}
