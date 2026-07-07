#pragma once
#include <Arduino.h>

// High-level helpers built on top of the UNMODIFIED Check_4Way()/SendESC()/
// GetESC() from 4Way.cpp / ESC_Serial.cpp. These just assemble the same byte
// packets a real 4-way host (esc-configurator, BLHeliSuite, etc.) would send,
// so the ESC-facing behaviour is byte-for-byte identical to the BLE bridge -
// only the transport in front of it changes.

struct Am32Info {
  bool ok;
  uint8_t sig_lo;
  uint8_t sig_hi;
  uint8_t boot_rev;
  uint8_t interface_mode;
};

// Opens the single-wire UART to the ESC and performs cmd_DeviceInitFlash.
// Must succeed before read/write calls.
Am32Info am32_connect();

// Ends the passthrough session and releases the signal pin.
void am32_disconnect();

// Reads AM32_EEPROM_SIZE (192) bytes from the ESC's settings page into out[].
// Returns true on success (ack==ACK_OK and CRC valid).
bool am32_read_eeprom(uint8_t *out, size_t len);

// Erases the settings page and writes len bytes (<=192) back to it.
// *** WRITE PATH — see README caveat before trusting this on real hardware ***
bool am32_write_eeprom(const uint8_t *in, size_t len);
