#include "web_server_mgr.h"
#include "servo_lift.h"
#include "display.h"
#include "motor_conf.h"
#include "ps3_handler.h"
#include <Preferences.h>

// ============================================================
//  INSTANCES
// ============================================================
static WebServer   server(80);
static Preferences motorPrefs;

// ============================================================
//  MOTOR CONFIG PERSISTENCE
// ============================================================
void loadMotorConfig() {
  motorPrefs.begin("motor_cfg", true);
  for (int i = 0; i < 4; i++) {
    motorConfigs[i].baseSpeed = motorPrefs.getInt(("m"+String(i)+"b").c_str(), 210);
    motorConfigs[i].minSpeed  = motorPrefs.getInt(("m"+String(i)+"n").c_str(), 140);
    motorConfigs[i].maxSpeed  = motorPrefs.getInt(("m"+String(i)+"x").c_str(), 255);
  }
  motorPrefs.end();
}

void saveMotorConfig() {
  motorPrefs.begin("motor_cfg", false);
  for (int i = 0; i < 4; i++) {
    motorPrefs.putInt(("m"+String(i)+"b").c_str(), motorConfigs[i].baseSpeed);
    motorPrefs.putInt(("m"+String(i)+"n").c_str(), motorConfigs[i].minSpeed);
    motorPrefs.putInt(("m"+String(i)+"x").c_str(), motorConfigs[i].maxSpeed);
  }
  motorPrefs.end();
}

// ============================================================
//  HELPER
// ============================================================
static int parseJsonInt(const String& json, const String& key, int def) {
  String k = "\"" + key + "\":";
  int idx = json.indexOf(k);
  return (idx == -1) ? def : json.substring(idx + k.length()).toInt();
}

static String parseJsonStr(const String& json, const String& key, const String& def) {
  String k = "\"" + key + "\":\"";
  int idx = json.indexOf(k);
  if (idx == -1) return def;
  int start = idx + k.length();
  int end   = json.indexOf('"', start);
  return (end == -1) ? def : json.substring(start, end);
}

