# Cape Town OLED Clock

An ESP8266 clock for the HW-364 / D-duino-style board with an onboard 128x64 SSD1306 OLED. It connects to Wi-Fi, syncs time from NTP, and displays the Cape Town date and time with a small animated graphic.

## Hardware and display setup

- ESP8266 NodeMCU-based HW-364 board
- Built-in 128x64 SSD1306 OLED
- The sketch uses U8g2 software I²C, matching the working setup for this board:
  - Clock: GPIO14
  - Data: GPIO12

Board revisions may wire or label the OLED pins differently. The HW-364 reference example and this project's known-working setup use different SDA/SCL assignments, so if adapting the sketch, verify the pin order against your exact board revision.

## Configure Wi-Fi

Copy `secrets.h.example` to `secrets.h` and enter your Wi-Fi network name and password:

```sh
cp secrets.h.example secrets.h
```

`secrets.h` is ignored by Git and must not be committed.

## Libraries and board

- ESP8266 Arduino core
- U8g2 by olikraus
- Board profile used for compilation: NodeMCU 1.0 (ESP-12E Module), `esp8266:esp8266:nodemcuv2`

Install U8g2 from Arduino IDE's Library Manager. With Arduino CLI, install the ESP8266 core and library, then build and upload from this directory:

```sh
arduino-cli core install esp8266:esp8266
arduino-cli lib install U8g2
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .
arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 --port /dev/ttyUSB0 .
```

Use the serial monitor at 115200 baud for connection and time-sync messages. The sketch applies UTC+2 for South Africa and tries multiple NTP servers after Wi-Fi connects.
