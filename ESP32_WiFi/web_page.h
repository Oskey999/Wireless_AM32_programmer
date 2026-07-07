#pragma once

// Self-contained page: fetches /api/fields to build the form, /api/status
// to test the link, /api/eeprom (GET/POST) to read/write settings.
// No build step, no external JS libraries - just fetch() + vanilla DOM.
const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AM32 WiFi Link</title>
<style>
  body { font-family: -apple-system, sans-serif; max-width: 640px; margin: 0 auto; padding: 16px; background:#111; color:#eee; }
  h1 { font-size: 1.3em; }
  .status { padding: 8px 12px; border-radius: 6px; margin-bottom: 12px; }
  .ok { background: #1e4620; }
  .bad { background: #4a1e1e; }
  .field { display: flex; justify-content: space-between; align-items: center; padding: 8px 0; border-bottom: 1px solid #333; }
  .field label { flex: 1; }
  .field .hint { font-size: 0.75em; color: #999; display: block; }
  .field input { width: 90px; padding: 4px; background:#222; color:#eee; border:1px solid #444; border-radius:4px; }
  button { padding: 10px 16px; margin: 4px 4px 4px 0; border: none; border-radius: 6px; background: #2d6cdf; color: white; font-size: 1em; }
  button:disabled { background: #444; }
  #log { font-size: 0.8em; color: #888; white-space: pre-wrap; margin-top: 12px; }
</style>
</head>
<body>
<h1>AM32 ESC &mdash; WiFi Link</h1>
<div id="statusBox" class="status">Not connected</div>
<button id="btnConnect">Connect to ESC</button>
<button id="btnRead" disabled>Read Settings</button>
<button id="btnWrite" disabled>Write Settings</button>
<div id="form"></div>
<div id="log"></div>

<script>
let fields = [];

function log(msg) {
  const el = document.getElementById('log');
  el.textContent = new Date().toLocaleTimeString() + '  ' + msg + '\n' + el.textContent;
}

async function loadFields() {
  const res = await fetch('/api/fields');
  fields = await res.json();
  const form = document.getElementById('form');
  form.innerHTML = '';
  fields.forEach(f => {
    const row = document.createElement('div');
    row.className = 'field';
    row.innerHTML = `<label>${f.label}<span class="hint">${f.hint}</span></label>
                      <input type="number" id="p_${f.key}" data-key="${f.key}">`;
    form.appendChild(row);
  });
}

async function connect() {
  document.getElementById('statusBox').textContent = 'Connecting...';
  try {
    const res = await fetch('/api/status');
    const j = await res.json();
    const box = document.getElementById('statusBox');
    if (j.ok) {
      box.className = 'status ok';
      box.textContent = `Connected — sig ${j.sig_hi.toString(16)}${j.sig_lo.toString(16)}, boot rev ${j.boot_rev}`;
      document.getElementById('btnRead').disabled = false;
      document.getElementById('btnWrite').disabled = false;
      log('Connected to ESC bootloader');
    } else {
      box.className = 'status bad';
      box.textContent = 'No response from ESC — check wiring / arm sequence';
      log('InitFlash failed');
    }
  } catch (e) {
    log('Connect error: ' + e);
  }
}

async function readSettings() {
  log('Reading EEPROM...');
  const res = await fetch('/api/eeprom');
  const j = await res.json();
  if (!j.ok) { log('Read failed'); return; }
  fields.forEach(f => {
    const el = document.getElementById('p_' + f.key);
    if (el) el.value = j.values[f.key];
  });
  log('Read OK');
}

async function writeSettings() {
  const values = {};
  fields.forEach(f => {
    const el = document.getElementById('p_' + f.key);
    values[f.key] = parseInt(el.value, 10) || 0;
  });
  log('Writing EEPROM...');
  const res = await fetch('/api/eeprom', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({values})
  });
  const j = await res.json();
  log(j.ok ? 'Write OK' : 'Write FAILED');
}

document.getElementById('btnConnect').onclick = connect;
document.getElementById('btnRead').onclick = readSettings;
document.getElementById('btnWrite').onclick = writeSettings;
loadFields();
</script>
</body>
</html>
)HTML";
