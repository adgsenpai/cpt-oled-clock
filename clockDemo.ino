#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "secrets.h"

// Matches the known-working HW-364 setup: SCL=GPIO14 (D5), SDA=GPIO12 (D6).
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0, /* clock=*/14, /* data=*/12, U8X8_PIN_NONE);

uint32_t lastDraw = 0;
uint8_t animationFrame = 0;
int lastReportedWifiStatus = -1;
bool ntpStarted = false;
bool timeSyncReported = false;

const char *wifiStatusLabel(wl_status_t status) {
  switch (status) {
    case WL_NO_SSID_AVAIL: return "NO SSID";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "WAITING";
  }
}

void drawClockFace(const struct tm &timeInfo, bool timeValid) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(4, 10, "CPT / TIME");
  u8g2.drawCircle(119, 7, 5);
  u8g2.drawHLine(4, 15, 120);

  const uint8_t orbitX[] = {8, 10, 12, 10};
  const uint8_t orbitY[] = {28, 25, 28, 31};
  u8g2.drawCircle(10, 28, 6);
  u8g2.drawDisc(orbitX[animationFrame], orbitY[animationFrame], 2);

  char timeText[6];
  if (timeValid) {
    snprintf(timeText, sizeof(timeText), "%02d:%02d",
             timeInfo.tm_hour, timeInfo.tm_min);
  } else {
    snprintf(timeText, sizeof(timeText), "--:--");
  }
  u8g2.setFont(u8g2_font_ncenB24_tr);
  u8g2.drawStr(19, 44, timeText);

  u8g2.setFont(u8g2_font_6x10_tf);
  if (timeValid) {
    static const char *DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    static const char *MONTHS[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    char dateText[24];
    snprintf(dateText, sizeof(dateText), "%s %02d %s %04d",
             DAYS[timeInfo.tm_wday], timeInfo.tm_mday,
             MONTHS[timeInfo.tm_mon], timeInfo.tm_year + 1900);
    u8g2.drawStr(15, 55, dateText);
  } else if (WiFi.status() == WL_CONNECTED) {
    u8g2.drawStr(34, 55, "SYNCING TIME...");
  } else {
    char statusText[22];
    snprintf(statusText, sizeof(statusText), "WIFI: %s", wifiStatusLabel(WiFi.status()));
    u8g2.drawStr(21, 55, statusText);
  }

  u8g2.drawFrame(4, 60, 120, 3);
  if (timeValid) {
    const uint8_t progress = map(timeInfo.tm_sec, 0, 59, 0, 118);
    u8g2.drawBox(5, 61, progress, 1);
  } else {
    u8g2.drawPixel(5 + ((millis() / 90) % 118), 61);
  }

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println(F("Starting HW-364 clock with known-working U8g2 software I2C"));

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
  u8g2.drawStr(0, 15, "OLED DRIVER OK");
  u8g2.drawStr(0, 35, "Connecting WiFi");
  u8g2.sendBuffer();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setAutoReconnect(true);
  Serial.printf("Connecting to SSID: %s\n", WIFI_SSID);

  // Cape Town uses South Africa Standard Time (UTC+2), with no DST.
  configTime(2 * 60 * 60, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  const uint32_t now = millis();
  const wl_status_t wifiStatus = WiFi.status();
  if (wifiStatus != lastReportedWifiStatus) {
    lastReportedWifiStatus = wifiStatus;
    Serial.printf("Wi-Fi status: %d (%s)\n",
                  static_cast<int>(wifiStatus), wifiStatusLabel(wifiStatus));
    if (wifiStatus == WL_CONNECTED) {
      Serial.printf("Connected. IP address: %s\n", WiFi.localIP().toString().c_str());
      if (!ntpStarted) {
        // Start NTP only after Wi-Fi is online and try multiple independent servers.
        configTime(2 * 60 * 60, 0,
                   "time.google.com", "pool.ntp.org", "time.nist.gov");
        ntpStarted = true;
        Serial.println(F("NTP sync started"));
      }
    }
  }

  if (now - lastDraw >= 250) {
    lastDraw = now;
    animationFrame = (animationFrame + 1) % 4;

    struct tm timeInfo = {};
    const time_t currentTime = time(nullptr);
    const bool timeValid = currentTime > 1704067200;
    if (timeValid) {
      localtime_r(&currentTime, &timeInfo);
      if (!timeSyncReported) {
        Serial.printf("Time synchronized: %04d-%02d-%02d %02d:%02d:%02d\n",
                      timeInfo.tm_year + 1900, timeInfo.tm_mon + 1,
                      timeInfo.tm_mday, timeInfo.tm_hour,
                      timeInfo.tm_min, timeInfo.tm_sec);
        timeSyncReported = true;
      }
    }

    drawClockFace(timeInfo, timeValid);
  }

  delay(10);
}
