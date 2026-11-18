"""Provision a local OTA signing key and embed its public half in the build."""

import os
from pathlib import Path
import subprocess
import tempfile

Import("env")

project = Path(env.subst("$PROJECT_DIR"))
key = project / ".ota-signing.key"
if not key.exists():
    with tempfile.NamedTemporaryFile(dir=project) as temporary:
        subprocess.run(
            ["openssl", "genpkey", "-algorithm", "EC", "-pkeyopt",
             "ec_paramgen_curve:prime256v1", "-out", temporary.name], check=True
        )
        try:
            os.link(temporary.name, key)
            print(f"[{project.name}] Created local OTA signing key .ota-signing.key")
        except FileExistsError:
            pass  # A parallel build already provisioned the shared key.

public_key = subprocess.check_output(["openssl", "pkey", "-in", str(key), "-pubout"], text=True)
output = Path(env.subst("$BUILD_DIR")) / "generated"
output.mkdir(parents=True, exist_ok=True)
header = output / "OtaPublicKey.h"
content = '#pragma once\nstatic constexpr char OTA_PUBLIC_KEY[] = R"PEM(' + public_key + ')PEM";\n'
if not header.exists() or header.read_text() != content:
    header.write_text(content)
env.AppendUnique(CPPPATH=[str(output)])
