#pragma once

// The LFR calibration page, kept out of the .ino on purpose: the Arduino /
// PlatformIO prototype generator parses .ino files and mistakes the
// JavaScript "function" keywords in this raw string for C++ declarations,
// which breaks the build. Headers are passed through untouched.

#include <Arduino.h>

static const char tunerPage[] PROGMEM = R"HTML(
<meta name=viewport content="width=device-width,initial-scale=1">
<title>LFR Tuner</title>
<style>
:root{color-scheme:dark}
body{background:#111;color:#eee;font:15px/1.4 system-ui,sans-serif;margin:0;padding:12px}
h1{font-size:17px;margin:0 0 10px}
h1 span{font-family:ui-monospace,monospace;font-size:14px;color:#9c9}
.tel{background:#000;border:1px solid #333;border-radius:8px;padding:10px;margin-bottom:12px}
.dots{display:flex;gap:6px;margin-bottom:8px}
.d{flex:1;height:40px;border-radius:6px;background:#222;border:1px solid #444;
   display:flex;align-items:center;justify-content:center;font-size:12px;color:#666}
.d.on{background:#0c0;color:#000;border-color:#0f0;font-weight:700}
.mode{font-size:18px;font-weight:700;color:#5cf;font-family:ui-monospace,monospace}
.lr{font-family:ui-monospace,monospace;color:#aaa;float:right}
.row{display:flex;align-items:center;gap:6px;margin-bottom:6px}
.row label{flex:1;font-family:ui-monospace,monospace;font-size:13px}
input{width:62px;background:#000;color:#eee;border:1px solid #555;border-radius:6px;
      padding:8px;font-size:16px;text-align:center}
button{background:#333;color:#eee;border:1px solid #666;border-radius:6px;
       padding:9px 11px;font-size:15px}
button:active{background:#555}
.acts{display:flex;flex-wrap:wrap;gap:8px;margin-top:14px}
.acts button{flex:1;min-width:84px;padding:14px}
.go{background:#063;border-color:#0a5}.no{background:#600;border-color:#a00}
.msg{margin-top:10px;font-family:ui-monospace,monospace;font-size:12px;
     color:#9c9;min-height:16px;word-break:break-all}
a{color:#68f;font-size:13px}
</style>
<h1>LFR Tuner <span id=st>[...]</span></h1>
<div class=tel>
  <div class=dots id=dots></div>
  <span class=mode id=mode>--</span><span class=lr id=lr></span>
</div>
<div id=vars></div>
<div class=acts>
  <button class=go onclick="cmd('START')">START</button>
  <button class=no onclick="cmd('STOP')">STOP</button>
  <button onclick="cmd('RAMP')">RAMP</button>
  <button onclick="cmd('SAVE')">SAVE</button>
</div>
<div class=msg id=msg></div>
<p><a href="/mp3">MP3 / RFID controls</a></p>
<script>
// key -> friendly label, matching the Nano's tuneTable
var V=[["MIN","min_drive"],["BASE","base_speed"],["CO","curve_outer"],
       ["CI","curve_inner"],["SO","sharp_outer"],["SI","sharp_inner"],
       ["RB","right_turn_boost"],
       ["SRCH","search_speed"],["SRCHI","search_inner"],
       ["JUNC","junction_speed"],["JUNCI","junction_inner"],
       ["LDIR","lost_dir"]];
var vars=document.getElementById('vars');
V.forEach(function(e){
  var k=e[0],d=document.createElement('div');d.className='row';
  d.innerHTML='<label>'+e[1]+'</label>'+
    '<button onclick="bump(\''+k+'\',-5)">-5</button>'+
    '<input id="k_'+k+'" type=number>'+
    '<button onclick="bump(\''+k+'\',5)">+5</button>';
  vars.appendChild(d);
});
function set(k,v){fetch('/tuneset?k='+k+'&v='+v);}
function bump(k,d){var i=document.getElementById('k_'+k);
  i.value=(parseInt(i.value||'0',10)+d);set(k,i.value);}
function cmd(c){fetch('/tunecmd?c='+c);}
V.forEach(function(e){var i=document.getElementById('k_'+e[0]);
  i.addEventListener('change',function(){set(e[0],i.value);});});
var dots=document.getElementById('dots');
for(var n=1;n<=5;n++){var d=document.createElement('div');
  d.className='d';d.id='s'+n;d.textContent='S'+n;dots.appendChild(d);}
function poll(){
  fetch('/tunedata').then(function(r){return r.json();}).then(function(r){
    var t=r.t||'';
    for(var n=1;n<=5;n++){
      var m=t.match(new RegExp('S'+n+'=(\\d)'));
      document.getElementById('s'+n).className='d'+(m&&m[1]=='1'?' on':'');
    }
    var mm=t.match(/M=(\S+)/);
    document.getElementById('mode').textContent=mm?mm[1]:'--';
    var l=t.match(/L=(-?\d+)/),rr=t.match(/R=(-?\d+)/);
    document.getElementById('lr').textContent=(l&&rr)?'L='+l[1]+' R='+rr[1]:'';
    document.getElementById('st').textContent=r.r?'[RUNNING]':'[STOPPED]';
    (r.v||'').split(' ').forEach(function(p){
      var kv=p.split('='),i=document.getElementById('k_'+kv[0]);
      // never clobber a field mid-edit
      if(i&&document.activeElement!==i)i.value=kv[1];
    });
    document.getElementById('msg').textContent=r.m||'';
  }).catch(function(){});
}
setInterval(poll,300);poll();
// Ask the Nano for its real values on load and every 3 s. The boxes then show
// the firmware's actual state, so a brownout that reloads EEPROM is visible
// instead of silently contradicting what was typed.
function refresh(){fetch('/tunecmd?c=GET');}
setTimeout(refresh,600);setInterval(refresh,3000);
</script>
)HTML";
