# AI-Agent PCB Design → JLCPCB PCBA: Research & Recommended Workflow

> Researched 2026-10-11 using live web search + repo analysis (MCP tools: ddg-search, zread, local file inspection).

## TL;DR Recommendation

**Use EasyEDA Pro + JLC's official `easyeda/easyeda-api-skill` bridge as the primary AI workflow.**
It is the only path where **LCSC part numbers are native to every placed component**, and the
one-click **"Order at JLCPCB"** button packages Gerber + BOM + CPL automatically — closing the
prompt → design → PCBA order loop with zero file-stitching.

Keep the already-installed **KiCad MCP server (`mixelpixx/KiCAD-MCP-Server`) as the secondary /
power-user path** (it is currently `disabled` in your MCP settings), paired with Bouni's
`kicad-jlcpcb-tools` for the final BOM/CPL generation.

---

## 1. How JLCPCB PCBA ordering actually works (the contract you must satisfy)

JLCPCB needs exactly **3 file sets** for a turnkey SMT assembly order:

| File | What it tells them | Required content |
|---|---|---|
| **Gerber + drill** (RS-274X, zipped) | The physical board (copper, silks, mask, edges, holes) | Standard Gerber export |
| **BOM** (CSV/XLSX) | *Which* real component to load on the pick-and-place machine | Per line: **LCSC Part #** (e.g. `C25804`), Comment/Value, Designators (refs), Footprint |
| **CPL / Pick & Place** (CSV) | *Where* each component goes and its orientation | Per row: Designator, Mid X, Mid Y, Layer (top/bottom), Rotation |

Their SMT line cross-matches **Designators** between BOM and CPL, loads the physical reel that the
**LCSC Part #** resolves to, and solders it at the CPL coordinates. That is literally "how they know
which component gets soldered where."

- **Basic parts** → free assembly; **Extended parts** → ~$3 setup fee per unique part number.
- So an AI workflow must guarantee: *every symbol/footprint placed by the agent carries a real,
  in-stock LCSC part number that survives into the exported BOM.*

---

## 2. Option A (RECOMMENDED): EasyEDA Pro + official API Skill

### What it is
- **`easyeda/easyeda-api-skill`** (github.com/easyeda/easyeda-api-skill) — official, open-source,
  maintained by EasyEDA/JLC themselves. JLCPCB's own blog documents this as *the* AI PCB workflow.
- Architecture: AI agent → **local Node bridge server** (HTTP/WS, default port 49620) →
  **`run-api-gateway` extension** inside EasyEDA Pro executes the JS against the live editor.
  Local-first; every operation is inspectable JS.
- Agent plans → searches EasyEDA's **real component library** (symbols/footprints are real UUIDs,
  not hallucinated) → places, wires, net-flags, auto-routes, runs ERC → refines iteratively.

### Why it wins for JLCPCB specifically
1. EasyEDA Pro's library **is the LCSC/JLCPCB parts library** — every placed part already carries
   its LCSC Part #, price and stock.
