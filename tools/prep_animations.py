#!/usr/bin/env python3 -I
"""Convert Quaternius Universal Animation Library (CC0) clips to a compact runtime format (.sanim).
 - resamples every clip to a fixed 30 fps (slerp), stores local rotations as int16 quaternions
 - stores translation tracks only for bones that have them (root / pelvis)
Usage: prep_animations.py <UAL1_Standard.glb> <out.sanim>

.sanim layout (little endian):
  'SANM' u32 version(1) u32 nbones  nbones*char[32] names
  u32 nclips; per clip: char[32] name, f32 fps, u32 frames, u8 loop, u8[nbones] has_trans,
    int16[frames*nbones*4] rot (xyzw * 32767), f32[frames*3] translation per has_trans bone (bone order)
"""
import json, math, struct, sys

KEEP = ["Idle_Loop", "Idle_Talking_Loop", "Walk_Loop", "Walk_Formal_Loop", "Jog_Fwd_Loop", "Sprint_Loop", "Crouch_Idle_Loop",
        "Crouch_Fwd_Loop", "Jump_Start", "Jump_Loop", "Jump_Land", "Roll", "Sword_Idle", "Sword_Attack", "Punch_Jab", "Punch_Cross",
        "Hit_Chest", "Hit_Head", "Death01", "Interact", "PickUp_Table", "Spell_Simple_Enter", "Spell_Simple_Idle_Loop",
        "Spell_Simple_Shoot", "Spell_Simple_Exit", "Sitting_Idle_Loop", "Sitting_Talking_Loop", "Sitting_Enter", "Sitting_Exit",
        "Dance_Loop", "Push_Loop", "A_TPose"]
LOOPS = {n for n in KEEP if n.endswith("_Loop")} | {"Sword_Idle"}
FPS = 30.0

def load_glb(path):
    d = open(path, "rb").read()
    jl = struct.unpack_from("<I", d, 12)[0]
    j = json.loads(d[20:20 + jl])
    bin_off = 20 + jl + 8
    return j, d[bin_off:]

def accessor(j, blob, ai):
    a = j["accessors"][ai]; v = j["bufferViews"][a["bufferView"]]
    ncomp = {"SCALAR": 1, "VEC3": 3, "VEC4": 4}[a["type"]]
    off = v.get("byteOffset", 0) + a.get("byteOffset", 0)
    fmt = "<%df" % ncomp
    stride = v.get("byteStride") or 4 * ncomp
    return [struct.unpack_from(fmt, blob, off + i * stride) for i in range(a["count"])]

def slerp(a, b, t):
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0: b = tuple(-x for x in b); dot = -dot
    if dot > 0.9995:
        r = tuple(x + (y - x) * t for x, y in zip(a, b))
    else:
        th = math.acos(dot); s = math.sin(th)
        wa = math.sin((1 - t) * th) / s; wb = math.sin(t * th) / s
        r = tuple(wa * x + wb * y for x, y in zip(a, b))
    n = math.sqrt(sum(x * x for x in r)) or 1.0
    return tuple(x / n for x in r)

def sample(times, vals, t, is_rot):
    if t <= times[0]: return vals[0]
    if t >= times[-1]: return vals[-1]
    lo, hi = 0, len(times) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if times[mid] <= t: lo = mid
        else: hi = mid
    u = (t - times[lo]) / (times[hi] - times[lo])
    if is_rot: return slerp(vals[lo], vals[hi], u)
    return tuple(x + (y - x) * u for x, y in zip(vals[lo], vals[hi]))

def main(src, dst):
    j, blob = load_glb(src)
    nodes = j["nodes"]; joints = j["skins"][0]["joints"]
    bone_names = [nodes[i]["name"] for i in joints]
    node_to_bone = {n: i for i, n in enumerate(joints)}
    nb = len(bone_names)
    # rest local rotations (for bones without a rotation channel)
    rest = [tuple(nodes[i].get("rotation", (0, 0, 0, 1))) for i in joints]
    out = bytearray(b"SANM" + struct.pack("<II", 1, nb))
    for n in bone_names: out += n.encode().ljust(32, b"\0")
    clips = [a for a in j["animations"] if a["name"] in KEEP]
    out += struct.pack("<I", len(clips))
    for a in clips:
        rot_tr, tr_tr = {}, {}
        dur = 0.0
        for ch in a["channels"]:
            node = ch["target"]["node"]
            if node not in node_to_bone: continue
            s = a["samplers"][ch["sampler"]]
            times = [t[0] for t in accessor(j, blob, s["input"])]
            vals = accessor(j, blob, s["output"])
            dur = max(dur, times[-1])
            if ch["target"]["path"] == "rotation": rot_tr[node_to_bone[node]] = (times, vals)
            elif ch["target"]["path"] == "translation": tr_tr[node_to_bone[node]] = (times, vals)
        frames = max(2, int(round(dur * FPS)) + 1)
        out += a["name"].encode().ljust(32, b"\0") + struct.pack("<fIB", FPS, frames, 1 if a["name"] in LOOPS else 0)
        has_t = [1 if b in tr_tr else 0 for b in range(nb)]
        out += bytes(has_t)
        for f in range(frames):
            t = min(f / FPS, dur)
            for b in range(nb):
                q = sample(*rot_tr[b], t, True) if b in rot_tr else rest[b]
                out += struct.pack("<4h", *[int(round(max(-1, min(1, c)) * 32767)) for c in q])
        for b in range(nb):
            if b in tr_tr:
                for f in range(frames):
                    out += struct.pack("<3f", *sample(*tr_tr[b], min(f / FPS, dur), False))
    open(dst, "wb").write(out)
    print("clips", len(clips), "bytes", len(out))

main(sys.argv[1], sys.argv[2])
