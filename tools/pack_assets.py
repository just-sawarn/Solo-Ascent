#!/usr/bin/env python3 -I
"""Build streamable asset packs and stamp the web build.

  python3 -I tools/pack_assets.py <repo-root> <web-build-dir>

Writes <build>/packs/<name>.<hash8>.pak (content-addressed, so immutable and cache-forever),
<build>/packs.json (manifest: size + sha256 per pack, `boot` packs are fetched before the game
starts) and stamps <build>/sw.js with a build id so every new build installs a fresh service worker.

Pack format (little endian): "SAPK" u32 version(1) u32 count, then per file
  u16 pathlen, path (utf-8, absolute in the virtual FS), u32 size, bytes
"""
import hashlib, json, os, struct, sys

PACKS = {
    # name: (boot?, [(source dir relative to repo, virtual mount point)])
    "characters": (True, [("assets/models/characters", "/assets/characters")]),
    "animations": (True, [("assets/models/animations", "/assets/animations")]),
    "weapons":    (True, [("assets/models/weapons", "/assets/weapons")]),
}
# a tiny extra pack that exists only so tests can exercise on-demand streaming
SELFTEST = ("selftest", False, b"streamed pack ok\n", "/packs/selftest.txt")


def build_pack(files):
    out = bytearray(b"SAPK" + struct.pack("<II", 1, len(files)))
    for path, data in files:
        p = path.encode()
        out += struct.pack("<H", len(p)) + p + struct.pack("<I", len(data)) + data
    return bytes(out)


def collect(root, src, mount):
    base = os.path.join(root, src)
    items = []
    for dp, _, fns in os.walk(base):
        for fn in sorted(fns):
            if fn.startswith("."):
                continue
            full = os.path.join(dp, fn)
            rel = os.path.relpath(full, base).replace(os.sep, "/")
            with open(full, "rb") as f:
                items.append((mount + "/" + rel, f.read()))
    return sorted(items)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def main():
    root, build = sys.argv[1], sys.argv[2]
    pdir = os.path.join(build, "packs")
    os.makedirs(pdir, exist_ok=True)
    for f in os.listdir(pdir):
        os.remove(os.path.join(pdir, f))
    manifest = {"version": 1, "packs": {}}
    jobs = [(n, boot, sum((collect(root, s, m) for s, m in srcs), [])) for n, (boot, srcs) in PACKS.items()]
    jobs.append((SELFTEST[0], SELFTEST[1], [(SELFTEST[3], SELFTEST[2])]))
    for name, boot, files in jobs:
        blob = build_pack(files)
        h = sha(blob)
        fn = f"{name}.{h[:8]}.pak"
        with open(os.path.join(pdir, fn), "wb") as f:
            f.write(blob)
        manifest["packs"][name] = {"file": "packs/" + fn, "size": len(blob), "sha256": h, "files": len(files), "boot": boot}
    mpath = os.path.join(build, "packs.json")
    with open(mpath, "w") as f:
        json.dump(manifest, f, indent=1, sort_keys=True)
    # build id = hash of every file the page loads
    bid = hashlib.sha256()
    for n in ["index.html", "index.js", "index.wasm", "index.data", "glue.js", "game.css", "packs.json"]:
        p = os.path.join(build, n)
        if os.path.exists(p):
            with open(p, "rb") as f:
                bid.update(n.encode() + f.read())
    tmpl = os.path.join(root, "web", "sw.js")
    with open(tmpl) as f:
        sw = f.read().replace("__BUILD_ID__", bid.hexdigest()[:12])
    with open(os.path.join(build, "sw.js"), "w") as f:
        f.write(sw)
    total = sum(p["size"] for p in manifest["packs"].values())
    print(f"packs: {len(manifest['packs'])} ({total/1e6:.2f} MB), build id {bid.hexdigest()[:12]}")


main()