2. **Fabrication → Order PCB/PCBA** exports Gerber + BOM (with LCSC #) + CPL in JLCPCB's exact
   format, or uploads directly via the built-in "Order at JLCPCB" flow.
3. DRC rule templates preset to JLCPCB's fab capabilities.
4. Free, ~10-minute setup, Windows supported (you're on Win10).

### Setup (≈10 min)
1. Install **EasyEDA Pro** (easyeda.com/page/download).
2. In EasyEDA Pro: Advanced → Extensions Manager → install **`run-api-gateway.eext`**
   (from ext.lceda.cn/item/oshwhub/run-api-gateway), enable it, and in its Config turn on
   *"Allow interact with external"* + *"Show at header menu"*.
3. `git clone https://github.com/easyeda/easyeda-api-skill && npm install && npm run build:docs && npm run server`
4. In EasyEDA Pro: API Gateway menu → **Reconnect** → "Bridge connected (port 49620)".
5. Point your agent at the skill docs (SKILL.md). It's a standard **Agent Skills** package
   (Claude Code/Codex/QwenCode load it from their skills dir). From **Zoo Code**, the agent can
   drive the bridge directly over HTTP POST to `localhost:49620` — no special client needed.

### MCP-native alternative on the same engine
- **`zhoushoujianwork/easyeda-agent`** — Go CLI + **stdio MCP server** + skill bundle using the
  same `eda.*` API via an in-EDA connector plugin: reads datasheets, builds component libraries,
  places/routes schematic & PCB, silk adjustments, DRC. Drop-in as an MCP server entry in Zoo Code.
- **`oaslananka/easyeda-mcp-pro`** — production-grade MCP server (34–119 profile-gated tools):
  schematic inspection/edit, DRC, BOM sourcing, fab export.
- **`Spectoda/easyeda-mcp`** — MCP + bridge focused on inspect/review + careful automation.
- These are younger than the official skill; treat as conveniences on top of the same gateway idea.

---

## 3. Option B: KiCad + your local KiCAD-MCP-Server + kicad-jlcpcb-tools

### What you already have
`D:\projects\PCB projects\KiCAD-MCP-Server` = **mixelpixx/KiCAD-MCP-Server** v2.1/2.2-line,
currently **disabled** in both Roo and Zoo MCP configs. Verified locally:

- **122 tools / 16 categories**: create project (`.kicad_sch` + `.kicad_pcb`), place ~10,000
  KiCad symbols via dynamic loading, intelligent wiring (pin discovery, orthogonal routing,
  power flags, net labels), board outline, footprint creation, **Freerouting autorouter**,
  DRC, snapshots.
- **JLCPCB parts integration** (verified in [`JLCPCB_USAGE_GUIDE.md`](../../PCB%20projects/KiCAD-MCP-Server/docs/JLCPCB_USAGE_GUIDE.md)):
  - `search_jlcpcb_parts` — parametric search (value/package/tolerance), Basic-vs-Extended flags,
    price breaks, stock, `in_stock` filter
  - `get_jlcpcb_part` — full detail by LCSC # (e.g. C58972 → 0603WAF1002T5E, Basic, $0.001/ea)
  - `suggest_jlcpcb_alternatives` — cheaper/in-stock/Basic substitutes
  - `download_jlcpcb_database` — local SQLite (full ~2.5M parts via JLCSearch, no API key;
    or official API w/ enterprise keys)
  - Package→KiCad-footprint mapping (0603 → `Resistor_SMD:R_0603_1608Metric` etc.)
- **Exports**: `export_gerber`, `export_bom` (CSV/XML/HTML/JSON), `export_position_file`
  (pick & place), netlist, 3D.

### The one gap (verified in `python/commands/export.py`)
`export_bom` emits Reference/Value/Footprint/Layer — **no LCSC # column** (the JLCPCB-columns
upgrade exists only in `docs/archive/JLCPCB_INTEGRATION_PLAN.md`, not implemented).

### Fix for the gap — Bouni's `kicad-jlcpcb-tools`
Battle-tested KiCad PCB Editor plugin (the community standard):
- Parts browser to **assign LCSC part numbers** to footprints (stored as a field),
- One-click **Generate BOM + CPL + Gerber zip in JLCPCB's exact expected format**.
- Pair it with `uPCompute/easyeda2kicad` to import any LCSC part (symbol+footprint+3D) into KiCad
  when the MCP's footprint suggestion isn't enough.

### When to prefer Option B
- Complex boards (4+ layers, dense routing, real copper zones — JLCPCB's own GPT-6 Astra
  comparison noted KiCad keeps native refillable zones vs EasyEDA's fixed fill regions).
- You want open file formats and full offline reproducibility.
- You already have it installed — enabling is 2 clicks in MCP settings + KiCad + Python deps.

---

## 4. Comparison

| Criterion | A: EasyEDA Pro + official API skill | B: KiCad + mixelpixx MCP + jlcpcb-tools |
|---|---|---|
| JLCPCB part sync | ★★★ native LCSC library, stock/pricing in-editor | ★★ via JLCSearch DB + PCM libraries |
| BOM with LCSC # | ★★★ automatic (one-click export/order) | ★★ needs kicad-jlcpcb-tools or field discipline |
| CPL/pick&place | ★★★ built-in export | ★★★ plugin / `export_position_file` |
| AI maturity | ★★★ official skill, JLC-documented | ★★ 122 tools but young project (v2.1-alpha line) |
| Autorouting | ★★ basic built-in | ★★★ Freerouting integration |
| Advanced layout | ★★ fixed fill regions | ★★★ zones, open EDA, kicad-cli/KiBot ecosystem |
| Setup effort | ★★★ ~10 min, free | ★★ already installed; needs KiCad + plugin pairing |
| Lock-in | EasyEDA project format | Open KiCad files |

---

## 5. Recommended action plan

1. **Primary**: set up Option A (EasyEDA Pro + `easyeda-api-skill` bridge). For PCBA prototypes this
   is the shortest, most reliable loop: prompt → real LCSC parts → verify ERC/DRC in-editor →
   one-click Order at JLCPCB.
2. In Zoo Code, optionally add `easyeda-agent` as an MCP server for typed actions instead of raw JS.
3. **Secondary**: re-enable `kicad` MCP (flip `disabled: false` in
   `C:\Users\tahfi\AppData\Roaming\Code\User\globalStorage\zoocodeorganization.zoo-code\settings\mcp_settings.json`),
   run `download_jlcpcb_database`, install `kicad-jlcpcb-tools` — use for complex boards.
4. Always do a human review pass (ERC/DRC, footprint-vs-part sanity) before ordering — agents
   still make pin-association mistakes (JLC's own GPT-6 Astra test surfaced ~50 mismatches).

## 6. Cost breakdown (Option A: EasyEDA Pro + official AI skill)

### Free ($0) — the entire toolchain
| Component | Cost | Note |
|---|---|---|
| EasyEDA Pro desktop | **$0** | Official pricing page: "core features are absolutely free for everyone" |
| `run-api-gateway` extension | **$0** | Free from JLC extension marketplace (ext.lceda.cn) |
| `easyeda-api-skill` + bridge | **$0** | Public repo, runs locally on Node.js; no auth/keys |
| LCSC parts library data | **$0** | Browsing/placing parts with LCSC #, stock, pricing is free |
| Community MCP wrappers | **$0** | easyeda-agent, easyeda-mcp-pro are open-source |

### NOT free — the real costs
| Item | Typical cost |
|---|---|
| **The AI agent itself** | Model/API usage — e.g. this Zoo session, or Claude Code subscription. Usually the largest recurring cost of "AI PCB design" |
| JLCPCB fabrication | ~$2–8 promo for 2-layer prototypes (higher for 4-layer) |
| PCBA setup | Basic parts: free assembly; **Extended parts ≈ $3 per unique part #**; stencil if needed |
| Components | LCSC unit prices × quantity (shown live in the parts library) |
| Shipping to BD | Varies by courier |

### Optional (not required)
- EasyEDA paid plan (a few $/month): only buys extras like more cloud storage / private projects.
  The AI workflow runs fine fully local — a free account login is enough for library access.

## 7. Setup status on this PC (2026-10-11) — DONE except EasyEDA Pro itself

| Item | State |
| --- | --- |
| Node.js v24.19 / npm 11 / git | ✅ already present |
| `D:\projects\easyeda-api-skill` (cloned, deps, v1.1.44) | ✅ bridge running (`npm run server`, port 49620) |
| Bridge health | ✅ `{"service":"easyeda-bridge","status":"ok"}` |
| Skill wired for Claude Code | ✅ junction `C:\Users\tahfi\.claude\skills\easyeda-api-skill` → repo. The VS Code Claude Code **extension** reads this same skills dir — the `claude` CLI (also installed, v2.1.296) is optional |
| Zoo Code integration | ✅ [`tools/easyeda-bridge/eda.mjs`](../tools/easyeda-bridge/eda.mjs) helper + workspace rule `.roo/rules/easyeda-bridge.md` |
| EasyEDA Pro installer | ✅ `C:\Users\tahfi\Desktop\easyeda-setup\easyeda-pro-windows-x64-3.2.149.exe` (279 MB) |
| Gateway extension | ✅ installed & enabled in EasyEDA Pro (v1.0.5, github.com/easyeda/eext-run-api-gateway, Apache-2.0) |
| **End-to-end link** | ✅ **LIVE & VERIFIED 2026-10-11** — `edaConnected: true`, execute round-trip works, live `eda` API surface introspected (`sch_*`, `pcb_*`, `lib_*`, `dmt_*`) |

**Remaining manual steps (GUI only):**

1. Run the installer from the Desktop folder above.
2. EasyEDA Pro → Advanced → Extensions Manager → import the `.eext` → enable it → in its Config tick
   **Allow interact with external** and **Show at header menu**. (Or: Extensions Manager → search
   "Run API Gateway" → install v1.0.6 directly from the marketplace.)
3. Top menu **API Gateway → Reconnect**.
4. Verify from VS Code: `node tools/easyeda-bridge/eda.mjs health` → expect `edaConnected: true`.
5. Then just prompt here (Zoo Code) or in the Claude Code panel: e.g. *"check my schematic"* or
   *"draw an NE555 minimal system"*.

## Sources
- JLCPCB official blog: *AI PCB Design: From Prompt to PCB with EasyEDA Pro* (jlcpcb.com/blog/ai-pcb-design-easyeda)
- github.com/easyeda/easyeda-api-skill (+ ext.lceda.cn/item/oshwhub/run-api-gateway)
- Local: `D:\projects\PCB projects\KiCAD-MCP-Server` (mixelpixx) — README, JLCPCB_USAGE_GUIDE.md, export.py
- github.com/Bouni/kicad-jlcpcb-tools; github.com/uPCompute/easyeda2kicad
- github.com/zhoushoujianwork/easyeda-agent; github.com/oaslananka/easyeda-mcp-pro; github.com/Spectoda/easyeda-mcp
- JLCPCB Help: Bill of Materials for PCB Assembly; PCBA Files Preparation (BOM/CPL formats)
