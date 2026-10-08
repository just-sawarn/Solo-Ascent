#!/usr/bin/env python3 -I
"""Fails if a file under assets/models, assets/textures or assets/audio is not covered by assets/LICENSES.json."""
import json, os, sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")
lic = json.load(open(os.path.join(root, "LICENSES.json")))
ALLOWED = {"CC0-1.0", "MIT", "CC-BY-4.0", "Apache-2.0", "Zlib"}
bad = [k for k, v in lic.items() if v.get("license") not in ALLOWED or not v.get("source")]
missing = []
for sub in ("models", "textures", "audio"):
    base = os.path.join(root, sub)
    for dp, _, files in os.walk(base):
        for f in files:
            rel = os.path.relpath(os.path.join(dp, f), os.path.join(root, ".."))
            if not any(rel.startswith(k + "/") for k in lic):
                missing.append(rel)
for k in bad: print("bad license entry:", k)
for m in missing: print("no license entry:", m)
print("OK" if not bad and not missing else "FAILED")
sys.exit(1 if bad or missing else 0)
