"""Compile this Hub's offline dashboard into the ESP32 application image.

PlatformIO pre-build script; can also run directly to inspect/test the bundle.
No frontend server, npm, cloud asset, or separate filesystem upload is needed.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

MIME = {
    ".html": "text/html; charset=utf-8", ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8", ".svg": "image/svg+xml",
    ".png": "image/png", ".jpg": "image/jpeg", ".woff2": "font/woff2",
}


def bundle(project, output):
    source = Path(project) / "data"
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    header = ["#pragma once", "#include <Arduino.h>",
              "struct DashboardAsset { const char* path; const char* mime; const uint8_t* data; size_t size; };"]
    assets, manifest = [], []
    for file in sorted(source.rglob("*")):
        if not file.is_file() or file.suffix not in MIME:
            continue
        url = "/" + file.relative_to(source).as_posix()
        raw = file.read_bytes()
        compressed = gzip.compress(raw, compresslevel=9, mtime=0)
        name = "dashboard_asset_" + str(len(assets))
        header.append("static const uint8_t " + name + "[] PROGMEM = {")
        for start in range(0, len(compressed), 24):
            header.append(",".join(str(v) for v in compressed[start:start + 24]) + ",")
        header.append("};")
        assets.append(f'{{{json.dumps(url)}, {json.dumps(MIME[file.suffix])}, {name}, sizeof({name})}}')
        manifest.append({"path": url, "raw": len(raw), "gzip": len(compressed),
                         "sha256": hashlib.sha256(raw).hexdigest()})
    if not any(item["path"] == "/index.html" for item in manifest):
        raise ValueError("Dashboard index.html is missing")
    header += ["static const DashboardAsset DASHBOARD_ASSETS[] = {",
               ",\n".join(assets), "};",
               "static const size_t DASHBOARD_ASSET_COUNT = sizeof(DASHBOARD_ASSETS) / sizeof(DASHBOARD_ASSETS[0]);"]
    target = output / "DashboardAssets.h"
    content = "\n".join(header) + "\n"
    if not target.exists() or target.read_text() != content:
        target.write_text(content)
    (output / "dashboard-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"[NOV_HUB] Embedded {len(assets)} dashboard assets, {sum(x['gzip'] for x in manifest):,} gzip bytes; no uploadfs needed.")
    return manifest


try:
    Import("env")
except NameError:
    env = None

if env is not None:
    directory = Path(env.subst("$BUILD_DIR")) / "generated"
    bundle(env.subst("$PROJECT_DIR"), directory)
    env.Append(CPPPATH=[str(directory)])
elif __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=Path(__file__).resolve().parents[1] / ".pio/dashboard-check")
    args = parser.parse_args()
    bundle(Path(__file__).resolve().parents[1], args.out)
