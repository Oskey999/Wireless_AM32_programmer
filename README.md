# Wireless_AM32_programmer
Turn an esp32 c3 into a programmer that connects through wifi to your phone

Using mostly the esp32 code from this repo https://github.com/BrushlessPower/BlHeli-Passthrough/tree/main

# Usage
open ESP32_WIFI.ino in arduino studio
needed Libraries for ESP32 use
    Espressif ESP32: https://github.com/espressif/arduino-esp32
    NimBLE-Arduino: https://github.com/h2zero/NimBLE-Arduino
    ESPSoftwareserial: https://github.com/plerup/espsoftwareserial/
all Libraries are available in Arduino Library Manager

using GPIO pin 4 as the signal wire
connect the esp32 to power and use like any other programmer

connect to wifi access point AM32-ESC
with password : flashme123

you can access the gui at http://am32.local

programming can be very unintuiitve because it is just the bytes for now, for fully understanding the bytes refer to here: https://wiki.am32.ca/development/Open-ESC-EEPROM-Format.html

# Modifications
change wifi properties such as name and password in wifi_comm.cpp with
''' cpp
static const char *AP_SSID = "AM32-ESC";
static const char *AP_PASS = "flashme123"; // 8+ chars required by WiFi.softAP
static const char *MDNS_HOSTNAME = "am32"; // -> http://am32.local
'''
change GPIO pin in Global.h through #define SERVO_OUT