// ============================================================
//  HTML (PROGMEM) — Dashboard lengkap
// ============================================================
static const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Transporter Config</title>
<style>
  :root{--blue:#007bff;--orange:#ff9800;--green:#4caf50;--red:#e53935;--purple:#9c27b0;--teal:#00897b;--bg:#0d1117;--card:#161b22;--border:#30363d;--text:#e6edf3;--sub:#8b949e}
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Segoe UI',Arial,sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
  header{background:linear-gradient(135deg,#1a1f2e,#0d1117);border-bottom:1px solid var(--border);padding:18px 24px;display:flex;align-items:center;gap:12px}
  header h1{font-size:1.4rem;font-weight:700;letter-spacing:.5px}
  header .badge{background:var(--green);color:#fff;font-size:.7rem;padding:2px 8px;border-radius:20px;font-weight:600}
  .tabs{display:flex;gap:0;border-bottom:1px solid var(--border);background:var(--card);overflow-x:auto}
  .tab{padding:12px 20px;cursor:pointer;font-size:.85rem;color:var(--sub);border-bottom:2px solid transparent;white-space:nowrap;transition:all .2s}
  .tab.active{color:var(--text);border-bottom-color:var(--blue)}
  .tab:hover:not(.active){color:var(--text)}
  .page{display:none;padding:24px;max-width:960px;margin:0 auto}
  .page.active{display:block}
  .section{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:20px;margin-bottom:16px}
  .section-title{font-size:.75rem;font-weight:700;text-transform:uppercase;letter-spacing:1px;margin-bottom:16px;padding-bottom:10px;border-bottom:1px solid var(--border)}
  .grid{display:grid;gap:16px}
  .grid-2{grid-template-columns:repeat(auto-fit,minmax(200px,1fr))}
  .grid-3{grid-template-columns:repeat(auto-fit,minmax(160px,1fr))}
  .grid-4{grid-template-columns:repeat(auto-fit,minmax(130px,1fr))}
  .field label{display:block;font-size:.78rem;color:var(--sub);margin-bottom:6px;font-weight:500}
  .field input[type=number],.field input[type=text],.field select{width:100%;background:#0d1117;border:1px solid var(--border);border-radius:6px;padding:8px 10px;color:var(--text);font-size:.9rem;transition:border .2s}
  .field input:focus,.field select:focus{outline:none;border-color:var(--blue)}
  .field input[type=range]{width:100%;accent-color:var(--blue);cursor:pointer}
  .field .range-val{font-size:.85rem;color:var(--blue);font-weight:700;margin-left:6px}
  .motor-card{background:#0d1117;border:1px solid var(--border);border-radius:8px;padding:16px}
  .motor-card .motor-name{font-size:.8rem;font-weight:700;color:var(--blue);margin-bottom:12px;text-transform:uppercase;letter-spacing:.5px}
  .btn{border:none;border-radius:6px;padding:10px 20px;cursor:pointer;font-size:.85rem;font-weight:600;transition:all .2s;display:inline-flex;align-items:center;gap:6px}
  .btn-primary{background:var(--blue);color:#fff} .btn-primary:hover{background:#0056b3}
  .btn-success{background:var(--green);color:#fff} .btn-success:hover{background:#388e3c}
  .btn-danger{background:var(--red);color:#fff}  .btn-danger:hover{background:#b71c1c}
  .btn-warning{background:var(--orange);color:#fff} .btn-warning:hover{background:#f57c00}
  .btn-purple{background:var(--purple);color:#fff} .btn-purple:hover{background:#6a1b9a}
  .btn-teal{background:var(--teal);color:#fff} .btn-teal:hover{background:#00695c}
  .btn-block{width:100%;justify-content:center;padding:12px}
  .btn-group{display:flex;gap:10px;flex-wrap:wrap}
  .toggle{position:relative;display:inline-block;width:44px;height:24px}
  .toggle input{opacity:0;width:0;height:0}
  .slider-t{position:absolute;cursor:pointer;inset:0;background:#30363d;border-radius:24px;transition:.3s}
  .slider-t:before{content:'';position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
  input:checked+.slider-t{background:var(--green)}
  input:checked+.slider-t:before{transform:translateX(20px)}
  .toggle-row{display:flex;align-items:center;justify-content:space-between;padding:4px 0}
  .toast{position:fixed;bottom:24px;right:24px;padding:12px 20px;border-radius:8px;font-size:.85rem;font-weight:600;z-index:999;opacity:0;transform:translateY(10px);transition:all .3s;pointer-events:none}
  .toast.show{opacity:1;transform:translateY(0)}
  .toast.ok{background:#1b5e20;color:#a5d6a7;border:1px solid #2e7d32}
  .toast.err{background:#b71c1c;color:#ef9a9a;border:1px solid #c62828}
  .mac-input{font-family:monospace;letter-spacing:2px;font-size:1rem}
  .warning-box{background:#1a1200;border:1px solid #f57c00;border-radius:8px;padding:12px 16px;font-size:.82rem;color:#ffcc02;margin-top:12px}
  .logo-preview{width:128px;height:64px;background:#000;border:1px solid var(--border);border-radius:4px;image-rendering:pixelated;display:block;margin:10px auto}
  .upload-area{border:2px dashed var(--border);border-radius:8px;padding:24px;text-align:center;cursor:pointer;transition:border .2s}
  .upload-area:hover{border-color:var(--blue)}
  .upload-area input{display:none}
  .chip{display:inline-block;background:#21262d;border:1px solid var(--border);border-radius:4px;padding:2px 8px;font-size:.75rem;font-family:monospace;color:var(--sub)}
  hr{border:none;border-top:1px solid var(--border);margin:16px 0}
  .status-dot{width:8px;height:8px;border-radius:50%;display:inline-block;margin-right:6px}
  .dot-green{background:var(--green);box-shadow:0 0 6px var(--green)}
  .dot-red{background:var(--red)}
</style>
</head>
<body>

<header>
  <span style="font-size:1.8rem">🤖</span>
  <div>
    <h1>Transporter Control Panel</h1>
    <span style="font-size:.75rem;color:var(--sub)">ESP32 Robot Configuration Dashboard</span>
  </div>
  <span class="badge" id="connBadge">ONLINE</span>
</header>

<div class="tabs">
  <div class="tab active" onclick="showTab('motor')">⚙️ Motor</div>
  <div class="tab" onclick="showTab('servo')">🦾 Servo & Lift</div>
  <div class="tab" onclick="showTab('display')">📺 Display</div>
  <div class="tab" onclick="showTab('ps3')">🎮 PS3</div>
  <div class="tab" onclick="showTab('system')">🔧 System</div>
</div>

<!-- ═══ TAB: MOTOR ═══ -->
<div id="tab-motor" class="page active">
  <div class="section">
    <div class="section-title" style="color:var(--blue)">⚙️ Individual Motor Speed Config</div>
    <div class="grid grid-2" id="motorCards"></div>
  </div>
  <button class="btn btn-primary btn-block" onclick="saveMotor()">💾 Save Motor Config</button>
</div>

<!-- ═══ TAB: SERVO & LIFT ═══ -->
<div id="tab-servo" class="page">
  <div class="section">
    <div class="section-title" style="color:var(--green)">🦾 Gripper Servo</div>
    <div class="grid grid-3">
      <div class="field"><label>Open Angle (°)</label><input type="number" id="openAngle" min="0" max="180" value="50"></div>
      <div class="field"><label>Close Angle (°)</label><input type="number" id="closeAngle" min="0" max="180" value="150"></div>
      <div class="field"><label>Speed (ms/°)</label><input type="number" id="servoSpeed" min="5" max="50" value="15"></div>
    </div>
    <hr>
    <div class="btn-group" style="margin-top:4px">
      <button class="btn btn-success" onclick="servoCtrl('open')">🔓 Open Gripper</button>
      <button class="btn btn-danger"  onclick="servoCtrl('close')">🔒 Close Gripper</button>
    </div>
  </div>
  <div class="section">
    <div class="section-title" style="color:var(--orange)">🏗️ Lift Motor</div>
    <div class="grid grid-2">
      <div class="field">
        <label>Speed Up &nbsp;<span class="range-val" id="liftUpVal">200</span></label>
        <input type="range" id="liftSpeedUp" min="0" max="255" value="200" oninput="document.getElementById('liftUpVal').textContent=this.value">
      </div>
      <div class="field">
        <label>Speed Down &nbsp;<span class="range-val" id="liftDownVal">200</span></label>
        <input type="range" id="liftSpeedDown" min="0" max="255" value="200" oninput="document.getElementById('liftDownVal').textContent=this.value">
      </div>
    </div>
    <hr>
    <div class="btn-group">
      <button class="btn btn-warning" onclick="liftCtrl('up')">⬆️ Test Lift Up</button>
      <button class="btn btn-teal"    onclick="liftCtrl('down')">⬇️ Test Lift Down</button>
      <button class="btn btn-danger"  onclick="liftCtrl('stop')">⏹ Stop Lift</button>
    </div>
  </div>
  <button class="btn btn-primary btn-block" onclick="saveServo()">💾 Save Servo & Lift Config</button>
</div>

<!-- ═══ TAB: DISPLAY ═══ -->
<div id="tab-display" class="page">
  <div class="section">
    <div class="section-title" style="color:var(--purple)">📺 Display Settings</div>
    <div class="grid grid-2">
      <div class="field">
        <label>Brightness &nbsp;<span class="range-val" id="brightVal">255</span></label>
        <input type="range" id="brightness" min="0" max="255" value="255" oninput="document.getElementById('brightVal').textContent=this.value">
      </div>
      <div class="field">
        <label>Contrast &nbsp;<span class="range-val" id="contrastVal">255</span></label>
        <input type="range" id="contrast" min="0" max="255" value="255" oninput="document.getElementById('contrastVal').textContent=this.value">
      </div>
    </div>
    <div class="grid grid-3" style="margin-top:16px">
      <div class="field"><label>Display Mode</label>
        <select id="displayMode"><option value="0">Normal</option><option value="1">Dark</option><option value="2">Minimal</option></select>
      </div>
      <div class="field"><label>Font Size</label>
        <select id="fontSize"><option value="1">Small</option><option value="2">Medium</option><option value="3">Large</option></select>
      </div>
      <div class="field"><label>Update Rate (ms)</label>
        <input type="number" id="updateRate" min="100" max="5000" step="100" value="500">
      </div>
    </div>
  </div>
  <div class="section">
    <div class="section-title" style="color:var(--purple)">🖼️ Custom Logo (128×64 BMP)</div>
    <div class="upload-area" onclick="document.getElementById('bmpFile').click()">
      <input type="file" id="bmpFile" accept=".bmp" onchange="handleBmpUpload(this)">
      <p style="color:var(--sub);font-size:.85rem">Klik untuk pilih file BMP (128×64 px, 1-bit monochrome)</p>
      <p style="color:var(--sub);font-size:.75rem;margin-top:4px">Logo akan disimpan ke flash dan tampil saat boot</p>
    </div>
    <canvas id="logoPreview" class="logo-preview" width="128" height="64" style="display:none"></canvas>
    <button id="btnUploadLogo" class="btn btn-purple" style="display:none;margin-top:10px" onclick="uploadLogo()">📤 Upload Logo</button>
  </div>
  <button class="btn btn-primary btn-block" onclick="saveDisplay()">💾 Save Display Config</button>
</div>

<!-- ═══ TAB: PS3 ═══ -->
<div id="tab-ps3" class="page">
  <div class="section">
    <div class="section-title" style="color:var(--blue)">🎮 PS3 Controller MAC Address</div>
    <p style="font-size:.82rem;color:var(--sub);margin-bottom:16px">
      MAC address digunakan untuk pairing Bluetooth PS3. Setelah save, ESP32 akan restart otomatis.
    </p>
    <div class="field" style="max-width:280px">
      <label>MAC Address Controller</label>
      <input type="text" id="ps3Mac" class="mac-input" placeholder="xx:xx:xx:xx:xx:xx" maxlength="17">
    </div>
    <div class="warning-box">
      ⚠️ <strong>Cara dapat MAC address PS3:</strong><br>
      Gunakan tool <strong>SixaxisPairTool</strong> di PC via USB, atau lihat sticker di balik controller.
      Format: <span class="chip">xx:xx:xx:xx:xx:xx</span> (huruf kecil)
    </div>
    <hr>
    <button class="btn btn-primary" style="margin-top:4px" onclick="saveMac()">💾 Save MAC &amp; Restart ESP32</button>
  </div>
  <div class="section">
    <div class="section-title">📶 Status Koneksi</div>
    <div style="display:flex;align-items:center;gap:8px">
      <span class="status-dot" id="ps3StatusDot"></span>
      <span id="ps3StatusText" style="font-size:.85rem">Memeriksa...</span>
    </div>
  </div>
</div>

<!-- ═══ TAB: SYSTEM ═══ -->
<div id="tab-system" class="page">
  <div class="section">
    <div class="section-title" style="color:var(--red)">🔧 System Actions</div>
    <div class="grid grid-2">
      <div style="background:#1a0000;border:2px solid var(--red);border-radius:8px;padding:16px">
        <div style="font-size:.9rem;font-weight:700;margin-bottom:8px;color:#ff5252">⛔ Emergency Stop</div>
        <p style="font-size:.78rem;color:var(--sub);margin-bottom:12px">Matikan semua motor & lifter seketika via STBY pin. PS3: tekan <strong>L3+R3</strong>.</p>
        <div class="btn-group">
          <button class="btn btn-danger" id="btnEstop" onclick="toggleEstop(true)">⛔ EMERGENCY STOP</button>
          <button class="btn btn-success" id="btnResume" onclick="toggleEstop(false)" style="display:none">✅ RESUME MOTOR</button>
        </div>
      </div>
      <div style="background:#0d1117;border:1px solid var(--border);border-radius:8px;padding:16px">
        <div style="font-size:.85rem;font-weight:600;margin-bottom:8px">🔄 Restart ESP32</div>
        <p style="font-size:.78rem;color:var(--sub);margin-bottom:12px">Restart mikrokontroler. Semua config yang sudah disimpan tetap ada.</p>
        <button class="btn btn-warning" onclick="sysAction('restart')">🔄 Restart Now</button>
      </div>
      <div style="background:#0d1117;border:1px solid var(--border);border-radius:8px;padding:16px">
        <div style="font-size:.85rem;font-weight:600;margin-bottom:8px">⚠️ Reset Semua Config</div>
        <p style="font-size:.78rem;color:var(--sub);margin-bottom:12px">Hapus semua konfigurasi dan kembalikan ke nilai default pabrik.</p>
        <button class="btn btn-danger" onclick="confirmReset()">🗑️ Factory Reset</button>
      </div>
    </div>
  </div>
  <div class="section">
    <div class="section-title">ℹ️ Info Sistem</div>
    <div id="sysInfo" style="font-size:.82rem;color:var(--sub);font-family:monospace;line-height:1.8">Memuat...</div>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
// ─── Motor Cards ─────────────────────────────────────────────
const motorNames = ['Back Right','Back Left','Front Left','Front Right'];
function buildMotorCards(cfg) {
  const c = document.getElementById('motorCards');
  c.innerHTML = motorNames.map((name,i) => `
    <div class="motor-card">
      <div class="motor-name">${name}</div>
      <div class="grid" style="gap:10px">
        <div class="field"><label>Base Speed</label>
          <input type="number" id="m${i}b" min="0" max="255" value="${cfg['motor_'+i+'_base']||210}"></div>
        <div class="field"><label>Min Speed</label>
          <input type="number" id="m${i}n" min="0" max="255" value="${cfg['motor_'+i+'_min']||140}"></div>
        <div class="field"><label>Max Speed</label>
          <input type="number" id="m${i}x" min="0" max="255" value="${cfg['motor_'+i+'_max']||255}"></div>
      </div>
    </div>`).join('');
}

// ─── Tab navigation ──────────────────────────────────────────
function showTab(name) {
  document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
  document.getElementById('tab-'+name).classList.add('active');
  event.target.classList.add('active');
}

// ─── Toast ───────────────────────────────────────────────────
function toast(msg, ok=true) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast show ' + (ok ? 'ok' : 'err');
  setTimeout(()=>t.className='toast',3000);
}

// ─── Load all config ─────────────────────────────────────────
function loadAll() {
  fetch('/api/config').then(r=>r.json()).then(d=>{
    buildMotorCards(d);
    // Servo & Lift
    setVal('openAngle', d.openAngle);
    setVal('closeAngle', d.closeAngle);
    setVal('servoSpeed', d.servoSpeed);
    setRange('liftSpeedUp', 'liftUpVal', d.liftSpeedUp);
    setRange('liftSpeedDown','liftDownVal',d.liftSpeedDown);
    // Display
    setRange('brightness','brightVal',d.brightness);
    setRange('contrast','contrastVal',d.contrast);
    setVal('displayMode',d.displayMode);
    setVal('fontSize',d.fontSize);
    setVal('updateRate',d.updateRate);
    // PS3
    setVal('ps3Mac', d.ps3Mac||'');
  }).catch(()=>toast('Gagal load config',false));
}

function setVal(id,v){const e=document.getElementById(id);if(e&&v!==undefined)e.value=v}
function setRange(id,valId,v){if(v===undefined)return;setVal(id,v);const e=document.getElementById(valId);if(e)e.textContent=v}

// ─── Save handlers ───────────────────────────────────────────
function postJson(url, data) {
  return fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)})
    .then(r=>r.json());
}

function saveMotor() {
  const data = {};
  for(let i=0;i<4;i++){
    data['motor_'+i+'_base'] = +document.getElementById('m'+i+'b').value;
    data['motor_'+i+'_min']  = +document.getElementById('m'+i+'n').value;
    data['motor_'+i+'_max']  = +document.getElementById('m'+i+'x').value;
  }
  postJson('/api/motor', data).then(r=>toast(r.success?'✅ Motor saved!':'❌ '+r.message, r.success));
}

function saveServo() {
  const data = {
    openAngle:    +document.getElementById('openAngle').value,
    closeAngle:   +document.getElementById('closeAngle').value,
    servoSpeed:   +document.getElementById('servoSpeed').value,
    liftSpeedUp:  +document.getElementById('liftSpeedUp').value,
    liftSpeedDown:+document.getElementById('liftSpeedDown').value,
  };
  postJson('/api/servo', data).then(r=>toast(r.success?'✅ Servo & Lift saved!':'❌ '+r.message, r.success));
}

function saveDisplay() {
  const data = {
    brightness:  +document.getElementById('brightness').value,
    contrast:    +document.getElementById('contrast').value,
    displayMode: +document.getElementById('displayMode').value,
    fontSize:    +document.getElementById('fontSize').value,
    updateRate:  +document.getElementById('updateRate').value,
  };
  postJson('/api/display', data).then(r=>toast(r.success?'✅ Display saved!':'❌ '+r.message, r.success));
}

function saveMac() {
  const mac = document.getElementById('ps3Mac').value.trim().toLowerCase();
  const re  = /^([0-9a-f]{2}:){5}[0-9a-f]{2}$/;
  if (!re.test(mac)) { toast('❌ Format MAC tidak valid! Contoh: 2c:81:58:2f:26:a9', false); return; }
  if (!confirm('ESP32 akan restart setelah save MAC. Lanjutkan?')) return;
  postJson('/api/ps3mac', {mac}).then(r=>toast(r.success?'✅ MAC saved, restarting...':'❌ '+r.message, r.success));
}

// ─── Servo & Lift controls ───────────────────────────────────
function servoCtrl(cmd) {
  fetch('/api/servo/'+cmd,{method:'POST'}).then(r=>r.json()).then(r=>toast(r.message,r.success));
}
function liftCtrl(cmd) {
  fetch('/api/lift/'+cmd,{method:'POST'}).then(r=>r.json()).then(r=>toast(r.message,r.success));
}
// ─── System actions ──────────────────────────────────────────
function sysAction(action) {
  if(action==='restart' && !confirm('Restart ESP32 sekarang?')) return;
  fetch('/api/system/'+action,{method:'POST'}).then(r=>r.json())
    .then(r=>toast(r.message,r.success)).catch(()=>toast('ESP32 restarting...',true));
}

function confirmReset() {
  if(!confirm('⚠️ SEMUA konfigurasi akan direset ke default!\nTindakan ini tidak bisa dibatalkan.\n\nLanjutkan?')) return;
  sysAction('reset');
}

// ─── BMP Logo upload ─────────────────────────────────────────
let bmpBase64 = null;

function handleBmpUpload(input) {
  const file = input.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = function(e) {
    const bytes = new Uint8Array(e.target.result);
    // Validasi BMP header
    if (bytes[0]!==0x42||bytes[1]!==0x4D) { toast('❌ Bukan file BMP!',false); return; }
    // Tampilkan preview via canvas
    const canvas = document.getElementById('logoPreview');
    const ctx = canvas.getContext('2d');
    const img = new Image();
    img.onload = function() {
      ctx.clearRect(0,0,128,64);
      ctx.drawImage(img,0,0,128,64);
      canvas.style.display='block';
      document.getElementById('btnUploadLogo').style.display='inline-flex';
    };
    img.src = e.target.result;
    // Simpan base64 untuk upload
    bmpBase64 = btoa(String.fromCharCode(...bytes));
  };
  reader.readAsArrayBuffer(file);
}

function uploadLogo() {
  if (!bmpBase64) return;
  postJson('/api/display/logo', {bmp: bmpBase64})
    .then(r=>toast(r.success?'✅ Logo uploaded! Restart untuk melihat hasilnya.':'❌ '+r.message, r.success));
}

// ─── System info & PS3 status ────────────────────────────────
function loadSysInfo() {
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('sysInfo').innerHTML =
      `Free Heap : ${d.freeHeap} bytes<br>` +
      `Uptime    : ${Math.floor(d.uptime/1000)}s<br>` +
      `WiFi SSID : ${d.ssid}<br>` +
      `IP Address: ${d.ip}<br>` +
      `PS3 MAC   : ${d.ps3Mac}`;
    const dot  = document.getElementById('ps3StatusDot');
    const txt  = document.getElementById('ps3StatusText');
    if (d.ps3Connected) {
      dot.className='status-dot dot-green';
      txt.textContent='PS3 Terhubung ✅';
    } else {
      dot.className='status-dot dot-red';
      txt.textContent='PS3 Tidak Terhubung';
    }
  }).catch(()=>{
    document.getElementById('sysInfo').textContent='Gagal memuat info sistem';
  });
}

// ─── Init ────────────────────────────────────────────────────
loadAll();
loadSysInfo();
setInterval(loadSysInfo, 5000);
</script>
</body>
</html>
)rawliteral";

// ============================================================
//  HANDLERS
// ============================================================
static void handleRoot()       { server.send_P(200,"text/html",HTML); }

static void handleConfigGet() {
  String j = "{";
  for (int i=0;i<4;i++){
    if(i>0)j+=",";
    j+="\"motor_"+String(i)+"_base\":"+motorConfigs[i].baseSpeed;
    j+=",\"motor_"+String(i)+"_min\":"+motorConfigs[i].minSpeed;
    j+=",\"motor_"+String(i)+"_max\":"+motorConfigs[i].maxSpeed;
  }
  j+=",\"openAngle\":"    +String(servoConfig.openAngle);
  j+=",\"closeAngle\":"   +String(servoConfig.closeAngle);
  j+=",\"servoSpeed\":"   +String(servoConfig.servoSpeed);
  j+=",\"liftSpeedUp\":"  +String(liftConfig.speedUp);
  j+=",\"liftSpeedDown\":"+String(liftConfig.speedDown);
  j+=",\"brightness\":"   +String(displayConfig.brightness);
  j+=",\"contrast\":"     +String(displayConfig.contrast);
  j+=",\"displayMode\":"  +String(displayConfig.displayMode);
  j+=",\"fontSize\":"     +String(displayConfig.fontSize);
  j+=",\"updateRate\":"   +String(displayConfig.updateRate);
  j+=",\"ps3Mac\":\""     +String(ps3Config.macAddress)+"\"";
  j+="}";
  server.send(200,"application/json",j);
}

static void handleMotorPost() {
  if(!server.hasArg("plain")){server.send(400,"application/json","{\"success\":false,\"message\":\"No data\"}");return;}
  const String& j=server.arg("plain");
  for(int i=0;i<4;i++){
    motorConfigs[i].baseSpeed=parseJsonInt(j,"motor_"+String(i)+"_base",motorConfigs[i].baseSpeed);
    motorConfigs[i].minSpeed =parseJsonInt(j,"motor_"+String(i)+"_min", motorConfigs[i].minSpeed);
    motorConfigs[i].maxSpeed =parseJsonInt(j,"motor_"+String(i)+"_max", motorConfigs[i].maxSpeed);
  }
  saveMotorConfig();
  server.send(200,"application/json","{\"success\":true}");
}

static void handleServoPost() {
  if(!server.hasArg("plain")){server.send(400,"application/json","{\"success\":false}");return;}
  const String& j=server.arg("plain");
  servoConfig.openAngle  =parseJsonInt(j,"openAngle",  servoConfig.openAngle);
  servoConfig.closeAngle =parseJsonInt(j,"closeAngle", servoConfig.closeAngle);
  servoConfig.servoSpeed =parseJsonInt(j,"servoSpeed", servoConfig.servoSpeed);
  liftConfig.speedUp     =parseJsonInt(j,"liftSpeedUp",   liftConfig.speedUp);
  liftConfig.speedDown   =parseJsonInt(j,"liftSpeedDown", liftConfig.speedDown);
  saveServoConfig();
  server.send(200,"application/json","{\"success\":true}");
}

static void handleDisplayPost() {
  if(!server.hasArg("plain")){server.send(400,"application/json","{\"success\":false}");return;}
  const String& j=server.arg("plain");
  displayConfig.brightness   =parseJsonInt(j,"brightness",  displayConfig.brightness);
  displayConfig.contrast     =parseJsonInt(j,"contrast",    displayConfig.contrast);
  displayConfig.displayMode  =parseJsonInt(j,"displayMode", displayConfig.displayMode);
  displayConfig.fontSize     =parseJsonInt(j,"fontSize",    displayConfig.fontSize);
  displayConfig.updateRate   =parseJsonInt(j,"updateRate",  displayConfig.updateRate);
  saveDisplayConfig();
  server.send(200,"application/json","{\"success\":true}");
}

static void handleLogoPost() {
  if(!server.hasArg("plain")){server.send(400,"application/json","{\"success\":false,\"message\":\"No data\"}");return;}
  // Decode base64 BMP dan simpan ke Preferences
  bool ok = saveBitmapToPrefs(server.arg("plain"));
  if(ok){
    displayConfig.customLogo = true;
    saveDisplayConfig();
    server.send(200,"application/json","{\"success\":true}");
  } else {
    server.send(200,"application/json","{\"success\":false,\"message\":\"Invalid BMP or too large\"}");
  }
}

static void handlePs3MacPost() {
  if(!server.hasArg("plain")){server.send(400,"application/json","{\"success\":false}");return;}
  String mac = parseJsonStr(server.arg("plain"),"mac",ps3Config.macAddress);
  mac.toCharArray(ps3Config.macAddress, sizeof(ps3Config.macAddress));
  savePs3Config();
  server.send(200,"application/json","{\"success\":true}");
  pendingRestart = true;  // restart di-handle oleh loop()
}

static void handleServoOpen()  { servoOpen();  server.send(200,"application/json","{\"success\":true,\"message\":\"Servo opened\"}"); }
static void handleServoClose() { servoClose(); server.send(200,"application/json","{\"success\":true,\"message\":\"Servo closed\"}"); }

static void handleLiftUp()   { liftUp(liftConfig.speedUp);     server.send(200,"application/json","{\"success\":true,\"message\":\"Lift UP\"}"); }
static void handleLiftDown() { liftDown(liftConfig.speedDown); server.send(200,"application/json","{\"success\":true,\"message\":\"Lift DOWN\"}"); }
static void handleLiftStop() { liftStop();                     server.send(200,"application/json","{\"success\":true,\"message\":\"Lift STOP\"}"); }


static void handleEStop() {
  extern volatile bool eStopActive;
  extern void triggerEStop();
  // Panggil langsung fungsi E-Stop
  eStopActive = true;
  motorStandby(false);
  liftStop();
  showEStop(true);
  Serial.println("[WEB] E-Stop triggered via WebServer");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"⛔ Emergency Stop aktif!\"}");
}

static void handleEStopResume() {
  extern volatile bool eStopActive;
  eStopActive = false;
  motorStandby(true);
  showEStop(false);
  Serial.println("[WEB] E-Stop resumed via WebServer");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"✅ Motor aktif kembali!\"}");
}

static void handleSystemRestart() {
  server.send(200,"application/json","{\"success\":true,\"message\":\"Restarting...\"}");
  pendingRestart = true;
}

static void handleSystemReset() {
  // Hapus semua Preferences namespace
  Preferences p;
  p.begin("motor_cfg",false); p.clear(); p.end();
  p.begin("servo_cfg",false); p.clear(); p.end();
  p.begin("disp_cfg", false); p.clear(); p.end();
  p.begin("ps3_cfg",  false); p.clear(); p.end();
  p.begin("logo_cfg", false); p.clear(); p.end();
  server.send(200,"application/json","{\"success\":true,\"message\":\"Reset done. Restarting...\"}");
  pendingRestart = true;
}

static void handleStatus() {
  String j = "{";
  j += "\"freeHeap\":"    + String(ESP.getFreeHeap());
  j += ",\"uptime\":"     + String(millis());
  j += ",\"ssid\":\""     + String(WIFI_SSID) + "\"";
  j += ",\"ip\":\""       + WiFi.softAPIP().toString() + "\"";
  j += ",\"ps3Mac\":\""   + String(ps3Config.macAddress) + "\"";
  j += ",\"ps3Connected\":"+ String(Ps3.isConnected()?1:0);
  j += "}";
  server.send(200,"application/json",j);
}

// ============================================================
//  SETUP & LOOP
// ============================================================
void webServerSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] AP IP: "); Serial.println(WiFi.softAPIP());

  server.on("/",                    handleRoot);
  server.on("/api/config",  HTTP_GET,  handleConfigGet);
  server.on("/api/motor",   HTTP_POST, handleMotorPost);
  server.on("/api/servo",   HTTP_POST, handleServoPost);
  server.on("/api/display", HTTP_POST, handleDisplayPost);
  server.on("/api/display/logo", HTTP_POST, handleLogoPost);
  server.on("/api/ps3mac",  HTTP_POST, handlePs3MacPost);
  server.on("/api/servo/open",  HTTP_POST, handleServoOpen);
  server.on("/api/servo/close", HTTP_POST, handleServoClose);
  server.on("/api/lift/up",   HTTP_POST, handleLiftUp);
  server.on("/api/lift/down", HTTP_POST, handleLiftDown);
  server.on("/api/lift/stop", HTTP_POST, handleLiftStop);
  server.on("/api/system/estop",   HTTP_POST, handleEStop);
  server.on("/api/system/resume",  HTTP_POST, handleEStopResume);
  server.on("/api/system/restart", HTTP_POST, handleSystemRestart);
  server.on("/api/system/reset",   HTTP_POST, handleSystemReset);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.begin();
  Serial.println("[WiFi] Web Server started");
}

void webServerHandle() { server.handleClient(); }