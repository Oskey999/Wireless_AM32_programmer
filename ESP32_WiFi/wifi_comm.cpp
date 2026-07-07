#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "wifi_comm.h"
#include "web_page.h"
#include "am32_link.h"
#include "eeprom_map.h"

// ---- Configure your AP here ----
static const char *AP_SSID = "AM32-ESC";
static const char *AP_PASS = "flashme123"; // 8+ chars required by WiFi.softAP
static const char *MDNS_HOSTNAME = "am32"; // -> http://am32.local

static WebServer server(80);
static Am32Eeprom_t g_eeprom; // last-read cache, used for read-modify-write on POST

// ---------- tiny JSON helpers (no external library needed) ----------

static void appendField(String &json, const Am32Param &p, bool comma) {
  json += "{\"key\":\"";
  json += p.key;
  json += "\",\"offset\":";
  json += p.offset;
  json += ",\"label\":\"";
  json += p.label;
  json += "\",\"hint\":\"";
  json += p.hint;
  json += "\"}";
  if (comma) json += ",";
}

// Finds "key":NUMBER in a flat JSON object string. Good enough for our
// fixed, known set of uint8 fields - swap for ArduinoJson if you need
// something more robust (nested objects, strings, etc).
static bool findIntValue(const String &body, const char *key, long &out) {
  String needle = String("\"") + key + "\"";
  int idx = body.indexOf(needle);
  if (idx < 0) return false;
  idx = body.indexOf(':', idx);
  if (idx < 0) return false;
  idx++;
  while (idx < (int)body.length() && (body[idx] == ' ')) idx++;
  int start = idx;
  while (idx < (int)body.length() && (isDigit(body[idx]) || body[idx] == '-')) idx++;
  if (idx == start) return false;
  out = body.substring(start, idx).toInt();
  return true;
}

// ---------- route handlers ----------

static void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

static void handleFields() {
  String json = "[";
  for (size_t i = 0; i < AM32_PARAM_COUNT; i++) {
    appendField(json, AM32_PARAMS[i], i + 1 < AM32_PARAM_COUNT);
  }
  json += "]";
  server.send(200, "application/json", json);
}

static void handleStatus() {
  Am32Info info = am32_connect();
  String json = "{";
  json += "\"ok\":"; json += info.ok ? "true" : "false";
  json += ",\"sig_lo\":"; json += info.sig_lo;
  json += ",\"sig_hi\":"; json += info.sig_hi;
  json += ",\"boot_rev\":"; json += info.boot_rev;
  json += ",\"interface_mode\":"; json += info.interface_mode;
  json += "}";
  server.send(200, "application/json", json);
  // NOTE: connection to the ESC bootloader is left open (Enable4Way=true)
  // so the follow-up Read/Write calls can use it immediately.
}

static void handleGetEeprom() {
  bool ok = am32_read_eeprom(g_eeprom.buffer, AM32_EEPROM_SIZE);
  String json = "{\"ok\":"; json += ok ? "true" : "false";
  json += ",\"values\":{";
  for (size_t i = 0; i < AM32_PARAM_COUNT; i++) {
    json += "\""; json += AM32_PARAMS[i].key; json += "\":";
    json += g_eeprom.buffer[AM32_PARAMS[i].offset];
    if (i + 1 < AM32_PARAM_COUNT) json += ",";
  }
  json += "}}";
  server.send(200, "application/json", json);
}

static void handlePostEeprom() {
  String body = server.arg("plain");

  // Read-modify-write: start from the last known-good 192 bytes (read fresh
  // if we haven't already) so reserved_*/tune[]/can.* survive untouched.
  am32_read_eeprom(g_eeprom.buffer, AM32_EEPROM_SIZE);

  for (size_t i = 0; i < AM32_PARAM_COUNT; i++) {
    long v;
    if (findIntValue(body, AM32_PARAMS[i].key, v)) {
      g_eeprom.buffer[AM32_PARAMS[i].offset] = (uint8_t)constrain(v, 0, 255);
    }
  }

  bool ok = am32_write_eeprom(g_eeprom.buffer, AM32_EEPROM_SIZE);
  String json = "{\"ok\":"; json += ok ? "true" : "false"; json += "}";
  server.send(200, "application/json", json);
}

void init_wifi(void) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP started, connect to http://");
  Serial.println(WiFi.softAPIP()); // still works as a fallback: 192.168.4.1

  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS ready: http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS init failed - fall back to the IP address above");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/fields", HTTP_GET, handleFields);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/eeprom", HTTP_GET, handleGetEeprom);
  server.on("/api/eeprom", HTTP_POST, handlePostEeprom);
  server.begin();
}

void process_wifi(void) {
  server.handleClient();
}