"""Host tests for the actual NOV_HUB sensor implementation and JSON emitters.

Run: python3 firmware/NOV_HUB/tests/test_profile.py
No physical device, serial port, frontend server, or gas exposure is involved.
"""
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]


def function(source, name):
    start = source.index(name)
    # Caller supplies a complete declaration to avoid matching a call site.
    body = source.index("{", start)
    depth = 0
    # These simple emitters have string literals containing braces; lexical
    # brace scanning must ignore quoted C++ strings/comments.
    quoted = False
    escaped = False
    for end in range(body, len(source)):
        c = source[end]
        if quoted:
            if escaped: escaped = False
            elif c == "\\": escaped = True
            elif c == '"': quoted = False
            continue
        if c == '"': quoted = True; continue
        if c == "{": depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0: return source[start:end + 1]
    raise AssertionError(name)


class ProfileTests(unittest.TestCase):
    def test_original_hub_unchanged(self):
        baseline = json.loads((ROOT / "ORIGINAL_HUB_SHA256.json").read_text())
        for relative, expected in baseline.items():
            self.assertEqual(hashlib.sha256((REPO / "firmware/hub" / relative).read_bytes()).hexdigest(), expected, relative)

    def test_sensor_provenance_and_actual_json(self):
        main = (ROOT / "src/main.cpp").read_text()
        declarations = ["const char* stationName(uint8_t stationId)",
                        "HealthRow* healthRowLocked(uint8_t stationId)",
                        "String sensorsJsonLocked()", "String catalogJson()",
                        "String demoCameraImageJson(uint32_t now)", "String demoCameraGalleryJson(uint32_t now)",
                        "String stateJson()",
                        "uint32_t hueWheel(uint8_t wheelPos)", "uint32_t scaleColor(uint32_t c, uint8_t scale)",
                        "void updateLeds()"]
        chunks = "\n\n".join(function(main, name) for name in declarations)
        with tempfile.TemporaryDirectory(prefix="nov-host-") as temp:
            folder = Path(temp)
            harness = r'''
#include <Arduino.h>
#include <cstring>
#include <iostream>
#include <math.h>
#include "MuseumState.h"
#include "HubSensors.h"
#include "CameraDemo.h"
uint32_t fakeMillis = 0;
int fakeLevels[64];
int fakeAdc[64];
float fakeTemps[64], fakeHumidity[64];
SerialStub Serial;
MuseumState gState;
SemaphoreHandle_t gStateMutex = nullptr;
void stateLock() {} void stateUnlock() {}
uint8_t pendingCountLocked() { return 0; }
struct Photos { int count(){return 0;} const char* backend(){return "none";} } gPhotos;
struct PixelStub {
  uint32_t data[LED_CHAIN_LENGTH] = {};
  int numPixels(){return LED_CHAIN_LENGTH;}
  uint32_t Color(int r,int g,int b){return (r<<16)|(g<<8)|b;}
  void clear(){for(auto &p:data)p=0;}
  void setPixelColor(unsigned index,uint32_t c){assert(index<LED_CHAIN_LENGTH);data[index]=c;}
  void show(){}
} pixels;
DemoCameraClock gDemoCamera;
'''
            harness += chunks
            harness += r'''
int main() {
  DemoCameraClock clock;
  assert(clock.sequenceAt(0)==1 && clock.sequenceAt(19999)==1);
  assert(clock.sequenceAt(20000)==2 && clock.sequenceAt(39999)==2);
  assert(clock.sequenceAt(100000)==6 && clock.sequenceAt(120000)==7);
  assert(DemoCameraClock::photoIndex(7)==0 && clock.countAt(120000)==6);
  assert(clock.ageAt(20000,2)==0 && clock.ageAt(20000,1)==20000);
  clock.restart(25000);assert(clock.countAt(25000)==1);
  assert(DemoCameraClock::photoIndex(clock.sequenceAt(25000))==0);
  std::cout<<"GALLERY "<<demoCameraGalleryJson(20000).c_str()<<"\n";
  for (int i=0;i<64;i++) {fakeLevels[i]=HIGH;fakeTemps[i]=NAN;fakeHumidity[i]=NAN;fakeAdc[i]=i*100;}
  assert(STATION_COUNT==6 && REAL_STATION_COUNT==6 && SIMULATED_STATION_COUNT==0);
  assert(LED_CHAIN_LENGTH==48 && LED_PIXELS_PER_STATION==8 && NEOPIXEL_PIN==45);
  assert(BUZZER_PIN==47 && HUB_SD_CS_PIN==14);
  assert(!HUB_GAS_CALIBRATED && stationName(7)[0]==0);
  sensorsBegin();
  assert(gState.healthCount==6);
  for (const auto& row : gState.health) assert(!row.hasTH && !row.hasGas && !row.hasProx);
  fakeTemps[15]=26.4f;fakeHumidity[15]=61.0f;
  fakeTemps[38]=28.5f;fakeHumidity[38]=55.5f;
  fakeTemps[39]=29.0f;fakeHumidity[39]=57.0f;
  fakeLevels[42]=LOW;
  for(fakeMillis=100;fakeMillis<=5000;fakeMillis+=100) sensorsUpdate();
  assert(gState.health[0].hasTH && gState.health[0].tempX10==264);
  assert(!gState.health[1].hasTH && gState.health[4].tempX10==285 && gState.health[5].tempX10==290);
  for(int i=0;i<6;i++) {
    assert(gState.sensors[i].tryCount==1);
    assert(gState.health[i].hasGas && gState.health[i].hasProx && gState.health[i].proxShared);
    assert(gState.health[i].prox==(i==2 || i==3));
  }
  std::cout<<"STATE "<<stateJson().c_str()<<"\n";
  std::cout<<"CARDS "<<catalogJson().c_str()<<"\n";
  fakeLevels[42]=HIGH;
  fakeTemps[15]=NAN;
  fakeMillis=5500;sensorsUpdate();
  assert(!gState.health[0].hasTH && gState.sensors[0].dht==DHT_NO_RESPONSE && gState.sensors[0].okCount==1);
  fakeMillis=5530;sensorsUpdate();
  std::cout<<"RECENT "<<stateJson().c_str()<<"\n";
  fakeMillis+=HUB_PROX_RECENT_MS;
  std::cout<<"LATER "<<stateJson().c_str()<<"\n";
  assert(!gState.health[4].hasFreshTemperature(17000));
  for (uint8_t zone=0; zone<HUB_PROX_ZONE_COUNT; zone++) {
    for (uint8_t input=0; input<HUB_PROX_ZONE_COUNT; input++)
      fakeLevels[HUB_PROX_ZONE_PINS[input]]=(input==zone ? LOW : HIGH);
    fakeMillis+=100;
    sensorsUpdate();
    fakeMillis+=30;
    sensorsUpdate();
    for (int station=0; station<STATION_COUNT; station++)
      assert(gState.health[station].hasProx && gState.health[station].prox==(station/2==zone));
  }
  // The last active pair blinks without obscuring S1's robot position.
  strcpy(gState.robotStation, STATION_IDS[0]);
  updateLeds();
  for (int i=0;i<LED_CHAIN_LENGTH;i++)
    assert(pixels.data[i]==(i<8 ? 0x00C800U : i>=32 ? 0xB44100U : 0U));
  fakeMillis+=LED_PROX_HALF_PERIOD_MS;
  updateLeds();
  for (int i=0;i<LED_CHAIN_LENGTH;i++) assert(pixels.data[i]==(i<8 ? 0x00C800U : 0U));
  for (uint8_t pin : HUB_PROX_ZONE_PINS) fakeLevels[pin]=HIGH;
  sensorsUpdate();
  fakeMillis+=30;
  sensorsUpdate();

  // Mixed MQ signal directions become established independently after stable samples.
  fakeAdc[HUB_MQ_PINS[0]]=3450;
  fakeAdc[HUB_MQ_PINS[1]]=22;
  for (int sample=0;sample<14;sample++) { fakeMillis+=1000; sensorsUpdate(); }
  assert(gState.health[0].gasStatus==MqStatus::Normal && gState.health[1].gasStatus==MqStatus::Normal);
  fakeAdc[HUB_MQ_PINS[0]]=2200; fakeAdc[HUB_MQ_PINS[1]]=1200;
  fakeMillis+=1000; sensorsUpdate();
  assert(gState.health[0].gasStatus==MqStatus::Elevated && gState.health[1].gasStatus==MqStatus::Elevated);
  fakeAdc[HUB_MQ_PINS[0]]=90; fakeAdc[HUB_MQ_PINS[1]]=3100;
  fakeMillis+=1000; sensorsUpdate();
  assert(gState.health[0].gasStatus==MqStatus::High && gState.health[1].gasStatus==MqStatus::High);
  std::cout<<"GAS "<<stateJson().c_str()<<"\n";
  for (uint8_t pin : HUB_MQ_PINS) fakeAdc[pin]=0;
  for (uint8_t pin : HUB_PROX_ZONE_PINS) fakeLevels[pin]=LOW;
  fakeMillis+=1000;sensorsUpdate();
  for (const auto& row : gState.health) assert(!row.hasGas && !row.hasProx && !row.prox);
  fakeAdc[HUB_MQ_PINS[0]]=3450;
  for (uint8_t pin : HUB_PROX_ZONE_PINS) fakeLevels[pin]=HIGH;
  fakeMillis+=1000;sensorsUpdate();
  assert(gState.health[0].hasGas && gState.health[0].gasStatus==MqStatus::Learning);
  for (int station=0; station<STATION_COUNT; station++) {
    strcpy(gState.robotStation, STATION_IDS[station]);
    for (bool moving : {false, true}) {
      gState.robotMoving=moving;
      updateLeds();
      for (int i=0; i<LED_CHAIN_LENGTH; i++) {
        const uint32_t colour = moving ? 0x0000FFU : 0x00C800U;
        assert(pixels.data[i]==(i/LED_PIXELS_PER_STATION==station ? colour : 0U));
      }
    }
  }
  // The emergency pattern overrides the last station across both red flashes.
  gState.emergency=true;
  const uint32_t red=(LED_EMERGENCY_BRIGHTNESS*255/LED_BRIGHTNESS)<<16;
  for (const uint32_t offset : {0U, 160U, 350U, 500U}) {
    fakeMillis=30000+offset;
    updateLeds();
    for (auto colour : pixels.data) assert(colour==((offset==0 || offset==350) ? red : 0U));
  }
  gState.emergency=false;
  updateLeds();
  for (int i=0; i<LED_CHAIN_LENGTH; i++) assert(pixels.data[i]==(i>=40 ? 0x0000FFU : 0U));

  // A complete idle sweep visits every pixel, including the last board.
  gState.robotStation[0]=0;
  bool visited[LED_CHAIN_LENGTH]={};
  for (int step=0; step<LED_CHAIN_LENGTH; step++) {
    fakeMillis+=LED_FLOW_STEP_MS;
    updateLeds();
    for (int i=0; i<LED_CHAIN_LENGTH; i++) visited[i] |= pixels.data[i]!=0;
  }
  for (bool lit : visited) assert(lit);
}
'''
            (folder / "check.cpp").write_text(harness)
            subprocess.run(["g++", "-std=c++11", "-I" + str(ROOT / "tests/fakes"),
                            "-I" + str(ROOT / "include"), str(folder / "check.cpp"),
                            str(ROOT / "src/HubSensors.cpp"), "-o", str(folder / "check")], check=True)
            output = subprocess.check_output([str(folder / "check")], text=True)
        state = json.loads(next(line[6:] for line in output.splitlines() if line.startswith("STATE ")))
        cards = json.loads(next(line[6:] for line in output.splitlines() if line.startswith("CARDS ")))
        gallery = json.loads(next(line[8:] for line in output.splitlines() if line.startswith("GALLERY ")))
        recent = json.loads(next(line[7:] for line in output.splitlines() if line.startswith("RECENT ")))
        later = json.loads(next(line[6:] for line in output.splitlines() if line.startswith("LATER ")))
        self.assertTrue(gallery["simulated"])
        self.assertEqual(gallery["intervalMs"], 20000)
        self.assertEqual([p["index"] for p in gallery["photos"]], [2, 1])
        self.assertEqual([p["age"] for p in gallery["photos"]], [0, 20000])
        self.assertFalse(state["image"]["simulated"])
        self.assertEqual(state["image"]["seq"], 0)
        self.assertEqual(len(state["health"]), 6)
        self.assertTrue(all(h["source"] == "real" and not h["simulated"] for h in state["health"]))
        self.assertEqual([h["temp"] for h in state["health"]], [26.4, 26.9, 27.4, 27.9, 28.5, 29.0])
        self.assertEqual([h["hum"] for h in state["health"]], [61.0, 59.6, 58.2, 56.8, 55.5, 57.0])
        self.assertEqual([h["thSource"] for h in state["health"]], ["measured", "estimated", "estimated", "estimated", "measured", "measured"])
        self.assertEqual([h["gas"] for h in state["health"]], [400, 500, 600, 700, 100, 200])
        self.assertTrue(all(h["gasOk"] is None for h in state["health"]))
        gas = json.loads(next(line[4:] for line in output.splitlines() if line.startswith("GAS ")))
        self.assertEqual([row["gasStatus"] for row in gas["health"][:2]], ["high", "high"])
        self.assertEqual([h["prox"] for h in state["health"]], [False, False, True, True, False, False])
        self.assertTrue(all(h["prox"] is False for h in recent["health"]))
        self.assertEqual([h["proxRecent"] for h in recent["health"]], [False, False, True, True, False, False])
        self.assertFalse(any(h["proxRecent"] for h in later["health"]))
        self.assertEqual([p["pin"] for p in state["sensors"]], [15, 16, 17, 18, 38, 39, 4, 5, 6, 7, 1, 2])
        self.assertNotIn("temp", state["sensors"][1])
        self.assertEqual(state["sensors"][6]["sensorMv"], 336)
        self.assertEqual(state["profile"]["realStations"], 6)
        self.assertEqual(state["profile"]["simulatedStations"], 0)
        expected = ["83 54 6A 28", "64 14 F6 B3", "4A 6B DF 34", "D8 36 D4 B9", "94 60 6B B2",
                    "18 48 F4 BA", "94 58 1A B3", "09 4C B1 92", "13 8C 5A 28"]
        self.assertEqual([c["uid"] for c in cards["cards"]], expected)
        self.assertFalse(cards["cards"][7]["active"])
        self.assertEqual(cards["cards"][-1]["role"], "end")
        # The dashboard's active order/UIDs must match the actual C++ catalog.
        script = "const fs=require('fs'),vm=require('vm');const c={};vm.createContext(c);vm.runInContext(fs.readFileSync(process.argv[1],'utf8')+';this.rows=ARTIFACTS',c);console.log(JSON.stringify(c.rows));"
        ui = json.loads(subprocess.check_output(["node", "-e", script, str(ROOT / "data/js/data.js")], text=True))
        self.assertEqual([(a["id"], a["uid"], a["simulated"]) for a in ui],
                         [(c["id"], c["uid"], c["simulated"]) for c in cards["cards"] if c["role"] == "station"])
        # Persist realistic API fixtures for optional browser-only tests.
        out = ROOT / ".pio/test-fixtures"
        out.mkdir(parents=True, exist_ok=True)
        (out / "state.json").write_text(json.dumps(state, indent=2))

    def test_embedded_assets_are_reproducible(self):
        spec = importlib.util.spec_from_file_location("embed", ROOT / "tools/embed_dashboard.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as temp:
            manifest = module.bundle(ROOT, temp)
            header = (Path(temp) / "DashboardAssets.h").read_text()
            compressed = re.findall(r"\[\] PROGMEM = \{\n(.*?)\n\};", header, re.S)
            self.assertEqual(len(compressed), len(manifest))
            for item, data in zip(manifest, compressed):
                decoded = gzip.decompress(bytes(map(int, data.replace("\n", "").strip(",").split(","))))
                self.assertEqual(decoded, (ROOT / "data" / item["path"].lstrip("/")).read_bytes())
            self.assertIn('/admin.html', [m["path"] for m in manifest])
            self.assertIn('/js/vendor/jsQR.js', [m["path"] for m in manifest])
            self.assertEqual((Path(temp) / "DashboardAssets.h").read_text(), header)


if __name__ == "__main__":
    unittest.main(verbosity=2)
