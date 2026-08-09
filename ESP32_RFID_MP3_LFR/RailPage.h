#pragma once

// Rail-test page, served at /rail. Deliberately minimal: no line-following
// values, no steering. Just RAMP and a fixed PWM per side, so the drivetrain
// can be characterised on a straight rail instead of inferred from how the
// robot misbehaves on a track.
//
// Kept out of the .ino for the same reason as TunerPage.h -- the Arduino /
// PlatformIO prototype generator parses .ino files and mistakes the JavaScript
// "function" keywords in this raw string for C++ declarations.

#include <Arduino.h>

static const char railPage[] PROGMEM = R"HTML(
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Rail Test</title>
<style>
:root{color-scheme:dark}
body{background:#111;color:#eee;font:15px/1.4 system-ui,sans-serif;margin:0;padding:12px}
h1{font-size:17px;margin:0 0 4px}
h1 span{font-family:ui-monospace,monospace;font-size:14px;color:#9c9}
.hint{color:#888;font-size:12px;margin:0 0 12px}
.tel{background:#000;border:1px solid #333;border-radius:8px;padding:12px;margin-bottom:14px}
.mode{font-size:18px;font-weight:700;color:#5cf;font-family:ui-monospace,monospace}
.lr{font-family:ui-monospace,monospace;font-size:20px;color:#eee;float:right}
.row{display:flex;align-items:center;gap:6px;margin-bottom:10px}
.row label{flex:1;font-family:ui-monospace,monospace;font-size:15px}
input{width:74px;background:#000;color:#eee;border:1px solid #555;border-radius:6px;
      padding:10px;font-size:18px;text-align:center}
button{background:#333;color:#eee;border:1px solid #666;border-radius:6px;
       padding:10px 12px;font-size:15px}
button:active{background:#555}
.acts{display:flex;flex-wrap:wrap;gap:8px;margin-top:8px}
.acts button{flex:1;min-width:84px;padding:15px}
.go{background:#063;border-color:#0a5}.no{background:#600;border-color:#a00}
.sync{background:#134;border-color:#28a}
.msg{margin-top:12px;font-family:ui-monospace,monospace;font-size:12px;
     color:#9c9;min-height:16px;word-break:break-all}
a{color:#68f;font-size:13px}
</style>
<h1>Rail Test <span id=st>[...]</span></h1>
<p class=hint>Sensors are ignored in this mode. Values bypass the min_drive
floor, so what you type is what the motors get.</p>
<div class=tel>
  <span class=mode id=mode>--</span><span class=lr id=lr></span>
</div>
<div class=row>
  <label>left&nbsp;&nbsp;(ML)</label>
  <button onclick="bump('ML',-5)">-5</button>
  <button onclick="bump('ML',-1)">-1</button>
  <input id="k_ML" type=number>
  <button onclick="bump('ML',1)">+1</button>
  <button onclick="bump('ML',5)">+5</button>
</div>
<div class=row>
  <label>right (MR)</label>
  <button onclick="bump('MR',-5)">-5</button>
  <button onclick="bump('MR',-1)">-1</button>
  <input id="k_MR" type=number>
  <button onclick="bump('MR',1)">+1</button>
  <button onclick="bump('MR',5)">+5</button>
</div>
<div class=acts>
  <button class=sync onclick="match()">MATCH L=R</button>
  <button onclick="cmd('RAMP')">RAMP</button>
</div>
<div class=acts>
  <button class=go onclick="cmd('MAN')">DRIVE</button>
  <button class=no onclick="cmd('STOP')">STOP</button>
  <button onclick="cmd('SAVE')">SAVE</button>
</div>
<div class=msg id=msg></div>
<p><a href="/tuner">LFR tuner</a> &nbsp;|&nbsp; <a href="/mp3">MP3 / RFID</a></p>
<script>
function set(k,v){fetch('/tuneset?k='+k+'&v='+v);}
function cmd(c){fetch('/tunecmd?c='+c);}
function bump(k,d){var i=document.getElementById('k_'+k);
  i.value=(parseInt(i.value||'0',10)+d);set(k,i.value);}
// copy the left value onto the right, for finding the point where equal PWM
// still does not mean equal wheel speed
function match(){var l=document.getElementById('k_ML').value;
  document.getElementById('k_MR').value=l;set('MR',l);}
['ML','MR'].forEach(function(k){var i=document.getElementById('k_'+k);
  i.addEventListener('change',function(){set(k,i.value);});});
function poll(){
  fetch('/tunedata').then(function(r){return r.json();}).then(function(r){
    var t=r.t||'';
    var mm=t.match(/M=(\S+)/);
    document.getElementById('mode').textContent=mm?mm[1]:'--';
    var l=t.match(/L=(-?\d+)/),rr=t.match(/R=(-?\d+)/);
    document.getElementById('lr').textContent=(l&&rr)?'L='+l[1]+'  R='+rr[1]:'';
    document.getElementById('st').textContent=r.r?'[RUNNING]':'[STOPPED]';
    (r.v||'').split(' ').forEach(function(p){
      var kv=p.split('='),i=document.getElementById('k_'+kv[0]);
      if(i&&document.activeElement!==i)i.value=kv[1];
    });
    document.getElementById('msg').textContent=r.m||'';
  }).catch(function(){});
}
setInterval(poll,300);poll();
function refresh(){fetch('/tunecmd?c=GET');}
setTimeout(refresh,600);setInterval(refresh,3000);
</script>
)HTML";
