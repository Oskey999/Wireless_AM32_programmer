#pragma once
#include <Arduino.h>

// Mirrors AM32-firmware/Inc/eeprom.h byte-for-byte.
// Source of truth: https://github.com/am32-firmware/AM32/blob/main/Inc/eeprom.h
// Cross-check against https://wiki.am32.ca/development/Open-ESC-EEPROM-Format.html
// whenever AM32 bumps eeprom_version — offsets are NOT guaranteed stable across
// major firmware revisions.

#define AM32_EEPROM_SIZE 192

// ---- Set this for YOUR ESC's target MCU / flash size ----
// Known values pulled from AM32 firmware Src/main.c (eeprom_address selection):
//   32 KB flash, 1 KB pages  -> 0x08007c00
//   64 KB flash, 2 KB pages  -> 0x0800f800   (most common / STM32G071 default)
//  128 KB flash, 2 KB pages  -> 0x0801f800
#define ESC_EEPROM_FLASH_ADDR   0x0800f800UL
#define ESC_FLASH_PAGE_SIZE     2048UL   // 1024 for the 32KB/1KB-page target above

// Struct layout - identical field order/size to AM32's eeprom.h
typedef union {
  struct {
    uint8_t reserved_0;              // 0
    uint8_t eeprom_version;          // 1
    uint8_t reserved_1;              // 2
    uint8_t version_major;           // 3
    uint8_t version_minor;           // 4
    uint8_t max_ramp;                // 5  0.1%/ms .. 25%/ms
    uint8_t minimum_duty_cycle;      // 6  0.2% .. 51% (x0.2)
    uint8_t disable_stick_calibration; // 7
    uint8_t absolute_voltage_cutoff; // 8  0.5V increments
    uint8_t current_P;               // 9
    uint8_t current_I;               // 10
    uint8_t current_D;               // 11
    uint8_t active_brake_power;      // 12 1-5 (x20% duty)
    uint8_t reserved_eeprom_3[4];    // 13-16
    uint8_t dir_reversed;            // 17
    uint8_t bi_direction;            // 18
    uint8_t use_sine_start;          // 19
    uint8_t comp_pwm;                // 20
    uint8_t variable_pwm;            // 21
    uint8_t stuck_rotor_protection;  // 22
    uint8_t advance_level;           // 23
    uint8_t pwm_frequency;           // 24
    uint8_t startup_power;           // 25
    uint8_t motor_kv;                // 26
    uint8_t motor_poles;             // 27
    uint8_t brake_on_stop;           // 28
    uint8_t stall_protection;        // 29
    uint8_t beep_volume;             // 30
    uint8_t telemetry_on_interval;   // 31
    uint8_t servo_low_threshold;     // 32
    uint8_t servo_high_threshold;    // 33
    uint8_t servo_neutral;           // 34
    uint8_t servo_dead_band;         // 35
    uint8_t low_voltage_cut_off;     // 36
    uint8_t low_cell_volt_cutoff;    // 37
    uint8_t rc_car_reverse;          // 38
    uint8_t use_hall_sensors;        // 39
    uint8_t sine_mode_changeover_thottle_level; // 40
    uint8_t drag_brake_strength;     // 41
    uint8_t driving_brake_strength;  // 42
    uint8_t temperature_limit;       // 43
    uint8_t current_limit;           // 44
    uint8_t sine_mode_power;         // 45
    uint8_t input_type;              // 46
    uint8_t auto_advance;            // 47
    uint8_t tune[128];               // 48-175 (opaque - round-trip only, don't edit blindly)
    uint8_t can_node;                // 176
    uint8_t can_esc_index;           // 177
    uint8_t can_require_arming;      // 178
    uint8_t can_telem_rate;          // 179
    uint8_t can_require_zero_throttle; // 180
    uint8_t can_filter_hz;           // 181
    uint8_t can_debug_rate;          // 182
    uint8_t can_term_enable;         // 183
    uint8_t can_reserved[8];         // 184-191
  } f;
  uint8_t buffer[AM32_EEPROM_SIZE];
} Am32Eeprom_t;

// Describes one editable field for the web UI: name shown to user,
// byte offset in the struct, and a hint for how the JS should render/scale it.
struct Am32Param {
  const char *key;     // JSON key (matches struct field name)
  uint16_t offset;      // byte offset into buffer[]
  const char *label;    // human readable label
  const char *hint;     // short description / units for the UI
};

// Only the fields worth exposing as editable controls.
// (reserved_*, tune[], and can_reserved are preserved on read-modify-write
// but not shown/edited directly.)
static const Am32Param AM32_PARAMS[] = {
  {"dir_reversed",        17, "Motor Direction Reversed",        "0=Normal, 1=Reversed"},
  {"bi_direction",        18, "Bidirectional Mode",              "0=Off, 1=On, 2=On w/ brake"},
  {"use_sine_start",      19, "Sinusoidal Startup",              "0=Off, 1=On"},
  {"comp_pwm",            20, "Complementary PWM",               "0=Off, 1=On"},
  {"variable_pwm",        21, "Variable PWM Frequency",          "0=Off, 1=On"},
  {"stuck_rotor_protection", 22, "Stuck Rotor Protection",       "0=Off, 1=On"},
  {"advance_level",       23, "Timing Advance",                  "0-15 (x7.5 deg)"},
  {"pwm_frequency",       24, "PWM Frequency",                   "kHz, driver dependent"},
  {"startup_power",       25, "Startup Power",                   "50-150%"},
  {"motor_kv",            26, "Motor KV",                        "x40 (0=unused)"},
  {"motor_poles",         27, "Motor Poles",                     "count"},
  {"brake_on_stop",       28, "Brake On Stop",                   "0=Off, 1=On"},
  {"stall_protection",    29, "Stall Protection",                "0=Off, 1=On"},
  {"beep_volume",         30, "Beep Volume",                     "0-11"},
  {"telemetry_on_interval", 31, "Telemetry on Interval",         "0=Off, 1=On"},
  {"servo_low_threshold", 32, "Servo Low Threshold",             "us/4 - 4 offset"},
  {"servo_high_threshold",33, "Servo High Threshold",            "us/4 - 4 offset"},
  {"servo_neutral",       34, "Servo Neutral",                   "us/4 - 4 offset"},
  {"servo_dead_band",     35, "Servo Dead Band",                 "us"},
  {"low_voltage_cut_off", 36, "Low Voltage Cutoff",              "0=Off, else x0.1V/cell"},
  {"low_cell_volt_cutoff",37, "Low Cell Voltage Cutoff",         "x0.01V + 2.5V"},
  {"rc_car_reverse",      38, "RC Car Reverse (Bidirectional)",  "0=Off, 1=On"},
  {"use_hall_sensors",    39, "Use Hall Sensors",                "0=Off, 1=On"},
  {"drag_brake_strength", 41, "Drag Brake Strength",             "1-10 (x10%)"},
  {"driving_brake_strength",42,"Driving Brake Strength",         "1-10 (x10%)"},
  {"temperature_limit",   43, "Temperature Limit",               "70-140 C"},
  {"current_limit",       44, "Current Limit",                   "0=Off, else Amps"},
  {"sine_mode_power",     45, "Sine Mode Power",                 "1-10"},
  {"input_type",          46, "Input Type",                      "0=Auto,1=DShot,2=Servo/Oneshot,3=Serial"},
  {"auto_advance",        47, "Auto Timing Advance",             "0=Off, 1=On"},
};
static const size_t AM32_PARAM_COUNT = sizeof(AM32_PARAMS) / sizeof(AM32_PARAMS[0]);
