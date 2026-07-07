/*
 * AM32 WiFi Link for ESP32 / ESP32-C3
 * Based on BrushlessPower/BlHeli-Passthrough (ESP32 target).
 * 4Way.cpp / ESC_Serial.cpp / Global.cpp / MSP.cpp / serial_comm.cpp are
 * REUSED UNCHANGED from that project - only the transport (BLE -> WiFi AP +
 * embedded web GUI) is new. See am32_link.cpp / wifi_comm.cpp / web_page.h.
 *
 * USB serial passthrough (process_serial) is left running too, so you can
 * still plug into esc-configurator.com over USB as a fallback/sanity check.
 */

#include <Arduino.h>
#include "Global.h"
#include "serial_comm.h"   // USB-serial 4-way/MSP passthrough (unchanged, optional)
#include "wifi_comm.h"     // NEW: WiFi AP + embedded web GUI

void setup() {
  Serial.begin(115200);
  init_wifi();
}

void loop() {
  process_serial();  // still works over USB if you want to use esc-configurator.com directly
  process_wifi();
}
