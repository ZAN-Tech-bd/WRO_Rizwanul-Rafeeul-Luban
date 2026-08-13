#pragma once

// Robot-side field console served directly by the ESP32 SoftAP. Kept outside
// the .ino so Arduino's prototype generator cannot mistake JavaScript function
// declarations inside the raw string for C++ functions.

#include <Arduino.h>

static const char controlPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#d7e5e1">
<title>Robot Field Console</title>
<style>
:root{color-scheme:light;--paper:#f5f7f4;--panel:#fff;--ink:#172421;--muted:#64736e;
  --line:#cbd8d3;--teal:#0c7568;--teal2:#07584f;--amber:#d48b18;--red:#a33a32;
  --terminal:#13211e;--terminalInk:#bfe7d9;--shadow:0 14px 34px rgba(22,51,44,.10)}
*{box-sizing:border-box}
body{margin:0;background:linear-gradient(135deg,#d7e5e1 0 18%,var(--paper) 18% 100%);
  color:var(--ink);font:15px/1.45 "Trebuchet MS",Verdana,sans-serif;min-height:100vh}
main{width:min(920px,calc(100% - 28px));margin:auto;padding:28px 0 42px}
header{display:flex;align-items:flex-end;justify-content:space-between;gap:18px;margin-bottom:18px}
.eyebrow{color:var(--teal2);font:700 11px/1.2 "Courier New",monospace;letter-spacing:.14em;
  text-transform:uppercase;margin:0 0 5px}
h1{font:800 clamp(27px,6vw,48px)/.95 Georgia,serif;letter-spacing:-.035em;margin:0}
.address{color:var(--muted);font:12px/1.4 "Courier New",monospace;text-align:right}
.grid{display:grid;grid-template-columns:minmax(250px,.8fr) minmax(330px,1.2fr);gap:14px}
.stack{display:grid;gap:14px}
.panel{background:rgba(255,255,255,.94);border:1px solid var(--line);border-radius:16px;
  box-shadow:var(--shadow);overflow:hidden}
.panelHead{display:flex;align-items:center;justify-content:space-between;padding:14px 16px;
  border-bottom:1px solid var(--line)}
h2{font:700 15px/1.2 Georgia,serif;margin:0}.tag{font:700 10px/1 "Courier New",monospace;
  letter-spacing:.08em;text-transform:uppercase;color:var(--teal2)}
.body{padding:16px}.readout{display:flex;align-items:baseline;justify-content:center;gap:8px;
  padding:10px 0 15px}.number{font:800 64px/.85 Georgia,serif;color:var(--teal2)}
.scale{font:12px/1 "Courier New",monospace;color:var(--muted)}
input[type=range]{width:100%;accent-color:var(--teal);cursor:pointer}
.ticks{display:flex;justify-content:space-between;color:var(--muted);font:11px "Courier New",monospace}
.buttons{display:grid;grid-template-columns:1fr 1fr 1.35fr;gap:8px;margin-top:16px}
button{border:1px solid var(--line);border-radius:9px;background:#eef3f0;color:var(--ink);
  padding:11px 8px;font:700 13px "Trebuchet MS",sans-serif;cursor:pointer}
button:hover{border-color:var(--teal);color:var(--teal2)}button:active{transform:translateY(1px)}
button.primary{background:var(--teal);border-color:var(--teal);color:#fff}
button.danger{color:var(--red)}button:focus-visible,input:focus-visible{outline:3px solid #f4bd62;outline-offset:2px}
.status{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:15px}
.datum{border-top:1px solid var(--line);padding-top:9px}.label{display:block;color:var(--muted);
  font:10px/1.4 "Courier New",monospace;text-transform:uppercase;letter-spacing:.08em}
.value{display:block;font-weight:700;margin-top:2px;overflow-wrap:anywhere}.ok{color:var(--teal2)}
.motor{margin-top:15px}.motor:first-child{margin-top:0}.motorLine{display:flex;align-items:baseline;
  justify-content:space-between;gap:10px;margin-bottom:5px}.motorName{font:700 13px Georgia,serif}
.motorValue{font:800 24px/1 Georgia,serif;color:var(--teal2)}.motorMeta{color:var(--muted);
  font:10px/1.4 "Courier New",monospace;margin-top:10px}.motorActions{grid-template-columns:1fr 1fr;margin-top:14px}
.terminal{background:var(--terminal);color:var(--terminalInk);height:390px;margin:0;padding:14px 16px;
  overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;font:12px/1.55 "Courier New",monospace;
  tab-size:2}.terminal::selection{background:var(--amber);color:var(--terminal)}
.logTools{display:flex;gap:7px}.logTools button{padding:7px 10px;background:transparent;font-size:11px}
.logTools button:first-child{color:var(--teal2)}.empty{color:#829b92}
.foot{color:var(--muted);font:11px/1.5 "Courier New",monospace;margin:12px 2px 0}
.muteNote{background:#eaf1fb;border:1px solid #c3d6f0;border-radius:9px;color:#1a4fa0;
  font:11px/1.45 "Trebuchet MS",sans-serif;margin:12px 0 0;padding:9px 11px}
.alarm{display:none;background:var(--red);color:#fff;border-radius:12px;padding:12px 16px;margin-bottom:14px;
  font:700 14px/1.4 "Trebuchet MS",sans-serif;box-shadow:var(--shadow)}
.alarm.on{display:block}.alarm span{display:block;font-weight:400;font-size:12px;margin-top:4px;opacity:.9}
@media(prefers-reduced-motion:no-preference){.alarm.on{animation:pulse 1.1s ease-in-out infinite}
  @keyframes pulse{50%{opacity:.72}}}
@media(max-width:700px){main{padding-top:20px}.grid{grid-template-columns:1fr}header{align-items:flex-start;
  flex-direction:column}.address{text-align:left}.terminal{height:340px}}
@media(prefers-reduced-motion:no-preference){.panel{animation:arrive .35s ease-out both}.panel:nth-child(2){animation-delay:.08s}
  @keyframes arrive{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:none}}}
</style>
</head>
<body>
<main>
  <header>
    <div><p class="eyebrow">On-board service panel</p><h1>Robot<br>Field Console</h1></div>
    <div class="address">ESP32-Robot-Main<br>192.168.4.1<br><span id="hubLink">Hub link …</span></div>
  </header>
  <div class="alarm" id="alarm">Hub emergency active — every card except the END card is ignored.
    <span id="alarmDetail">Running to the END card.</span></div>
  <div class="grid">
    <div class="stack">
      <section class="panel">
        <div class="panelHead"><h2>Speaker level</h2><span class="tag" id="audioTag">checking</span></div>
        <div class="body">
          <div class="readout"><span class="number" id="volumeValue">--</span><span class="scale">/ 30</span></div>
          <input id="volume" type="range" min="0" max="30" step="1" value="20" aria-label="DFPlayer volume">
          <div class="ticks"><span>silent</span><span>field max</span></div>
          <p class="muteNote" id="muteNote" hidden>Hub mute is ON — the speaker is at 0 and this
            slider only sets the level to return to when the admin unmutes.</p>
          <div class="buttons">
            <button type="button" onclick="nudge(-1)">− one</button>
            <button type="button" onclick="nudge(1)">+ one</button>
            <button class="primary" id="apply" type="button" onclick="applyVolume()">Set volume</button>
          </div>
          <div class="status">
            <div class="datum"><span class="label">Drive</span><span class="value" id="drive">—</span></div>
            <div class="datum"><span class="label">Playback</span><span class="value" id="playback">—</span></div>
            <div class="datum"><span class="label">Last RFID</span><span class="value" id="card">—</span></div>
            <div class="datum"><span class="label">Camera</span><span class="value" id="camera">—</span></div>
          </div>
        </div>
      </section>
      <section class="panel">
        <div class="panelHead"><h2>Drive balance</h2><span class="tag" id="nanoTag">waiting for Nano</span></div>
        <div class="body">
          <div class="motor">
            <div class="motorLine"><span class="motorName">Motor L · channel A</span><span class="motorValue" id="mlValue">150</span></div>
            <input id="ml" type="range" min="0" max="255" step="1" value="150" aria-label="Left motor PWM">
          </div>
          <div class="motor">
            <div class="motorLine"><span class="motorName">Motor R · channel B</span><span class="motorValue" id="mrValue">210</span></div>
            <input id="mr" type="range" min="0" max="255" step="1" value="210" aria-label="Right motor PWM">
          </div>
          <div class="ticks"><span>0 · off</span><span>absolute PWM · 255</span></div>
          <div class="buttons motorActions">
            <button class="primary" id="motorApply" type="button" onclick="applyMotors()">Apply live</button>
            <button id="motorSave" type="button" onclick="saveMotors()">Save to Nano</button>
          </div>
          <div class="motorMeta" id="motorMeta">Apply updates both channels. If the drive is moving, the Nano changes PWM immediately.</div>
        </div>
      </section>
    </div>
    <section class="panel">
      <div class="panelHead">
        <h2>Robot event tape</h2>
        <div class="logTools"><button type="button" onclick="togglePause()" id="pause">Pause</button>
          <button class="danger" type="button" onclick="clearLog()">Clear</button></div>
      </div>
      <pre class="terminal empty" id="log">Waiting for robot events…</pre>
    </section>
  </div>
  <p class="foot">The event tape is a bounded RAM log, not permanent storage. It refreshes once per second and survives browser reconnects until the ESP32 restarts or Clear is pressed.</p>
</main>
<script>
var paused=false,lastSequence=-1,editing=false,motorEditing=false;
var slider=document.getElementById('volume'),readout=document.getElementById('volumeValue');
var ml=document.getElementById('ml'),mr=document.getElementById('mr');
slider.addEventListener('input',function(){editing=true;readout.textContent=slider.value;});
function motorPreview(){motorEditing=true;document.getElementById('mlValue').textContent=ml.value;document.getElementById('mrValue').textContent=mr.value;}
ml.addEventListener('input',motorPreview);mr.addEventListener('input',motorPreview);
function nudge(delta){editing=true;slider.value=Math.max(0,Math.min(30,parseInt(slider.value,10)+delta));readout.textContent=slider.value;}
function applyVolume(){var b=document.getElementById('apply');b.disabled=true;b.textContent='Setting…';
  fetch('/api/volume?v='+encodeURIComponent(slider.value),{method:'POST'}).then(function(r){if(!r.ok)throw Error();return r.json();})
  .then(function(d){editing=false;slider.value=d.volume;readout.textContent=d.volume;b.textContent='Volume set';setTimeout(function(){b.textContent='Set volume';},900);})
  .catch(function(){b.textContent='Set failed';}).then(function(){b.disabled=false;});}
function motorRequest(path,button,working,done){var b=document.getElementById(button);b.disabled=true;b.textContent=working;
  fetch(path+'?ml='+encodeURIComponent(ml.value)+'&mr='+encodeURIComponent(mr.value),{method:'POST'})
  .then(function(r){if(!r.ok)throw Error();return r.json();}).then(function(d){motorEditing=false;ml.value=d.ml;mr.value=d.mr;
    document.getElementById('mlValue').textContent=d.ml;document.getElementById('mrValue').textContent=d.mr;b.textContent=done;
    setTimeout(function(){b.textContent=button==='motorSave'?'Save to Nano':'Apply live';},1000);})
  .catch(function(){b.textContent='Command failed';}).then(function(){b.disabled=false;});}
function applyMotors(){motorRequest('/api/motors','motorApply','Applying…','Applied');}
function saveMotors(){motorRequest('/api/motors/save','motorSave','Saving…','Saved');}
function togglePause(){paused=!paused;document.getElementById('pause').textContent=paused?'Resume':'Pause';if(!paused)poll();}
function clearLog(){fetch('/api/log/clear',{method:'POST'}).then(function(){lastSequence=-1;document.getElementById('log').textContent='Log cleared.';});}
function render(d){if(!editing){slider.value=d.volume;readout.textContent=d.volume;}
  document.getElementById('alarm').className='alarm'+(d.emergency?' on':'');
  document.getElementById('alarmDetail').textContent=d.drive;
  document.getElementById('hubLink').textContent=d.hubLink?'Hub link listening':'Hub link off';
  document.getElementById('muteNote').hidden=!d.muted;
  var tag=document.getElementById('audioTag');
  tag.textContent=d.muted?'muted by Hub':(d.audio?'DFPlayer online':'DFPlayer offline');
  tag.className='tag'+(d.audio&&!d.muted?' ok':'');
  document.getElementById('drive').textContent=d.drive;document.getElementById('playback').textContent=d.playback;
  document.getElementById('card').textContent=d.card;document.getElementById('camera').textContent=d.camera;
  if(!motorEditing){ml.value=d.motorL;mr.value=d.motorR;document.getElementById('mlValue').textContent=d.motorL;document.getElementById('mrValue').textContent=d.motorR;}
  var nt=document.getElementById('nanoTag');nt.textContent=d.nanoTuned?'Nano confirmed':'awaiting confirmation';nt.className='tag'+(d.nanoTuned?' ok':'');
  document.getElementById('motorMeta').textContent=d.motorPending?'Motor command sent; waiting for Nano TUNE reply…':
    'Nano reports ML='+d.motorL+' / MR='+d.motorR+(d.driveRunning?' · live output active':' · drive stopped');
  if(!paused&&d.logSequence!==lastSequence){var box=document.getElementById('log');box.textContent=d.log||'No events recorded.';
    box.className='terminal'+(d.log?'':' empty');box.scrollTop=box.scrollHeight;lastSequence=d.logSequence;}}
function poll(){fetch('/api/state',{cache:'no-store'}).then(function(r){return r.json();}).then(render).catch(function(){});}
setInterval(function(){if(!paused)poll();},1000);poll();
</script>
</body>
</html>
)HTML";
