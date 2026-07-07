# AM32 WiFi Link (ESP32 / ESP32-C3)

Self-contained WiFi version of BrushlessPower's [BlHeli-Passthrough](https://github.com/BrushlessPower/BlHeli-Passthrough)
ESP32 target, in the style of [ESCape32's WiFiLink](https://github.com/neoxic/ESCape32/wiki/WiFiLink):
the ESP32 hosts its own WiFi access point and web GUI, so you connect a phone
or laptop straight to it with no other app required.

## What changed vs. the original repo

**Unchanged, reused as-is:**
- `4Way.cpp` / `4Way.h` — the actual 4-way protocol state machine (`Check_4Way`)
- `ESC_Serial.cpp` / `ESC_Serial.h` — single-wire UART to the ESC (`SendESC`/`GetESC`)
- `Global.cpp` / `Global.h`, `MSP.cpp` / `MSP.h`, `serial_comm.cpp` / `serial_comm.h`

This is the part that actually talks to the ESC bootloader — it's proven code,
so it's left untouched. Only the transport in front of it changed.

**New:**
- `wifi_comm.cpp/h` — replaces `ble_comm.cpp/h`. Starts a WiFi AP and a small
  web server instead of a BLE GATT service.
- `web_page.h` — the embedded single-page GUI (served directly from flash,
  no filesystem/SPIFFS needed).
- `am32_link.cpp/h` — high-level connect/read/write helpers that build the
  same byte-for-byte 4-way request packets the old BLE bridge used, but call
  `Check_4Way()` directly instead of relaying bytes to/from a phone.
- `eeprom_map.h` — named field table mirroring [AM32's `eeprom.h`](https://github.com/am32-firmware/AM32/blob/main/Inc/eeprom.h)
  (byte offsets 0–191), used to turn the raw settings block into the web
  form and back. Cross-referenced against the [EEPROM format wiki page](https://wiki.am32.ca/development/Open-ESC-EEPROM-Format.html).

## Wiring

Same as the original passthrough project: one GPIO (`SERVO_OUT`, default
GPIO16 in `Global.h`) to the ESC's signal pad, plus GND. It's single-wire
half-duplex — `ESC_Serial.cpp` already handles the TX/RX direction switching
for you.

## Libraries needed

- ESP32 board package (works on classic ESP32 and ESP32-C3)
- `WiFi.h` and `WebServer.h` — both ship with the ESP32 Arduino core, no
  extra install needed (deliberately avoided `ESPAsyncWebServer`/`AsyncTCP`
  since AsyncTCP's ESP32-C3/RISC-V support has historically been spotty)

## Using it

1. Flash `ESP32_WiFi.ino` (with all the `.h`/`.cpp` files alongside it) to
   your ESP32-C3.
2. Connect the ESP32 to the ESC's signal wire + GND, power the ESC.
3. On your phone/laptop, join the WiFi network `AM32-ESC` (password in
   `wifi_comm.cpp`, change it before you fly this around other people).
4. Browse to `http://192.168.4.1/`.
5. Tap **Connect to ESC** — this runs `cmd_DeviceInitFlash` and shows the
   bootloader signature if it responds.
6. **Read Settings** pulls the 192-byte EEPROM block and populates the form
   from `eeprom_map.h`.
7. Edit values, **Write Settings** does an erase + program cycle.

## ⚠️ Before you trust the Write path

The read side and the protocol framing (`Check_4Way`, CRC, ack handling) are
solid — they're unmodified from a working, released library. The one thing
I could **not** independently verify against AM32's actual bootloader source
(a separate, unpublished repo from the main firmware) is the exact address
encoding used by `CMD_SET_ADDRESS`/`cmd_DevicePageErase`:

- `eeprom_word_addr()` assumes the classic BLHeli 4-way convention: the
  16-bit address field is a **word address**, i.e. `real_byte_offset = value * 2`.
- `eeprom_page_number()` reuses `4Way.cpp`'s existing (unmodified)
  `cmd_DevicePageErase` math, which itself assumes **2KB flash pages**
  (`word_address = page * 1024` → `byte_offset = page * 2048`). This matches
  the STM32G071 64KB/128KB targets AM32 most commonly ships on. It will
  compute the wrong page on the smaller 32KB/1KB-page target (the
  `0x08007c00` case in `eeprom_map.h`).

Both of these were cross-checked for internal consistency against the three
`eeprom_address` constants in AM32's own `Src/main.c`, but "internally
consistent" isn't the same as "confirmed against the bootloader." **Test the
Read path first and confirm the values it returns match what
esc-configurator.com shows over USB for the same ESC before trusting Write.**
If you have a logic analyzer or can capture a known-good USB passthrough
session from esc-configurator.com, comparing the exact `CMD_SET_ADDRESS`
bytes it sends is the fastest way to confirm or correct the math in
`am32_link.cpp` before flashing settings for real.

Set `ESC_EEPROM_FLASH_ADDR` in `eeprom_map.h` to match your ESC's flash size
(default is 64KB / `0x0800f800`, the common STM32G071 case).
