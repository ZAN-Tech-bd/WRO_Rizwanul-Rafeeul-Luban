// Six-station bench web page. Kept in a header so the Arduino .ino
// prototype generator never sees the JavaScript inside the raw string.
#pragma once
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Hub Bench S1-S6</title><style>
body{font-family:monospace;background:#101418;color:#d8e0e8;margin:0;padding:16px}
h1{font-size:18px}h2{font-size:14px;color:#7fb3ff;margin:18px 0 6px}
table{border-collapse:collapse;width:100%}
td,th{padding:4px 10px 4px 0;font-size:14px;text-align:left;border-bottom:1px solid #222}
.v{color:#ffd479}.ok{color:#6fdc8c}.bad{color:#ff7b7b}.sw{display:inline-block;width:12px;height:12px;border:1px solid #555}
button{font-family:monospace;font-size:13px;margin:2px;padding:5px 9px;cursor:pointer}
.sel{outline:2px solid #ffd479}
.r{background:#c0392b;color:#fff}.g{background:#27ae60;color:#fff}
.b{background:#2980b9;color:#fff}.w{background:#ecf0f1;color:#111}
.o{background:#333;color:#ddd}.n{background:#1d2630;color:#d8e0e8}
#note{color:#8aa;font-size:12px}
</style></head><body>
<h1>WRO Museum - 6-station bench (hub J10-J15)</h1>
<h2>LED chain (48 px = 6 x 8 WS2812 via 74HCT125, GPIO45)</h2>
<div id="chips"></div>
<div>
<button class="r" onclick="led('r')">Red</button>
<button class="g" onclick="led('g')">Green</button>
<button class="b" onclick="led('b')">Blue</button>
<button class="w" onclick="led('w')">White</button>
<button class="o" onclick="led('0')">OFF</button>
<button onclick="led('a')">RGB cycle all</button>
<button onclick="led('t')">Pixel walk chain</button>
</div>
<div>Mode: <span id="ledmode" class="v">-</span></div>
<div id="note">Pick ALL or one station above, then a colour. Station order = chain order:
populate S1,S2,S3... consecutively - an empty middle port breaks the chain to later stations.</div>
<h2>Sensors (DHT GPIO15/16/17/18/38/39 - MQ ADC 4/5/6/7/1/2 - PROX zones 41/42/3)</h2>
<table><thead><tr><th>#</th><th>Station</th><th>LED</th><th>DHT11 &deg;C / %</th>
<th>MQ raw</th><th>MQ V pin / AO</th><th>PROX</th></tr></thead>
<tbody id="rows"></tbody></table>
<div id="note">PROX is shared per zone pair: S1+S2 = Z1, S3+S4 = Z2, S5+S6 = Z3 (LOW = detected).
MQ raw values only - not calibrated ppm; sensors need warm-up minutes.</div>
<h2>Board</h2>
<table>
<tr><td>Uptime</td><td><span id="up" class="v">-</span></td></tr>
<tr><td>AP clients</td><td><span id="cl" class="v">-</span></td></tr>
<tr><td>Wi-Fi</td><td>S1Bench / museum2026 - http://192.168.4.1/</td></tr>
</table>
<script>
let sel=0;
const chips=document.getElementById('chips');
function drawChips(){
 let h='<button class="n" id="c0" onclick="pick(0)">ALL</button>';
 for(let i=1;i<=6;i++)h+='<button class="n" id="c'+i+'" onclick="pick('+i+')">S'+i+'</button>';
 chips.innerHTML=h;markSel();}
function markSel(){for(let i=0;i<=6;i++){let b=document.getElementById('c'+i);
 if(b)b.className=i==sel?'n sel':'n';}}
function pick(i){sel=i;markSel()}
function led(c){fetch('/led?s='+sel+'&c='+c).then(tick)}
function fmt(n,d){return n==null?'--':Number(n).toFixed(d)}
function tick(){fetch('/api/data').then(r=>r.json()).then(j=>{
 document.getElementById('ledmode').textContent=j.mode;
 let h='';
 for(const s of j.stations){
  h+='<tr><td>S'+s.i+'</td><td>'+s.name+'</td>'+
   '<td><span class="sw" style="background:#'+s.led+'"></span> #'+s.led+'</td>';
  if(s.dht.ok)h+='<td class="v">'+fmt(s.dht.t,1)+' / '+fmt(s.dht.h,0)+'</td>';
  else h+='<td>-- / --</td>';
  h+='<td class="v">'+s.mq.raw+'</td>'+
   '<td class="v">'+fmt(s.mq.v,2)+' / '+fmt(s.mq.ao,2)+'</td>';
  if(s.prox=='DETECTED')h+='<td class="bad">DETECTED</td>';
  else h+='<td class="ok">CLEAR</td>';
  h+='</tr>';}
 document.getElementById('rows').innerHTML=h;
 let sec=Math.floor(j.up_ms/1000);
 document.getElementById('up').textContent=Math.floor(sec/60)+'m '+(sec%60)+'s';
 document.getElementById('cl').textContent=j.clients;
}).catch(()=>{})}
drawChips();setInterval(tick,1000);tick();
</script></body></html>)rawliteral";
