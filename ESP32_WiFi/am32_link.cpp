#include <Arduino.h>
#include "am32_link.h"
#include "Global.h"
#include "4Way.h"
#include "ESC_Serial.h"
#include "eeprom_map.h"

// Shared scratch buffer for building/parsing 4-way frames.
// Check_4Way() rewrites this buffer in place (request in, response out),
// exactly like it did when fed from ble_rx[] in ble_comm.cpp.
static uint8_t g_buf[300];

// Builds: [0x2F][cmd][addrHi][addrLo][paramLen][...param...][crcHi][crcLo]
// paramLen == 0 means "256 bytes of param" per the 4-way spec.
static uint16_t build_request(uint8_t cmd, uint16_t addr, uint8_t paramLen, const uint8_t *param) {
  g_buf[0] = cmd_Local_Escape;
  g_buf[1] = cmd;
  g_buf[2] = (addr >> 8) & 0xFF;
  g_buf[3] = addr & 0xFF;
  g_buf[4] = paramLen;

  uint16_t n = (paramLen == 0) ? 256 : paramLen;
  for (uint16_t i = 0; i < n; i++) {
    g_buf[5 + i] = param ? param[i] : 0x00;
  }

  uint16_t crc = 0;
  uint16_t crcLen = 5 + n;
  for (uint16_t i = 0; i < crcLen; i++) {
    crc = _crc_xmodem_update(crc, g_buf[i]);
  }
  g_buf[crcLen] = (crc >> 8) & 0xFF;
  g_buf[crcLen + 1] = crc & 0xFF;
  return crcLen + 2;
}

// Word-address the bootloader expects for Read/Write (classic 4-way
// convention: value sent is a 16-bit WORD address, real byte address = value*2)
static uint16_t eeprom_word_addr() {
  uint32_t byteOffset = ESC_EEPROM_FLASH_ADDR - 0x08000000UL;
  return (uint16_t)(byteOffset / 2);
}

// Page number for cmd_DevicePageErase. NOTE: 4Way.cpp's existing handler
// hardcodes the page math as word_address = page*1024 (-> byte offset =
// page*2048), i.e. it assumes 2KB flash pages regardless of what you set
// ESC_FLASH_PAGE_SIZE to. This matches STM32G071 targets (most common AM32
// board). If your ESC uses a 1KB-page MCU (the 0x08007c00 address case),
// you'll need to patch cmd_DevicePageErase in 4Way.cpp to match, or erase
// manually - don't trust this page number blindly on those targets.
static uint8_t eeprom_page_number() {
  uint32_t byteOffset = ESC_EEPROM_FLASH_ADDR - 0x08000000UL;
  return (uint8_t)(byteOffset / 2048UL);
}

Am32Info am32_connect() {
  Am32Info info = {false, 0, 0, 0, 0};

  InitSerialOutput(); // opens the single-wire UART, sets Enable4Way = true

  uint8_t param[1] = {0x00}; // ESC index 0
  build_request(cmd_DeviceInitFlash, 0x0000, 1, param);
  uint16_t rlen = Check_4Way(g_buf);
  (void)rlen;

  uint8_t oParamLen = g_buf[4] == 0 ? 256 : g_buf[4];
  uint8_t ack = g_buf[5 + oParamLen];

  if (ack == ACK_OK) {
    info.ok = true;
    info.sig_lo = g_buf[5];
    info.sig_hi = g_buf[6];
    info.boot_rev = g_buf[7];
    info.interface_mode = g_buf[8];
  }
  return info;
}

void am32_disconnect() {
  DeinitSerialOutput();
}

bool am32_read_eeprom(uint8_t *out, size_t len) {
  if (len > 256) return false;

  uint8_t param[1] = {(uint8_t)len}; // requested read length lives in the param VALUE
  build_request(cmd_DeviceRead, eeprom_word_addr(), 1, param);
  Check_4Way(g_buf);

  uint8_t oParamLen = g_buf[4] == 0 ? 256 : g_buf[4];
  uint8_t ack = g_buf[5 + oParamLen];

  if (ack != ACK_OK) return false;
  memcpy(out, &g_buf[5], min((size_t)oParamLen, len));
  return true;
}

bool am32_write_eeprom(const uint8_t *in, size_t len) {
  if (len > 192) return false;

  // 1) Erase the settings page first - flash bits can only go 1->0 without
  //    an erase, so a raw write over existing settings will silently corrupt
  //    data if you skip this.
  uint8_t eraseParam[1] = {eeprom_page_number()};
  build_request(cmd_DevicePageErase, 0x0000, 1, eraseParam);
  Check_4Way(g_buf);
  {
    uint8_t oParamLen = g_buf[4] == 0 ? 256 : g_buf[4];
    uint8_t ack = g_buf[5 + oParamLen];
    if (ack != ACK_OK) return false;
  }

  // 2) Program the page with the new 192-byte settings block.
  build_request(cmd_DeviceWrite, eeprom_word_addr(), (uint8_t)len, in);
  Check_4Way(g_buf);
  {
    uint8_t oParamLen = g_buf[4] == 0 ? 256 : g_buf[4];
    uint8_t ack = g_buf[5 + oParamLen];
    return ack == ACK_OK;
  }
}
