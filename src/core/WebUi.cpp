#include "WebUi.h"

#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <Update.h>
#include <WiFi.h>

#include "board_pins.h"
#include "PowerManager.h"
#include "ThemeManager.h"
#include "AutomationEngine.h"

namespace {
constexpr char kApPassword[] = "control";
constexpr char kMutationHeader[] = "X-ControlOS";
constexpr char kMutationValue[] = "1";
constexpr char kSessionCookie[] = "CONTROLSESSION";
constexpr char kLoginPath[] = "/login";
constexpr char kLogoutPath[] = "/logout";
const char* kHeaderKeys[] = {kMutationHeader, "Cookie"};


const char kLoginHtml[] PROGMEM = R"LOGIN(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#050806">
<title>ControlOS Login</title>
<style>
:root{--bg:#050806;--panel:#0a110d;--line:#173d27;--green:#00ff72;--muted:#7f9b88;--text:#e8fff0;--danger:#ff6474}
*{box-sizing:border-box}
html,body{margin:0;min-height:100%;background:radial-gradient(circle at top,#0c1e13,#050806 45%,#020403);color:var(--text);font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace}
body{display:grid;place-items:center;padding:18px}
.card{width:min(420px,100%);border:1px solid var(--line);background:rgba(8,16,11,.96);border-radius:18px;padding:22px;box-shadow:0 0 40px rgba(0,255,114,.08)}
.brand{color:var(--green);font-weight:900;letter-spacing:.12em;font-size:28px}
.sub{margin:7px 0 22px;color:var(--muted);font-size:12px}
label{display:block;color:var(--muted);font-size:11px;margin-bottom:7px}
input{width:100%;border:1px solid var(--line);background:#030704;color:var(--text);border-radius:11px;padding:13px;font:inherit;outline:none}
input:focus{border-color:var(--green)}
button{width:100%;margin-top:12px;border:0;border-radius:11px;background:var(--green);color:#00190a;padding:13px;font:inherit;font-weight:900;cursor:pointer}
.note{margin-top:15px;color:var(--muted);font-size:11px;line-height:1.5}
</style>
</head>
<body>
<form class="card" method="post" action="/login">
<div class="brand">CONTROL//OS</div>
<div class="sub">T-Embed CC1101 Plus · Secure Local Control</div>
<label for="password">ControlOS password</label>
<input id="password" name="password" type="password" autocomplete="current-password" autofocus>
<button type="submit">OPEN CONTROL PANEL</button>
<div class="note">Wi-Fi: <b>ControlOS</b><br>Address: <b>http://172.0.0.1</b></div>
</form>
</body>
</html>
)LOGIN";

const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover" />
  <meta name="theme-color" content="#050806" />
  <title>ControlOS Web Control</title>
  <style>
    :root{--bg:#050806;--panel:#0a110d;--panel2:#0d1711;--line:#173d27;--green:#00ff72;--green2:#77ffad;--muted:#7f9b88;--text:#e8fff0;--danger:#ff5d6c;--warn:#ffd166}
    *{box-sizing:border-box}html,body{margin:0;background:radial-gradient(circle at top,#0c1e13 0,#050806 38%,#020403 100%);color:var(--text);font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,"Liberation Mono",monospace;min-height:100%}
    body:before{content:"";position:fixed;inset:0;pointer-events:none;background:linear-gradient(rgba(0,255,114,.025) 1px,transparent 1px);background-size:100% 4px;mix-blend-mode:screen}
    .shell{max-width:1180px;margin:0 auto;padding:18px}.top{display:flex;gap:12px;justify-content:space-between;align-items:center;margin-bottom:14px}.brand{font-weight:900;letter-spacing:.14em;color:var(--green);font-size:clamp(20px,4vw,34px)}.sub{color:var(--muted);font-size:12px;margin-top:3px}.live{border:1px solid var(--line);background:rgba(0,255,114,.06);border-radius:999px;padding:8px 12px;color:var(--green2);font-size:12px;white-space:nowrap}
    .tabs{display:flex;gap:8px;overflow:auto;padding:4px 0 14px}.tab{border:1px solid var(--line);background:#071009;color:var(--muted);padding:10px 13px;border-radius:10px;cursor:pointer;font:inherit}.tab.active{background:var(--green);color:#001b0b;border-color:var(--green);font-weight:800}
    .view{display:none}.view.active{display:block}.grid{display:grid;grid-template-columns:repeat(12,1fr);gap:12px}.card{grid-column:span 3;border:1px solid var(--line);background:linear-gradient(180deg,rgba(13,23,17,.94),rgba(6,12,8,.94));border-radius:14px;padding:14px;box-shadow:0 0 0 1px rgba(0,255,114,.02) inset}.card.w6{grid-column:span 6}.card.w8{grid-column:span 8}.card.w12{grid-column:span 12}.label{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:.12em}.value{font-size:22px;color:var(--green2);margin-top:7px;word-break:break-word}.small{font-size:12px;color:var(--muted);line-height:1.5}
    h2{margin:0 0 12px;font-size:15px;color:var(--green2);letter-spacing:.08em}.buttons{display:flex;flex-wrap:wrap;gap:8px}.btn{appearance:none;border:1px solid var(--line);background:#0a140e;color:var(--text);padding:10px 12px;border-radius:10px;font:inherit;cursor:pointer}.btn:hover{border-color:var(--green);color:var(--green2)}.btn.primary{background:var(--green);border-color:var(--green);color:#001909;font-weight:900}.btn.danger{border-color:#6f2831;color:#ff9ba5}.btn.warn{border-color:#705d1a;color:var(--warn)}
    .remote{display:grid;grid-template-columns:82px 82px 82px;grid-template-rows:58px 58px 58px;gap:9px;justify-content:center}.remote .btn{font-size:20px}.remote .up{grid-column:2}.remote .left{grid-row:2;grid-column:1}.remote .ok{grid-row:2;grid-column:2}.remote .right{grid-row:2;grid-column:3}.remote .back{grid-row:3;grid-column:1}.remote .home{grid-row:3;grid-column:2 / span 2}
    .apps{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:8px}.app{padding:12px;text-align:left}.app b{color:var(--green2);display:block;margin-bottom:4px}.app span{font-size:11px;color:var(--muted)}
    .toolbar{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-bottom:10px}select,input{background:#08110b;border:1px solid var(--line);color:var(--text);border-radius:9px;padding:10px;font:inherit}input[type=text]{min-width:180px;flex:1}.path{color:var(--green2);word-break:break-all;font-size:12px;margin:8px 0 12px}
    table{width:100%;border-collapse:collapse;font-size:12px}th,td{text-align:left;padding:10px 8px;border-bottom:1px solid #102719}th{color:var(--muted);font-size:10px;text-transform:uppercase;letter-spacing:.09em}.nameBtn{background:none;border:0;color:var(--text);font:inherit;cursor:pointer;padding:0}.nameBtn.dir{color:var(--green2)}.actions{display:flex;gap:6px;justify-content:flex-end}.actions .btn{padding:6px 8px;font-size:11px}
    .drop{border:1px dashed #2d7248;border-radius:12px;padding:18px;text-align:center;color:var(--muted);transition:.2s}.drop.drag{border-color:var(--green);background:rgba(0,255,114,.07);color:var(--green2)}progress{width:100%;height:10px;margin-top:10px}.preview{width:100%;min-height:260px;resize:vertical;white-space:pre-wrap;max-height:340px;overflow:auto;background:#030604;border:1px solid #132b1c;border-radius:10px;padding:12px;font-size:11px;line-height:1.45;color:#c9ffda}
    .log{font-size:11px;line-height:1.55;color:#b4cdbb;white-space:pre-wrap;max-height:300px;overflow:auto}.pill{display:inline-block;border:1px solid var(--line);border-radius:999px;padding:3px 7px;color:var(--muted);font-size:10px}.ok{color:var(--green2)}.bad{color:var(--danger)}
    .toast{position:fixed;right:18px;bottom:18px;max-width:360px;background:#09120c;border:1px solid var(--line);padding:11px 13px;border-radius:10px;display:none;z-index:10}.toast.show{display:block}.footer{padding:18px 0 8px;color:#557160;font-size:10px;text-align:center}
    @media(max-width:760px){.card,.card.w6,.card.w8{grid-column:span 12}.shell{padding:12px}.remote{grid-template-columns:72px 72px 72px}.top{align-items:flex-start}.live{font-size:10px}table{font-size:11px}th:nth-child(2),td:nth-child(2){display:none}}
  </style>
</head>
<body>
<div class="shell">
  <div class="top"><div><div class="brand">CONTROL//OS</div><div class="sub">T-Embed CC1101 Plus · Dedicated AP Control Plane</div></div><div style="display:flex;gap:8px;align-items:center"><div class="live" id="live">CONNECTING...</div><a class="btn" style="text-decoration:none;padding:8px 10px" href="/logout">Logout</a></div></div>
  <div class="tabs">
    <button class="tab active" data-tab="dash">Dashboard</button>
    <button class="tab" data-tab="remote">Remote</button>
    <button class="tab" data-tab="led">LED</button>
    <button class="tab" data-tab="files">Files</button>
    <button class="tab" data-tab="system">System</button>
    <button class="tab" data-tab="settings">Settings</button>
  </div>

  <section id="dash" class="view active">
    <div class="grid">
      <div class="card"><div class="label">Active</div><div class="value" id="active">-</div></div>
      <div class="card"><div class="label">Uptime</div><div class="value" id="uptime">-</div></div>
      <div class="card"><div class="label">Free Heap</div><div class="value" id="heap">-</div></div>
      <div class="card"><div class="label">Clients</div><div class="value" id="clients">-</div></div>
      <div class="card"><div class="label">Battery</div><div class="value" id="battery">-</div></div>
      <div class="card"><div class="label">Power</div><div class="value" id="powerProfile">-</div></div>
      <div class="card w6"><h2>Storage</h2><div id="storage" class="small">Loading...</div></div>
      <div class="card w6"><h2>Network</h2><div id="network" class="small">Loading...</div></div>
      <div class="card w12"><h2>Launch Apps</h2><div class="apps" id="apps"></div></div>
    </div>
  </section>

  <section id="remote" class="view">
    <div class="grid">
      <div class="card w6"><h2>Hardware Remote</h2><div class="remote">
        <button class="btn up" onclick="control('select')">OK</button>
        <button class="btn left" onclick="control('left')">◀</button>
        <button class="btn ok" onclick="control('select')">●</button>
        <button class="btn right" onclick="control('right')">▶</button>
        <button class="btn back" onclick="control('back')">BACK</button>
        <button class="btn home" onclick="control('home')">HOME</button>
      </div><div class="small" style="margin-top:14px">Remote actions are injected into the same event path as the physical encoder and USER button.</div></div>
      <div class="card w6"><h2>Device Actions</h2><div class="buttons">
        <button class="btn" onclick="control('screen_on')">Display On</button>
        <button class="btn" onclick="control('screen_off')">Display Off</button>
        <button class="btn danger" onclick="confirmAction('reboot','Reboot device?')">Reboot</button>
      </div><div class="small" style="margin-top:14px">ControlOS runs its own isolated access point at 172.0.0.1 while the WebUI is active.</div></div>
      <div class="card w12"><h2>Direct App Launcher</h2><div class="apps" id="appsRemote"></div></div>
    </div>
  </section>


  <section id="led" class="view">
    <div class="grid">
      <div class="card w6">
        <h2>LED Studio</h2>
        <div class="toolbar"><label class="small">Power</label><select id="ledEnabled"><option value="1">On</option><option value="0">Off</option></select></div>
        <div class="toolbar"><label class="small">Effect</label><select id="ledEffect"></select></div>
        <div class="toolbar"><label class="small">Color</label><input id="ledColor" type="color" value="#00ff50"/><span class="pill" id="ledHex">#00FF50</span></div>
        <div class="small">Brightness <b id="ledBrightnessValue">-</b></div>
        <input id="ledBrightness" type="range" min="0" max="255" value="72" style="width:100%"/>
        <div class="small" style="margin-top:12px">Speed <b id="ledSpeedValue">-</b></div>
        <input id="ledSpeed" type="range" min="1" max="100" value="45" style="width:100%"/>
        <div class="buttons" style="margin-top:14px"><button class="btn primary" onclick="saveLed()">Apply</button><button class="btn" onclick="loadLed()">Reload</button></div>
      </div>
      <div class="card w6">
        <h2>Presets</h2>
        <div class="buttons">
          <button class="btn" onclick="ledPreset('#00ff50',1,70,35)">Control Green</button>
          <button class="btn" onclick="ledPreset('#00b7ff',2,90,42)">Ice Breathe</button>
          <button class="btn" onclick="ledPreset('#ff00c8',5,100,62)">Magenta Scanner</button>
          <button class="btn" onclick="ledPreset('#ff6a00',7,95,55)">Orange Comet</button>
          <button class="btn" onclick="ledPreset('#00ff50',3,90,55)">Rainbow</button>
          <button class="btn" onclick="ledPreset('#00ff50',15,88,72)">Cyber Pulse</button>
          <button class="btn" onclick="ledPreset('#00ff41',16,92,58)">Matrix</button>
          <button class="btn" onclick="ledPreset('#ff5a00',17,105,48)">Fire</button>
          <button class="btn" onclick="ledPreset('#00aaff',18,95,42)">Ocean</button>
          <button class="btn" onclick="ledPreset('#7cffb8',22,90,38)">Aurora</button>
          <button class="btn danger" onclick="ledPreset('#000000',0,0,45)">LED Off</button>
        </div>
        <div id="ledPreview" style="height:64px;border-radius:12px;border:1px solid var(--line);margin-top:16px;background:#00ff50;box-shadow:0 0 28px rgba(0,255,114,.22)"></div>
        <div class="small" style="margin-top:12px">Settings are stored in ESP32 NVS after a short debounce, so slider changes do not continuously write flash.</div>
      </div>
      <div class="card w12"><h2>Effects</h2><div class="small">Solid · Breathe · Rainbow · Chase · Scanner · Sparkle · Comet · Color Wipe · Theater Chase · Twinkle · Meteor · Wave · Dual Scanner · Heartbeat · Cyber Pulse · Matrix · Fire · Ocean · Confetti · Glitch · Random Color · Aurora. Speed controls the animation frame rate. Matrix, Fire, Ocean, Confetti, Random Color and Aurora use their own dynamic palettes.</div></div>
    </div>
  </section>

  <section id="files" class="view">
    <div class="grid">
      <div class="card w12">
        <h2>File Manager</h2>
        <div class="toolbar">
          <select id="fs"><option value="flash">Internal Flash</option><option value="sd">microSD</option></select>
          <button class="btn" onclick="goUp()">Up</button>
          <button class="btn" onclick="refreshFiles()">Refresh</button>
          <input id="folderName" type="text" placeholder="new folder name" />
          <button class="btn" onclick="mkdir()">Create Folder</button>
          <button class="btn" onclick="newFile()">New File</button>
        </div>
        <div class="path" id="path">/</div>
        <div class="drop" id="drop">Drop files here or <input id="fileInput" type="file" multiple /></div>
        <progress id="uploadProgress" max="100" value="0"></progress>
        <div style="overflow:auto;margin-top:10px"><table><thead><tr><th>Name</th><th>Size</th><th style="text-align:right">Actions</th></tr></thead><tbody id="fileRows"></tbody></table></div>
      </div>
      <div class="card w12"><h2>Text Editor</h2><div class="toolbar"><span class="small" id="editLabel">No file selected</span><button class="btn primary" onclick="savePreview()">Save</button></div><textarea class="preview" id="preview" spellcheck="false">Select a text file and press Preview.</textarea></div>
    </div>
  </section>

  <section id="system" class="view">
    <div class="grid">
      <div class="card w6"><h2>Runtime</h2><div class="small" id="runtime">Loading...</div></div>
      <div class="card w6"><h2>WebUI Security</h2><div class="small">HTTP Basic authentication is enabled. Mutating API calls also require an <span class="pill">X-ControlOS: 1</span> request header. The AP uses WPA2 with the configured AP password.</div></div>
      <div class="card w6"><h2>Firmware Update (OTA)</h2><div class="small">Upload a compiled application <code>.bin</code>. ControlOS validates the update stream and reboots after a successful write.</div><div class="toolbar" style="margin-top:10px"><input id="otaFile" type="file" accept=".bin"/><button class="btn warn" onclick="otaUpload()">Install Firmware</button></div><progress id="otaProgress" max="100" value="0"></progress></div>
      <div class="card w6"><h2>Recent Web Events</h2><div id="logs" class="log">Loading...</div></div>
      <div class="card w12"><h2>Configuration Maintenance</h2><div class="buttons"><button class="btn" onclick="exportConfig()">Export Config</button><label class="btn">Import Config<input id="configFile" type="file" accept=".json,application/json" style="display:none" onchange="importConfig(this.files[0])"/></label><button class="btn danger" onclick="factoryReset()">Factory Reset</button></div><div class="small" style="margin-top:10px">Backup/restore theme, power, LED and automation settings. Factory reset clears ControlOS NVS namespaces and reboots.</div></div>
      <div class="card w12"><h2>API</h2><div class="small">GET <code>/api/status</code>, GET <code>/api/apps</code>, POST <code>/api/control?action=...</code>, GET/POST <code>/api/led</code>, GET/POST <code>/api/theme</code>, GET/POST <code>/api/power</code>, GET/POST <code>/api/automation</code>, GET <code>/api/screen</code>, GET <code>/api/files?fs=flash&amp;path=/</code>, POST <code>/api/upload</code>, GET <code>/api/download</code>, POST <code>/api/ota</code>, GET <code>/api/config/export</code>, POST <code>/api/config/import</code>, POST <code>/api/factory-reset</code>.</div></div>
    </div>
  </section>
  <section id="settings" class="view">
    <div class="grid">
      <div class="card w6"><h2>Theme Engine</h2><div class="toolbar"><select id="themeSelect"></select><button class="btn primary" onclick="saveTheme()">Apply Theme</button></div><div class="small">Theme changes affect the T-Embed UI immediately and persist in NVS.</div></div>
      <div class="card w6"><h2>Power Profile</h2><div class="toolbar"><select id="powerSelect"></select><button class="btn primary" onclick="savePower()">Apply Profile</button></div><div id="batteryDetail" class="small">Loading...</div></div>
      <div class="card w6"><h2>Automation Engine</h2><div class="toolbar"><label class="small">Low battery rule</label><select id="autoLow"><option value="1">On</option><option value="0">Off</option></select></div><div class="toolbar"><label class="small">Threshold</label><input id="autoThreshold" type="number" min="5" max="50" value="15" /></div><div class="toolbar"><label class="small">Web-client LED rule</label><select id="autoWeb"><option value="1">On</option><option value="0">Off</option></select></div><button class="btn primary" onclick="saveAutomation()">Save Automations</button></div>
      <div class="card w6"><h2>Live Telemetry</h2><div id="wsState" class="small">WebSocket connecting...</div><div class="small" style="margin-top:8px">Read-only telemetry streams over <code>ws://172.0.0.1:81</code>. Remote mutations continue to require authenticated HTTP + mutation guard.</div></div>
    </div>
  </section>
  <div class="footer">ControlOS local control plane · 172.0.0.1</div>
</div>
<div class="toast" id="toast"></div>
<script>
const state={path:'/',fs:'flash',editPath:''};
const mut={'X-ControlOS':'1'};
const $=id=>document.getElementById(id);
function toast(msg,bad=false){const t=$('toast');t.textContent=msg;t.style.borderColor=bad?'#6f2831':'#173d27';t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2600)}
async function req(url,opt={}){const r=await fetch(url,opt);const text=await r.text();let data;try{data=JSON.parse(text)}catch{data=text}if(!r.ok)throw new Error((data&&data.error)||text||('HTTP '+r.status));return data}
function fmt(sec){sec=Number(sec||0);const d=Math.floor(sec/86400),h=Math.floor(sec%86400/3600),m=Math.floor(sec%3600/60),s=Math.floor(sec%60);return (d?d+'d ':'')+String(h).padStart(2,'0')+':'+String(m).padStart(2,'0')+':'+String(s).padStart(2,'0')}
function applyStatus(s){$('live').textContent='ONLINE · '+s.ip;$('active').textContent=s.active;$('uptime').textContent=fmt(s.uptime);$('heap').textContent=s.heapFreeText;$('clients').textContent=s.clients;if($('battery'))$('battery').textContent=s.battery&&s.battery.present?s.battery.soc+'%':'N/A';if($('powerProfile'))$('powerProfile').textContent=s.powerProfile||'-';$('storage').innerHTML=`Flash: <b class="${s.flash.ready?'ok':'bad'}">${s.flash.ready?'ready':'offline'}</b> · ${s.flash.usedText} / ${s.flash.totalText}<br>SD: <b class="${s.sd.ready?'ok':'bad'}">${s.sd.ready?'ready':'not mounted'}</b> · ${s.sd.usedText} / ${s.sd.totalText}`;$('network').innerHTML=`SSID: <b>${esc(s.ssid)}</b><br>IP: <b>${esc(s.ip)}</b><br>Mode: AP+STA · Clients: ${s.clients}`;$('runtime').innerHTML=`Chip: ESP32-S3<br>Flash: ${s.flashChipText}<br>Free heap: ${s.heapFreeText}<br>Min heap: ${s.heapMinText}<br>Free PSRAM: ${s.psramFreeText}<br>Theme: ${esc(s.theme||'-')}<br>Power: ${esc(s.powerProfile||'-')}<br>Selected launcher index: ${s.selected}`;}
async function loadStatus(){try{applyStatus(await req('/api/status'))}catch(e){$('live').textContent='DISCONNECTED';}}
function esc(v){return String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
async function loadApps(){try{const d=await req('/api/apps');const html=d.apps.map(a=>`<button class="btn app" onclick="openApp(${a.index})"><b>${esc(a.short)}</b><span>${esc(a.name)}</span></button>`).join('');$('apps').innerHTML=html;$('appsRemote').innerHTML=html}catch(e){toast(e.message,true)}}
async function control(action){try{await req('/api/control?action='+encodeURIComponent(action),{method:'POST',headers:mut});toast('Action: '+action);setTimeout(loadStatus,120)}catch(e){toast(e.message,true)}}
async function openApp(index){try{await req('/api/control?action=open&value='+index,{method:'POST',headers:mut});toast('App opened');setTimeout(loadStatus,120)}catch(e){toast(e.message,true)}}
function confirmAction(action,text){if(confirm(text))control(action)}
function cleanJoin(base,name){return (base==='/'?'':base)+'/'+name}
function baseName(path){const p=path.split('/').filter(Boolean);return p[p.length-1]||'/'}
async function refreshFiles(){state.fs=$('fs').value;try{const d=await req('/api/files?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(state.path));$('path').textContent=state.fs+':'+state.path;$('fileRows').innerHTML=d.items.map(f=>`<tr><td><button class="nameBtn ${f.dir?'dir':''}" onclick="${f.dir?`enterDir('${jsq(f.name)}')`:'void(0)'}">${f.dir?'[DIR] ':''}${esc(f.name)}</button></td><td>${f.dir?'-':esc(f.sizeText)}</td><td><div class="actions">${!f.dir?`<button class="btn" onclick="previewFile('${jsq(f.path)}')">Preview</button><button class="btn" onclick="downloadFile('${jsq(f.path)}')">Download</button>`:''}<button class="btn" onclick="renameItem('${jsq(f.path)}')">Rename</button><button class="btn danger" onclick="deleteItem('${jsq(f.path)}')">Delete</button></div></td></tr>`).join('')||'<tr><td colspan="3" class="small">Folder is empty.</td></tr>';}catch(e){$('fileRows').innerHTML='<tr><td colspan="3" class="bad">'+esc(e.message)+'</td></tr>';toast(e.message,true)}}
function jsq(v){return String(v).replace(/\\/g,'\\\\').replace(/'/g,"\\'")}
function enterDir(name){state.path=cleanJoin(state.path,name);refreshFiles()}
function goUp(){if(state.path==='/')return;const p=state.path.split('/').filter(Boolean);p.pop();state.path='/'+p.join('/');refreshFiles()}
async function mkdir(){const name=$('folderName').value.trim();if(!name)return;try{await req('/api/mkdir?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(cleanJoin(state.path,name)),{method:'POST',headers:mut});$('folderName').value='';refreshFiles();toast('Folder created')}catch(e){toast(e.message,true)}}
async function deleteItem(path){if(!confirm('Delete '+baseName(path)+'?'))return;try{await req('/api/delete?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(path),{method:'POST',headers:mut});refreshFiles();toast('Deleted')}catch(e){toast(e.message,true)}}
async function renameItem(path){const next=prompt('New name',baseName(path));if(!next||next===baseName(path))return;const parts=path.split('/').filter(Boolean);parts.pop();const parent='/'+parts.join('/');const to=cleanJoin(parent==='/'?'/':parent,next);try{await req('/api/rename?fs='+encodeURIComponent(state.fs)+'&from='+encodeURIComponent(path)+'&to='+encodeURIComponent(to),{method:'POST',headers:mut});refreshFiles();toast('Renamed')}catch(e){toast(e.message,true)}}
function downloadFile(path){window.location='/api/download?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(path)}
async function previewFile(path){try{const r=await fetch('/api/read?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(path));if(!r.ok)throw new Error(await r.text());state.editPath=path;$('editLabel').textContent=state.fs+':'+path;$('preview').value=await r.text()}catch(e){toast(e.message,true)}}
async function savePreview(){if(!state.editPath)return toast('No file selected',true);try{await req('/api/write?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(state.editPath),{method:'POST',headers:{...mut,'Content-Type':'text/plain;charset=utf-8'},body:$('preview').value});toast('Saved '+baseName(state.editPath));refreshFiles()}catch(e){toast(e.message,true)}}
function newFile(){const name=prompt('New file name','notes.txt');if(!name)return;state.editPath=cleanJoin(state.path,name);$('editLabel').textContent=state.fs+':'+state.editPath;$('preview').value='';savePreview()}
async function uploadFiles(files){for(const file of files){const fd=new FormData();fd.append('file',file,file.name);const xhr=new XMLHttpRequest();await new Promise((resolve,reject)=>{xhr.open('POST','/api/upload?fs='+encodeURIComponent(state.fs)+'&path='+encodeURIComponent(state.path));xhr.setRequestHeader('X-ControlOS','1');xhr.upload.onprogress=e=>{if(e.lengthComputable)$('uploadProgress').value=e.loaded/e.total*100};xhr.onload=()=>xhr.status<300?resolve():reject(new Error(xhr.responseText||'Upload failed'));xhr.onerror=()=>reject(new Error('Upload failed'));xhr.send(fd)});toast('Uploaded '+file.name)}$('uploadProgress').value=0;refreshFiles()}
async function otaUpload(){const f=$('otaFile').files[0];if(!f)return toast('Choose a .bin first',true);if(!confirm('Install '+f.name+' and reboot?'))return;const fd=new FormData();fd.append('firmware',f,f.name);const xhr=new XMLHttpRequest();xhr.open('POST','/api/ota');xhr.setRequestHeader('X-ControlOS','1');xhr.upload.onprogress=e=>{if(e.lengthComputable)$('otaProgress').value=e.loaded/e.total*100};xhr.onload=()=>{if(xhr.status<300){toast('Firmware written. Device rebooting.');$('live').textContent='REBOOTING...'}else toast(xhr.responseText||'OTA failed',true)};xhr.onerror=()=>toast('OTA failed',true);xhr.send(fd)}

async function loadLed(){try{const d=await req('/api/led');$('ledEnabled').value=d.enabled?'1':'0';$('ledEffect').innerHTML=d.effects.map((name,i)=>`<option value="${i}">${esc(name)}</option>`).join('');$('ledEffect').value=String(d.effect);$('ledColor').value=String(d.color).toLowerCase();$('ledHex').textContent=String(d.color).toUpperCase();$('ledBrightness').value=d.brightness;$('ledBrightnessValue').textContent=d.brightness+' / 255';$('ledSpeed').value=d.speed;$('ledSpeedValue').textContent=d.speed+'%';updateLedPreview()}catch(e){toast(e.message,true)}}
function ledRgb(hex){const h=hex.replace('#','');return {r:parseInt(h.slice(0,2),16)||0,g:parseInt(h.slice(2,4),16)||0,b:parseInt(h.slice(4,6),16)||0}}
function updateLedPreview(){const c=$('ledColor').value;$('ledHex').textContent=c.toUpperCase();$('ledBrightnessValue').textContent=$('ledBrightness').value+' / 255';$('ledSpeedValue').textContent=$('ledSpeed').value+'%';const br=Number($('ledBrightness').value)/255;$('ledPreview').style.background=c;$('ledPreview').style.opacity=String(Math.max(.08,br));}
async function saveLed(){const rgb=ledRgb($('ledColor').value);const q=new URLSearchParams({enabled:$('ledEnabled').value,effect:$('ledEffect').value,r:rgb.r,g:rgb.g,b:rgb.b,brightness:$('ledBrightness').value,speed:$('ledSpeed').value});try{await req('/api/led?'+q.toString(),{method:'POST',headers:mut});toast('LED settings applied');setTimeout(loadLed,80)}catch(e){toast(e.message,true)}}
function ledPreset(color,effect,brightness,speed){$('ledEnabled').value=effect===0?'0':'1';$('ledColor').value=color;$('ledBrightness').value=brightness;$('ledSpeed').value=speed;$('ledEffect').value=effect;updateLedPreview();saveLed()}
async function loadSettings(){try{const [t,p,a]=await Promise.all([req('/api/theme'),req('/api/power'),req('/api/automation')]);$('themeSelect').innerHTML=t.themes.map((n,i)=>`<option value="${i}">${esc(n)}</option>`).join('');$('themeSelect').value=t.theme;$('powerSelect').innerHTML=p.profiles.map((n,i)=>`<option value="${i}">${esc(n)}</option>`).join('');$('powerSelect').value=p.profile;$('autoLow').value=a.lowBatteryRule?'1':'0';$('autoThreshold').value=a.lowBatteryThreshold;$('autoWeb').value=a.webClientRule?'1':'0';$('batteryDetail').innerHTML=p.battery.present?`Battery ${p.battery.soc}% · ${p.battery.voltage.toFixed(2)} V · ${p.battery.currentMa.toFixed(0)} mA · ${p.battery.temperatureC.toFixed(1)} °C`:'BQ27220 not detected';}catch(e){toast(e.message,true)}}
async function saveTheme(){try{await req('/api/theme?theme='+encodeURIComponent($('themeSelect').value),{method:'POST',headers:mut});toast('Theme applied');loadStatus()}catch(e){toast(e.message,true)}}
async function savePower(){try{await req('/api/power?profile='+encodeURIComponent($('powerSelect').value),{method:'POST',headers:mut});toast('Power profile applied');loadStatus()}catch(e){toast(e.message,true)}}
async function saveAutomation(){const q=new URLSearchParams({lowBatteryRule:$('autoLow').value,lowBatteryThreshold:$('autoThreshold').value,webClientRule:$('autoWeb').value});try{await req('/api/automation?'+q.toString(),{method:'POST',headers:mut});toast('Automations saved')}catch(e){toast(e.message,true)}}
function exportConfig(){window.location='/api/config/export'}
async function importConfig(file){if(!file)return;try{const text=await file.text();await req('/api/config/import',{method:'POST',headers:{...mut,'Content-Type':'application/json'},body:text});toast('Configuration imported');loadLed();loadSettings();loadStatus()}catch(e){toast(e.message,true)}finally{$('configFile').value=''}}
async function factoryReset(){if(!confirm('Reset ControlOS settings and reboot?'))return;try{await req('/api/factory-reset',{method:'POST',headers:mut});toast('Factory reset queued');$('live').textContent='REBOOTING...'}catch(e){toast(e.message,true)}}
let ws;function connectWs(){try{ws=new WebSocket(`ws://${location.hostname}:81/`);ws.onopen=()=>{$('wsState')&&($('wsState').textContent='LIVE · telemetry connected')};ws.onmessage=e=>{try{const s=JSON.parse(e.data);if(s.type==='status')applyStatus(s)}catch{}};ws.onclose=()=>{$('wsState')&&($('wsState').textContent='Disconnected · retrying');setTimeout(connectWs,1800)};ws.onerror=()=>ws.close()}catch{setTimeout(connectWs,1800)}}
async function loadLogs(){try{const d=await req('/api/logs');$('logs').textContent=d.logs.join('\n')}catch(e){}}
for(const b of document.querySelectorAll('.tab'))b.onclick=()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));document.querySelectorAll('.view').forEach(x=>x.classList.remove('active'));b.classList.add('active');$(b.dataset.tab).classList.add('active');if(b.dataset.tab==='files')refreshFiles();if(b.dataset.tab==='led')loadLed();if(b.dataset.tab==='system')loadLogs();if(b.dataset.tab==='settings')loadSettings()};
$('ledColor').oninput=updateLedPreview;$('ledBrightness').oninput=updateLedPreview;$('ledSpeed').oninput=updateLedPreview;
$('fs').onchange=()=>{state.fs=$('fs').value;state.path='/';refreshFiles()};
$('fileInput').onchange=e=>uploadFiles(e.target.files);
const drop=$('drop');['dragenter','dragover'].forEach(n=>drop.addEventListener(n,e=>{e.preventDefault();drop.classList.add('drag')}));['dragleave','drop'].forEach(n=>drop.addEventListener(n,e=>{e.preventDefault();drop.classList.remove('drag')}));drop.addEventListener('drop',e=>uploadFiles(e.dataTransfer.files));
loadStatus();loadApps();loadLed();loadSettings();connectWs();setInterval(loadStatus,5000);setInterval(()=>{if($('system').classList.contains('active'))loadLogs()},2500);
</script>
</body>
</html>
)HTML";
}  // namespace

WebUi::WebUi() : server_(80), ws_(81) {}

void WebUi::begin(App** apps, int appCount, LedController* leds, PowerManager* power, ThemeManager* themes, AutomationEngine* automations) {
  apps_ = apps;
  appCount_ = appCount;
  leds_ = leds;
  power_ = power;
  themes_ = themes;
  automations_ = automations;

  ssid_ = "ControlOS";
  webPassword_ = "control";

  const uint64_t chip = ESP.getEfuseMac();
  sessionToken_ =
      String(static_cast<uint32_t>(chip >> 32), HEX) +
      String(static_cast<uint32_t>(chip), HEX) +
      String(esp_random(), HEX) +
      String(millis(), HEX);

  pinMode(BoardPins::SdCs, OUTPUT);
  digitalWrite(BoardPins::SdCs, HIGH);
  flashReady_ = LittleFS.begin(true);
  sdReady_ = ensureSd();

  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(80);
  WiFi.mode(WIFI_OFF);
  delay(80);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const IPAddress localIp(172, 0, 0, 1);
  const IPAddress gateway(172, 0, 0, 1);
  const IPAddress subnet(255, 255, 255, 0);

  const bool configured =
      WiFi.softAPConfig(
          localIp,
          gateway,
          subnet
      );

  running_ =
      configured &&
      WiFi.softAP(
          ssid_.c_str(),
          kApPassword,
          6,
          false,
          4
      );

  if (running_) {
    dns_.setErrorReplyCode(
        DNSReplyCode::NoError
    );
    dns_.start(
        53,
        "*",
        localIp
    );
  }

  configureRoutes();
  server_.collectHeaders(kHeaderKeys, 2);
  server_.begin();
  ws_.begin();
  ws_.onEvent([](uint8_t, WStype_t, uint8_t*, size_t) {});

  log(String("WebUI started at http://") + ip());
  log(String("Telemetry WebSocket ws://") + ip() + ":81");
  log(String("SSID ") + ssid_ + " / AP password control / WebUI password control");
  if (!flashReady_) log("LittleFS mount failed");
  if (!sdReady_) log("microSD not mounted");
}

void WebUi::loop() {
  if (running_) {
    dns_.processNextRequest();
  }

  server_.handleClient();
  ws_.loop();
  const uint32_t now = millis();
  if (now - lastWsBroadcastMs_ >= 750) {
    lastWsBroadcastMs_ = now;
    String payload = buildStatusJson();
    ws_.broadcastTXT(payload);
  }
}

void WebUi::setUiState(int selected, const char* activeApp) {
  selected_ = selected;
  activeApp_ = activeApp == nullptr ? "Launcher" : activeApp;
}

String WebUi::ip() const {
  return WiFi.softAPIP().toString();
}

uint8_t WebUi::clientCount() const {
  return WiFi.softAPgetStationNum();
}

bool WebUi::popCommand(RemoteCommand& command) {
  if (queueHead_ == queueTail_) return false;
  command = queue_[queueTail_];
  queueTail_ = static_cast<uint8_t>((queueTail_ + 1) % QueueSize);
  return true;
}

void WebUi::enqueue(RemoteCommandType type, int value) {
  const uint8_t next = static_cast<uint8_t>((queueHead_ + 1) % QueueSize);
  if (next == queueTail_) {
    queueTail_ = static_cast<uint8_t>((queueTail_ + 1) % QueueSize);
  }

  queue_[queueHead_].type = type;
  queue_[queueHead_].value = value;
  queueHead_ = next;
}

void WebUi::log(const String& message) {
  const String line = String(millis() / 1000) + "s  " + message;
  logs_[logHead_] = line;
  logHead_ = static_cast<uint8_t>((logHead_ + 1) % LogSize);
  if (logCount_ < LogSize) ++logCount_;
  Serial.println(String("[WEB] ") + message);
}

String WebUi::cookieValue(const String& name) const {
  if (!server_.hasHeader("Cookie")) {
    return "";
  }

  const String cookie =
      server_.header("Cookie");

  const String key =
      name + "=";

  int start =
      cookie.indexOf(key);

  if (start < 0) {
    return "";
  }

  start += key.length();

  int end =
      cookie.indexOf(';', start);

  if (end < 0) {
    end = cookie.length();
  }

  String value =
      cookie.substring(start, end);

  value.trim();

  return value;
}

bool WebUi::authenticated() const {
  const String token =
      cookieValue(kSessionCookie);

  return
      !sessionToken_.isEmpty() &&
      token == sessionToken_;
}

bool WebUi::authorize() {
  if (authenticated()) {
    return true;
  }

  server_.sendHeader(
      "Location",
      kLoginPath,
      true
  );

  server_.send(
      302,
      "text/plain",
      "ControlOS login required"
  );

  return false;
}

void WebUi::sendLogin() {
  if (authenticated()) {
    server_.sendHeader(
        "Location",
        "/",
        true
    );

    server_.send(
        302,
        "text/plain",
        "Already authenticated"
    );

    return;
  }

  server_.send_P(
      200,
      "text/html; charset=utf-8",
      kLoginHtml
  );
}

void WebUi::handleLogin() {
  if (!server_.hasArg("password")) {
    server_.send(
        400,
        "text/plain",
        "Password required"
    );

    return;
  }

  if (server_.arg("password") != webPassword_) {
    log("WebUI login rejected");

    server_.send(
        401,
        "text/html; charset=utf-8",
        "<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<body style='background:#050806;color:#ff6474;font-family:monospace;padding:24px'>"
        "<h2>CONTROL//OS</h2><p>Wrong password.</p>"
        "<p><a style='color:#00ff72' href='/login'>Try again</a></p></body>"
    );

    return;
  }

  const String cookie =
      String(kSessionCookie) +
      "=" +
      sessionToken_ +
      "; Path=/; HttpOnly; SameSite=Lax";

  server_.sendHeader(
      "Set-Cookie",
      cookie
  );

  server_.sendHeader(
      "Location",
      "/",
      true
  );

  server_.send(
      302,
      "text/plain",
      "ControlOS login successful"
  );

  log("WebUI login accepted");
}

void WebUi::handleLogout() {
  server_.sendHeader(
      "Set-Cookie",
      String(kSessionCookie) +
          "=deleted; Path=/; Max-Age=0; HttpOnly; SameSite=Lax"
  );

  server_.sendHeader(
      "Location",
      kLoginPath,
      true
  );

  server_.send(
      302,
      "text/plain",
      "Logged out"
  );
}

bool WebUi::mutationAllowed() {
  if (!authorize()) return false;
  if (!server_.hasHeader(kMutationHeader) || server_.header(kMutationHeader) != kMutationValue) {
    server_.send(403, "application/json", "{\"error\":\"missing mutation guard\"}");
    return false;
  }
  return true;
}

void WebUi::configureRoutes() {
  server_.on(
      kLoginPath,
      HTTP_GET,
      [this]() {
        sendLogin();
      }
  );

  server_.on(
      kLoginPath,
      HTTP_POST,
      [this]() {
        handleLogin();
      }
  );

  server_.on(
      kLogoutPath,
      HTTP_GET,
      [this]() {
        handleLogout();
      }
  );

  server_.on("/", HTTP_GET, [this]() { sendIndex(); });
  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/apps", HTTP_GET, [this]() { sendApps(); });
  server_.on("/api/logs", HTTP_GET, [this]() { sendLogs(); });
  server_.on("/api/control", HTTP_POST, [this]() { handleControl(); });
  server_.on("/api/led", HTTP_GET, [this]() { sendLed(); });
  server_.on("/api/led", HTTP_POST, [this]() { handleLed(); });
  server_.on("/api/theme", HTTP_GET, [this]() { sendTheme(); });
  server_.on("/api/theme", HTTP_POST, [this]() { handleTheme(); });
  server_.on("/api/power", HTTP_GET, [this]() { sendPower(); });
  server_.on("/api/power", HTTP_POST, [this]() { handlePower(); });
  server_.on("/api/automation", HTTP_GET, [this]() { sendAutomation(); });
  server_.on("/api/automation", HTTP_POST, [this]() { handleAutomation(); });
  server_.on("/api/screen", HTTP_GET, [this]() { sendScreenState(); });
  server_.on("/api/config/export", HTTP_GET, [this]() { exportConfig(); });
  server_.on("/api/config/import", HTTP_POST, [this]() { importConfig(); });
  server_.on("/api/factory-reset", HTTP_POST, [this]() { factoryReset(); });

  server_.on("/api/files", HTTP_GET, [this]() { handleFiles(); });
  server_.on("/api/mkdir", HTTP_POST, [this]() { handleMkdir(); });
  server_.on("/api/delete", HTTP_POST, [this]() { handleDelete(); });
  server_.on("/api/rename", HTTP_POST, [this]() { handleRename(); });
  server_.on("/api/read", HTTP_GET, [this]() { handleRead(); });
  server_.on("/api/write", HTTP_POST, [this]() { handleWrite(); });
  server_.on("/api/download", HTTP_GET, [this]() { handleDownload(); });
  server_.on(
      "/api/upload", HTTP_POST, [this]() { handleUploadRequest(); }, [this]() { handleUploadData(); });
  server_.on(
      "/api/ota", HTTP_POST, [this]() { handleOtaRequest(); }, [this]() { handleOtaData(); });

  server_.onNotFound([this]() {
    if (!authenticated()) {
      server_.sendHeader(
          "Location",
          kLoginPath,
          true
      );

      server_.send(
          302,
          "text/plain",
          "ControlOS login"
      );

      return;
    }

    server_.send(
        404,
        "application/json",
        "{\"error\":\"not found\"}"
    );
  });
}

void WebUi::sendIndex() {
  if (!authorize()) return;
  server_.send_P(200, "text/html; charset=utf-8", kIndexHtml);
}

String WebUi::buildStatusJson() const {
  const uint64_t flashTotal = flashReady_ ? LittleFS.totalBytes() : 0;
  const uint64_t flashUsed = flashReady_ ? LittleFS.usedBytes() : 0;
  const uint64_t sdTotal = sdReady_ ? SD.totalBytes() : 0;
  const uint64_t sdUsed = sdReady_ ? SD.usedBytes() : 0;
  String json;
  json.reserve(1300);
  json += "{";
  json += "\"type\":\"status\",";
  json += "\"active\":\"" + jsonEscape(activeApp_) + "\",";
  json += "\"selected\":" + String(selected_) + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"clients\":" + String(clientCount()) + ",";
  json += "\"ssid\":\"" + jsonEscape(ssid_) + "\",";
  json += "\"ip\":\"" + jsonEscape(ip()) + "\",";
  json += "\"heapFreeText\":\"" + formatBytes(ESP.getFreeHeap()) + "\",";
  json += "\"heapMinText\":\"" + formatBytes(ESP.getMinFreeHeap()) + "\",";
  json += "\"psramFreeText\":\"" + formatBytes(ESP.getFreePsram()) + "\",";
  json += "\"flashChipText\":\"" + formatBytes(ESP.getFlashChipSize()) + "\",";
  json += "\"flash\":{";
  json += "\"ready\":" + String(flashReady_ ? "true" : "false") + ",";
  json += "\"usedText\":\"" + formatBytes(flashUsed) + "\",";
  json += "\"totalText\":\"" + formatBytes(flashTotal) + "\"},";
  json += "\"sd\":{";
  json += "\"ready\":" + String(sdReady_ ? "true" : "false") + ",";
  json += "\"usedText\":\"" + formatBytes(sdUsed) + "\",";
  json += "\"totalText\":\"" + formatBytes(sdTotal) + "\"},";
  if (power_ != nullptr) {
    const auto& b = power_->battery();
    json += "\"battery\":{";
    json += "\"present\":" + String(b.present ? "true" : "false") + ",";
    json += "\"soc\":" + String(b.soc) + ",";
    json += "\"voltage\":" + String(b.voltage, 3) + ",";
    json += "\"currentMa\":" + String(b.currentMa, 1) + ",";
    json += "\"temperatureC\":" + String(b.temperatureC, 1) + "},";
    json += "\"powerProfile\":\"" + jsonEscape(power_->profileName()) + "\",";
  }
  if (themes_ != nullptr) json += "\"theme\":\"" + jsonEscape(themes_->themeName()) + "\",";
  json += "\"wsPort\":81";
  json += "}";
  return json;
}

void WebUi::sendStatus() {
  if (!authorize()) return;
  server_.send(200, "application/json", buildStatusJson());
}

void WebUi::sendApps() {
  if (!authorize()) return;
  String json = "{\"apps\":[";
  for (int i = 0; i < appCount_; ++i) {
    if (i > 0) json += ',';
    json += "{\"index\":" + String(i) + ",\"name\":\"" + jsonEscape(apps_[i]->name()) +
            "\",\"short\":\"" + jsonEscape(apps_[i]->shortName()) + "\"}";
  }
  json += "]}";
  server_.send(200, "application/json", json);
}

void WebUi::sendLogs() {
  if (!authorize()) return;
  String json = "{\"logs\":[";
  const uint8_t start = static_cast<uint8_t>((logHead_ + LogSize - logCount_) % LogSize);
  for (uint8_t i = 0; i < logCount_; ++i) {
    if (i > 0) json += ',';
    const uint8_t index = static_cast<uint8_t>((start + i) % LogSize);
    json += "\"" + jsonEscape(logs_[index]) + "\"";
  }
  json += "]}";
  server_.send(200, "application/json", json);
}

void WebUi::handleControl() {
  if (!mutationAllowed()) return;
  const String action = server_.arg("action");
  const int value = server_.arg("value").toInt();

  if (action == "left") enqueue(RemoteCommandType::Encoder, -1);
  else if (action == "right") enqueue(RemoteCommandType::Encoder, 1);
  else if (action == "select") enqueue(RemoteCommandType::Select);
  else if (action == "back") enqueue(RemoteCommandType::Back);
  else if (action == "home") enqueue(RemoteCommandType::Home);
  else if (action == "open" && value >= 0 && value < appCount_) enqueue(RemoteCommandType::OpenApp, value);
  else if (action == "reboot") enqueue(RemoteCommandType::Reboot);
  else if (action == "screen_on") enqueue(RemoteCommandType::ScreenOn);
  else if (action == "screen_off") enqueue(RemoteCommandType::ScreenOff);
  else {
    server_.send(400, "application/json", "{\"error\":\"invalid action\"}");
    return;
  }

  log(String("remote action: ") + action + (action == "open" ? String(" ") + value : ""));
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::sendLed() {
  if (!authorize()) return;
  if (leds_ == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"LED controller unavailable\"}");
    return;
  }

  String json;
  json.reserve(420);
  json += "{";
  json += "\"enabled\":" + String(leds_->enabled() ? "true" : "false") + ",";
  json += "\"effect\":" + String(leds_->effectIndex()) + ",";
  json += "\"effectName\":\"" + jsonEscape(leds_->effectName()) + "\",";
  json += "\"effectCount\":" + String(LedController::effectCount()) + ",";
  json += "\"r\":" + String(leds_->red()) + ",";
  json += "\"g\":" + String(leds_->green()) + ",";
  json += "\"b\":" + String(leds_->blue()) + ",";
  json += "\"color\":\"" + leds_->hexColor() + "\",";
  json += "\"brightness\":" + String(leds_->brightness()) + ",";
  json += "\"speed\":" + String(leds_->speed()) + ",";
  json += "\"effects\":[";
  for (uint8_t i = 0; i < LedController::effectCount(); ++i) {
    if (i > 0) json += ',';
    json += "\"" + jsonEscape(LedController::effectName(static_cast<LedEffect>(i))) + "\"";
  }
  json += "]}";
  server_.send(200, "application/json", json);
}

void WebUi::handleLed() {
  if (!mutationAllowed()) return;
  if (leds_ == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"LED controller unavailable\"}");
    return;
  }

  if (server_.hasArg("enabled")) {
    const String raw = server_.arg("enabled");
    leds_->setEnabled(raw == "1" || raw == "true" || raw == "on");
  }

  if (server_.hasArg("effect")) {
    const int value = server_.arg("effect").toInt();
    if (value < 0 || value >= LedController::effectCount()) {
      server_.send(400, "application/json", "{\"error\":\"invalid LED effect\"}");
      return;
    }
    leds_->setEffect(static_cast<uint8_t>(value));
  }

  if (server_.hasArg("r") || server_.hasArg("g") || server_.hasArg("b")) {
    const int red = server_.hasArg("r") ? server_.arg("r").toInt() : leds_->red();
    const int green = server_.hasArg("g") ? server_.arg("g").toInt() : leds_->green();
    const int blue = server_.hasArg("b") ? server_.arg("b").toInt() : leds_->blue();
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
      server_.send(400, "application/json", "{\"error\":\"RGB values must be 0..255\"}");
      return;
    }
    leds_->setColor(static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue));
  }

  if (server_.hasArg("brightness")) {
    const int value = server_.arg("brightness").toInt();
    if (value < 0 || value > 255) {
      server_.send(400, "application/json", "{\"error\":\"brightness must be 0..255\"}");
      return;
    }
    leds_->setBrightness(static_cast<uint8_t>(value));
  }

  if (server_.hasArg("speed")) {
    const int value = server_.arg("speed").toInt();
    if (value < 1 || value > 100) {
      server_.send(400, "application/json", "{\"error\":\"speed must be 1..100\"}");
      return;
    }
    leds_->setSpeed(static_cast<uint8_t>(value));
  }

  log(String("LED: ") + leds_->effectName() + " " + leds_->hexColor() + " br=" +
      String(leds_->brightness()) + " speed=" + String(leds_->speed()));
  server_.send(200, "application/json", "{\"ok\":true}");
}


void WebUi::sendTheme() {
  if (!authorize()) return;
  if (themes_ == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"theme manager unavailable\"}");
    return;
  }
  String json = "{\"theme\":" + String(themes_->themeIndex()) + ",\"name\":\"" + jsonEscape(themes_->themeName()) + "\",\"themes\":[";
  for (uint8_t i = 0; i < ThemeManager::themeCount(); ++i) {
    if (i) json += ',';
    json += "\"" + jsonEscape(ThemeManager::themeName(static_cast<ThemeManager::ThemeId>(i))) + "\"";
  }
  json += "]}";
  server_.send(200, "application/json", json);
}

void WebUi::handleTheme() {
  if (!mutationAllowed()) return;
  if (themes_ == nullptr || !server_.hasArg("theme")) {
    server_.send(400, "application/json", "{\"error\":\"theme parameter required\"}");
    return;
  }
  const int value = server_.arg("theme").toInt();
  if (value < 0 || value >= ThemeManager::themeCount()) {
    server_.send(400, "application/json", "{\"error\":\"invalid theme\"}");
    return;
  }
  themes_->setTheme(static_cast<uint8_t>(value));
  log(String("theme: ") + themes_->themeName());
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::sendPower() {
  if (!authorize()) return;
  if (power_ == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"power manager unavailable\"}");
    return;
  }
  const auto& b = power_->battery();
  String json = "{\"profile\":" + String(static_cast<uint8_t>(power_->profile())) + ",\"profileName\":\"" + jsonEscape(power_->profileName()) + "\",\"profiles\":[";
  for (uint8_t i = 0; i < PowerManager::profileCount(); ++i) {
    if (i) json += ',';
    json += "\"" + jsonEscape(PowerManager::profileName(static_cast<PowerManager::Profile>(i))) + "\"";
  }
  json += "],\"battery\":{";
  json += "\"present\":" + String(b.present ? "true" : "false") + ",";
  json += "\"soc\":" + String(b.soc) + ",";
  json += "\"voltage\":" + String(b.voltage, 3) + ",";
  json += "\"currentMa\":" + String(b.currentMa, 1) + ",";
  json += "\"temperatureC\":" + String(b.temperatureC, 1) + ",";
  json += "\"remainingMah\":" + String(b.remainingMah) + ",";
  json += "\"fullMah\":" + String(b.fullMah) + "}}";
  server_.send(200, "application/json", json);
}

void WebUi::handlePower() {
  if (!mutationAllowed()) return;
  if (power_ == nullptr || !server_.hasArg("profile")) {
    server_.send(400, "application/json", "{\"error\":\"profile parameter required\"}");
    return;
  }
  const int value = server_.arg("profile").toInt();
  if (value < 0 || value >= PowerManager::profileCount()) {
    server_.send(400, "application/json", "{\"error\":\"invalid power profile\"}");
    return;
  }
  power_->setProfile(static_cast<uint8_t>(value));
  log(String("power profile: ") + power_->profileName());
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::sendAutomation() {
  if (!authorize()) return;
  if (automations_ == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"automation engine unavailable\"}");
    return;
  }
  String json = "{";
  json += "\"lowBatteryRule\":" + String(automations_->lowBatteryRule() ? "true" : "false") + ",";
  json += "\"lowBatteryThreshold\":" + String(automations_->lowBatteryThreshold()) + ",";
  json += "\"webClientRule\":" + String(automations_->webClientRule() ? "true" : "false");
  json += "}";
  server_.send(200, "application/json", json);
}

void WebUi::handleAutomation() {
  if (!mutationAllowed()) return;
  if (automations_ == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"automation engine unavailable\"}");
    return;
  }
  if (server_.hasArg("lowBatteryRule")) {
    const String v = server_.arg("lowBatteryRule");
    automations_->setLowBatteryRule(v == "1" || v == "true" || v == "on");
  }
  if (server_.hasArg("webClientRule")) {
    const String v = server_.arg("webClientRule");
    automations_->setWebClientRule(v == "1" || v == "true" || v == "on");
  }
  if (server_.hasArg("lowBatteryThreshold")) {
    const int v = server_.arg("lowBatteryThreshold").toInt();
    if (v < 5 || v > 50) {
      server_.send(400, "application/json", "{\"error\":\"threshold must be 5..50\"}");
      return;
    }
    automations_->setLowBatteryThreshold(static_cast<uint8_t>(v));
  }
  log("automation settings changed");
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::sendScreenState() {
  if (!authorize()) return;
  String json = "{\"width\":320,\"height\":170,\"active\":\"" + jsonEscape(activeApp_) + "\",\"selected\":" + String(selected_) + ",\"mode\":\"state-mirror\"}";
  server_.send(200, "application/json", json);
}

fs::FS* WebUi::selectFs(const String& name) {
  if (name == "flash") return flashReady_ ? static_cast<fs::FS*>(&LittleFS) : nullptr;
  if (name == "sd") {
    if (!sdReady_) sdReady_ = ensureSd();
    return sdReady_ ? static_cast<fs::FS*>(&SD) : nullptr;
  }
  return nullptr;
}

bool WebUi::ensureSd() {
  return SD.begin(BoardPins::SdCs, SPI, 10000000);
}

String WebUi::normalizePath(const String& raw) const {
  String path = raw;
  path.replace('\\', '/');
  path.trim();
  if (path.isEmpty()) path = "/";
  if (!path.startsWith("/")) path = "/" + path;
  while (path.indexOf("//") >= 0) path.replace("//", "/");
  if (path.indexOf("..") >= 0 || path.length() > 180) return "";
  while (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);
  return path;
}

void WebUi::exportConfig() {
  if (!authorize()) return;
  JsonDocument doc;
  doc["schema"] = 1;
  doc["firmware"] = "ControlOS v1.0";
  if (themes_) doc["theme"] = themes_->themeIndex();
  if (power_) doc["powerProfile"] = static_cast<uint8_t>(power_->profile());
  if (leds_) {
    JsonObject led = doc["led"].to<JsonObject>();
    led["enabled"] = leds_->enabled();
    led["effect"] = leds_->effectIndex();
    led["r"] = leds_->red();
    led["g"] = leds_->green();
    led["b"] = leds_->blue();
    led["brightness"] = leds_->brightness();
    led["speed"] = leds_->speed();
  }
  if (automations_) {
    JsonObject automation = doc["automation"].to<JsonObject>();
    automation["lowBatteryRule"] = automations_->lowBatteryRule();
    automation["lowBatteryThreshold"] = automations_->lowBatteryThreshold();
    automation["webClientRule"] = automations_->webClientRule();
  }
  String json;
  serializeJsonPretty(doc, json);
  server_.sendHeader("Content-Disposition", "attachment; filename=controlos-config.json");
  server_.send(200, "application/json", json);
}

void WebUi::importConfig() {
  if (!mutationAllowed()) return;
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, server_.arg("plain"));
  if (error) {
    server_.send(400, "application/json", String("{\"error\":\"") + jsonEscape(error.c_str()) + "\"}");
    return;
  }
  if (themes_ && doc["theme"].is<uint8_t>()) themes_->setTheme(doc["theme"].as<uint8_t>());
  if (power_ && doc["powerProfile"].is<uint8_t>()) power_->setProfile(doc["powerProfile"].as<uint8_t>());
  if (leds_ && doc["led"].is<JsonObject>()) {
    JsonObject led = doc["led"].as<JsonObject>();
    if (!led["enabled"].isNull()) leds_->setEnabled(led["enabled"].as<bool>());
    if (!led["effect"].isNull()) leds_->setEffect(led["effect"].as<uint8_t>());
    if (!led["r"].isNull() && !led["g"].isNull() && !led["b"].isNull()) {
      leds_->setColor(led["r"].as<uint8_t>(), led["g"].as<uint8_t>(), led["b"].as<uint8_t>());
    }
    if (!led["brightness"].isNull()) leds_->setBrightness(led["brightness"].as<uint8_t>());
    if (!led["speed"].isNull()) leds_->setSpeed(led["speed"].as<uint8_t>());
  }
  if (automations_ && doc["automation"].is<JsonObject>()) {
    JsonObject automation = doc["automation"].as<JsonObject>();
    if (!automation["lowBatteryRule"].isNull()) automations_->setLowBatteryRule(automation["lowBatteryRule"].as<bool>());
    if (!automation["lowBatteryThreshold"].isNull()) automations_->setLowBatteryThreshold(automation["lowBatteryThreshold"].as<uint8_t>());
    if (!automation["webClientRule"].isNull()) automations_->setWebClientRule(automation["webClientRule"].as<bool>());
  }
  log("configuration imported");
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::factoryReset() {
  if (!mutationAllowed()) return;
  static const char* namespaces[] = {
      "control_led", "control_theme", "control_power", "control_auto", "control_sound", "control_ir"};
  Preferences prefs;
  for (const char* ns : namespaces) {
    if (prefs.begin(ns, false)) {
      prefs.clear();
      prefs.end();
    }
  }
  log("factory reset requested");
  server_.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
  enqueue(RemoteCommandType::Reboot);
}

void WebUi::handleFiles() {
  if (!authorize()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String path = normalizePath(server_.arg("path"));
  if (fs == nullptr) {
    server_.send(503, "application/json", "{\"error\":\"filesystem unavailable\"}");
    return;
  }
  if (path.isEmpty()) {
    server_.send(400, "application/json", "{\"error\":\"invalid path\"}");
    return;
  }

  File dir = fs->open(path);
  if (!dir || !dir.isDirectory()) {
    server_.send(404, "application/json", "{\"error\":\"directory not found\"}");
    return;
  }

  String json = "{\"path\":\"" + jsonEscape(path) + "\",\"items\":[";
  bool first = true;
  uint16_t count = 0;
  File entry = dir.openNextFile();
  while (entry && count < 160) {
    String rawName = entry.name();
    const int slash = rawName.lastIndexOf('/');
    String name = slash >= 0 ? rawName.substring(slash + 1) : rawName;
    if (!name.isEmpty()) {
      const String child = path == "/" ? "/" + name : path + "/" + name;
      if (!first) json += ',';
      first = false;
      json += "{\"name\":\"" + jsonEscape(name) + "\",\"path\":\"" + jsonEscape(child) +
              "\",\"dir\":" + String(entry.isDirectory() ? "true" : "false") +
              ",\"size\":" + String(static_cast<uint32_t>(entry.size())) +
              ",\"sizeText\":\"" + formatBytes(entry.size()) + "\"}";
      ++count;
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  json += "]}";
  server_.send(200, "application/json", json);
}

void WebUi::handleMkdir() {
  if (!mutationAllowed()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String path = normalizePath(server_.arg("path"));
  if (fs == nullptr || path.isEmpty() || path == "/") {
    server_.send(400, "application/json", "{\"error\":\"invalid filesystem or path\"}");
    return;
  }
  if (fs->exists(path)) {
    server_.send(409, "application/json", "{\"error\":\"path already exists\"}");
    return;
  }
  if (!fs->mkdir(path)) {
    server_.send(500, "application/json", "{\"error\":\"mkdir failed\"}");
    return;
  }
  log(String("mkdir ") + server_.arg("fs") + ":" + path);
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::handleDelete() {
  if (!mutationAllowed()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String path = normalizePath(server_.arg("path"));
  if (fs == nullptr || path.isEmpty() || path == "/") {
    server_.send(400, "application/json", "{\"error\":\"refusing to delete root\"}");
    return;
  }
  if (!removeRecursive(*fs, path)) {
    server_.send(500, "application/json", "{\"error\":\"delete failed\"}");
    return;
  }
  log(String("delete ") + server_.arg("fs") + ":" + path);
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::handleRename() {
  if (!mutationAllowed()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String from = normalizePath(server_.arg("from"));
  const String to = normalizePath(server_.arg("to"));
  if (fs == nullptr || from.isEmpty() || to.isEmpty() || from == "/" || to == "/") {
    server_.send(400, "application/json", "{\"error\":\"invalid rename path\"}");
    return;
  }
  if (!fs->exists(from) || fs->exists(to)) {
    server_.send(409, "application/json", "{\"error\":\"source missing or destination exists\"}");
    return;
  }
  if (!fs->rename(from, to)) {
    server_.send(500, "application/json", "{\"error\":\"rename failed\"}");
    return;
  }
  log(String("rename ") + from + " -> " + to);
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::handleRead() {
  if (!authorize()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String path = normalizePath(server_.arg("path"));
  if (fs == nullptr || path.isEmpty()) {
    server_.send(400, "text/plain", "Invalid filesystem or path");
    return;
  }
  File file = fs->open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    server_.send(404, "text/plain", "File not found");
    return;
  }
  if (file.size() > 65536) {
    file.close();
    server_.send(413, "text/plain", "Preview limited to 64 KiB");
    return;
  }
  server_.streamFile(file, "text/plain; charset=utf-8");
  file.close();
}

void WebUi::handleWrite() {
  if (!mutationAllowed()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String path = normalizePath(server_.arg("path"));
  if (fs == nullptr || path.isEmpty() || path == "/") {
    server_.send(400, "application/json", "{\"error\":\"invalid filesystem or path\"}");
    return;
  }

  const String body = server_.arg("plain");
  if (body.length() > 65536) {
    server_.send(413, "application/json", "{\"error\":\"editor limited to 64 KiB\"}");
    return;
  }

  File file = fs->open(path, FILE_WRITE);
  if (!file) {
    server_.send(500, "application/json", "{\"error\":\"cannot open file for writing\"}");
    return;
  }
  const size_t written = file.print(body);
  file.close();
  if (written != body.length()) {
    server_.send(500, "application/json", "{\"error\":\"short write\"}");
    return;
  }
  log(String("write ") + server_.arg("fs") + ":" + path + " (" + body.length() + " B)");
  server_.send(200, "application/json", "{\"ok\":true}");
}

void WebUi::handleDownload() {
  if (!authorize()) return;
  fs::FS* fs = selectFs(server_.arg("fs"));
  const String path = normalizePath(server_.arg("path"));
  if (fs == nullptr || path.isEmpty()) {
    server_.send(400, "application/json", "{\"error\":\"invalid filesystem or path\"}");
    return;
  }
  File file = fs->open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    server_.send(404, "application/json", "{\"error\":\"file not found\"}");
    return;
  }
  String filename = path.substring(path.lastIndexOf('/') + 1);
  filename.replace('"', '_');
  server_.sendHeader("Content-Disposition", String("attachment; filename=\"") + filename + "\"");
  server_.streamFile(file, contentTypeFor(path));
  file.close();
}

void WebUi::handleUploadRequest() {
  if (!mutationAllowed()) return;
  if (uploadOk_) {
    log("file upload completed");
    server_.send(200, "application/json", "{\"ok\":true}");
  } else {
    const String error = uploadError_.isEmpty() ? "upload failed" : uploadError_;
    server_.send(500, "application/json", String("{\"error\":\"") + jsonEscape(error) + "\"}");
  }
}

void WebUi::handleUploadData() {
  if (!server_.authenticate("admin", webPassword_.c_str())) return;
  if (!server_.hasHeader(kMutationHeader) || server_.header(kMutationHeader) != kMutationValue) return;

  HTTPUpload& upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadOk_ = false;
    uploadError_ = "";
    uploadFs_ = selectFs(server_.arg("fs"));
    String base = normalizePath(server_.arg("path"));
    if (uploadFs_ == nullptr || base.isEmpty()) {
      uploadError_ = "filesystem unavailable or invalid path";
      return;
    }

    String filename = upload.filename;
    filename.replace('\\', '/');
    const int slash = filename.lastIndexOf('/');
    if (slash >= 0) filename = filename.substring(slash + 1);
    if (filename.isEmpty() || filename == "." || filename == "..") {
      uploadError_ = "invalid filename";
      return;
    }

    const String target = base == "/" ? "/" + filename : base + "/" + filename;
    uploadFile_ = uploadFs_->open(target, FILE_WRITE);
    if (!uploadFile_) uploadError_ = "cannot open destination";
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!uploadFile_) return;
    if (uploadFile_.write(upload.buf, upload.currentSize) != upload.currentSize) {
      uploadError_ = "write failed";
      uploadFile_.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile_) {
      uploadFile_.close();
      uploadOk_ = uploadError_.isEmpty();
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile_) uploadFile_.close();
    uploadError_ = "upload aborted";
    uploadOk_ = false;
  }
}

void WebUi::handleOtaRequest() {
  if (!mutationAllowed()) return;
  if (otaOk_) {
    log("OTA completed; reboot queued");
    server_.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
    enqueue(RemoteCommandType::Reboot);
  } else {
    const String error = otaError_.isEmpty() ? "OTA failed" : otaError_;
    server_.send(500, "application/json", String("{\"error\":\"") + jsonEscape(error) + "\"}");
  }
}

void WebUi::handleOtaData() {
  if (!server_.authenticate("admin", webPassword_.c_str())) return;
  if (!server_.hasHeader(kMutationHeader) || server_.header(kMutationHeader) != kMutationValue) return;

  HTTPUpload& upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaOk_ = false;
    otaError_ = "";
    if (!upload.filename.endsWith(".bin")) {
      otaError_ = "firmware must be a .bin";
      return;
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      otaError_ = String("Update.begin failed: ") + Update.errorString();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaError_.isEmpty()) return;
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      otaError_ = String("Update.write failed: ") + Update.errorString();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!otaError_.isEmpty()) {
      Update.abort();
      return;
    }
    if (!Update.end(true)) {
      otaError_ = String("Update.end failed: ") + Update.errorString();
      return;
    }
    otaOk_ = true;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaError_ = "OTA aborted";
    otaOk_ = false;
  }
}

bool WebUi::removeRecursive(fs::FS& fs, const String& path, uint8_t depth) {
  if (depth > 8 || path == "/") return false;

  File node = fs.open(path, FILE_READ);
  if (!node) return false;
  if (!node.isDirectory()) {
    node.close();
    return fs.remove(path);
  }
  node.close();

  while (true) {
    File dir = fs.open(path, FILE_READ);
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      return false;
    }

    File child = dir.openNextFile();
    if (!child) {
      dir.close();
      break;
    }

    String rawName = child.name();
    const int slash = rawName.lastIndexOf('/');
    const String name = slash >= 0 ? rawName.substring(slash + 1) : rawName;
    child.close();
    dir.close();

    if (name.isEmpty()) return false;
    const String childPath = path + "/" + name;
    if (!removeRecursive(fs, childPath, depth + 1)) return false;
  }

  return fs.rmdir(path);
}

String WebUi::jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<uint8_t>(c) < 0x20) out += '?';
        else out += c;
        break;
    }
  }
  return out;
}

String WebUi::formatBytes(uint64_t bytes) {
  if (bytes < 1024ULL) return String(static_cast<unsigned long>(bytes)) + " B";
  if (bytes < 1024ULL * 1024ULL) return String(static_cast<double>(bytes) / 1024.0, 1) + " KiB";
  if (bytes < 1024ULL * 1024ULL * 1024ULL)
    return String(static_cast<double>(bytes) / (1024.0 * 1024.0), 1) + " MiB";
  return String(static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), 2) + " GiB";
}

String WebUi::contentTypeFor(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".txt") || path.endsWith(".log") || path.endsWith(".md")) return "text/plain";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".gif")) return "image/gif";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".pdf")) return "application/pdf";
  return "application/octet-stream";
}
