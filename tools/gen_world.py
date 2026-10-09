#!/usr/bin/env python3 -I
"""Authoring source for data/regions.json and data/zones.json.  python3 -I tools/gen_world.py data"""
import json, os, sys

W = ["clear", "cloudy", "rain", "storm", "snow", "sandstorm", "ash", "fog"]

def weather(**kw):
    return {k: kw.get(k, 0.0) for k in W}

REGIONS = [
    {"id": "vale", "name": "Aurel Vale", "tier": 1,
     "terrain": {"base": 0.0, "amp": 5.0, "freq": 0.018, "ridge": 0.05, "rim": 1.0},
     "ground": {"low": [78, 128, 70], "high": [128, 156, 84], "cliff": [112, 104, 98], "accent": [150, 170, 90]},
     "sky_tint": [1.0, 1.0, 1.0], "fog_tint": [1.0, 1.0, 1.0], "fog_add": 0.0,
     "weather": weather(clear=0.55, cloudy=0.25, rain=0.15, storm=0.05),
     "props": [["tree", 9.0], ["bush", 12.0], ["rock", 3.0], ["pine", 3.0]],
     "enemies": [["goblin_grunt", 3], ["goblin_archer", 2], ["bandit_thug", 2], ["wolf", 2], ["demon_imp", 1], ["shadow_soldier", 1], ["rootling", 1], ["thorn_minion", 1]],
     "dungeon": {"wall": [92, 86, 100], "floor": [64, 60, 76], "torch": [255, 170, 90]}},
    {"id": "sunscar", "name": "Sunscar Desert", "tier": 2,
     "terrain": {"base": 0.0, "amp": 8.0, "freq": 0.014, "ridge": 0.25, "rim": 1.2},
     "ground": {"low": [214, 178, 112], "high": [232, 204, 140], "cliff": [176, 120, 82], "accent": [190, 140, 80]},
     "sky_tint": [1.05, 0.98, 0.88], "fog_tint": [1.1, 0.95, 0.75], "fog_add": 0.002,
     "weather": weather(clear=0.6, cloudy=0.1, sandstorm=0.25, fog=0.05),
     "props": [["cactus", 5.0], ["rock", 4.0], ["deadtree", 1.5], ["bones", 1.5], ["pillar", 1.0]],
     "enemies": [["scorpion", 3], ["sand_serpent", 2], ["tomb_wraith", 1], ["caravan_raider", 2]],
     "dungeon": {"wall": [168, 138, 92], "floor": [120, 98, 66], "torch": [255, 200, 110]}},
    {"id": "frostspire", "name": "Frostspire", "tier": 3,
     "terrain": {"base": 0.0, "amp": 13.0, "freq": 0.016, "ridge": 0.55, "rim": 1.4},
     "ground": {"low": [226, 234, 244], "high": [246, 250, 255], "cliff": [120, 140, 170], "accent": [170, 200, 230]},
     "sky_tint": [0.92, 0.98, 1.08], "fog_tint": [0.9, 0.97, 1.1], "fog_add": 0.003,
     "weather": weather(clear=0.3, cloudy=0.15, snow=0.4, fog=0.15),
     "props": [["pine", 6.0], ["icespike", 3.0], ["rock", 3.0], ["deadtree", 0.8]],
     "enemies": [["frost_wolf", 3], ["ice_wraith", 2], ["choir_husk", 2]],
     "dungeon": {"wall": [120, 150, 190], "floor": [86, 104, 140], "torch": [150, 210, 255]}},
    {"id": "cinderhold", "name": "Cinderhold", "tier": 4,
     "terrain": {"base": 0.0, "amp": 9.0, "freq": 0.02, "ridge": 0.4, "rim": 1.3},
     "ground": {"low": [58, 46, 48], "high": [96, 70, 62], "cliff": [40, 30, 32], "accent": [200, 80, 40]},
     "sky_tint": [1.1, 0.8, 0.7], "fog_tint": [1.2, 0.7, 0.55], "fog_add": 0.003,
     "weather": weather(clear=0.3, cloudy=0.2, ash=0.4, storm=0.1),
     "props": [["lavarock", 5.0], ["rock", 3.0], ["deadtree", 1.0], ["crystal", 0.6]],
     "enemies": [["ember_imp", 3], ["magma_golem", 2], ["demon_imp", 2]],
     "dungeon": {"wall": [74, 52, 52], "floor": [48, 34, 36], "torch": [255, 110, 40]}},
    {"id": "hollow", "name": "The Hollow", "tier": 5,
     "terrain": {"base": 0.0, "amp": 10.0, "freq": 0.017, "ridge": 0.5, "rim": 1.5},
     "ground": {"low": [48, 40, 72], "high": [84, 66, 118], "cliff": [30, 26, 46], "accent": [150, 90, 220]},
     "sky_tint": [0.85, 0.8, 1.15], "fog_tint": [0.85, 0.75, 1.2], "fog_add": 0.004,
     "weather": weather(clear=0.15, cloudy=0.3, fog=0.3, storm=0.25),
     "props": [["crystal", 3.0], ["deadtree", 2.0], ["bones", 1.5], ["rock", 2.0], ["pillar", 0.8]],
     "enemies": [["demon_legionary", 3], ["hollow_archer", 2], ["shadow_soldier", 2], ["shadow_brute", 1]],
     "dungeon": {"wall": [58, 46, 90], "floor": [36, 30, 58], "torch": [190, 120, 255]}},
]

def main(out):
    L = json.load(open(os.path.join(out, "locations.json")))["locations"]
    Q = json.load(open(os.path.join(out, "quests.json")))
    Q = Q["quests"] if isinstance(Q, dict) else Q
    # a boss objective spawns its boss at the objective's marker location
    boss_at = {}
    for q in Q:
        for o in q.get("objectives", []):
            if o.get("kind") == "boss":
                boss_at.setdefault(o.get("marker") or "", []).append({"enemy": o["target"], "quest": q["id"]})
    ids = {l["id"] for l in L}
    zones = []
    for l in L:
        if l.get("sandbox"):
            continue  # sub-places inside Aurelheim: markers within that zone, not zones of their own
        gen = {"town": "hub", "dungeon": "dungeon", "poi": "field", "shrine": "field"}[l["kind"]]
        zones.append({"id": l["id"], "region": l["region"], "gen": gen, "map": l["map"], "bosses": boss_at.get(l["id"], []),
                      "level": 1})
    # sandbox POIs belong to Aurelheim; their quest bosses spawn there
    for l in L:
        if l.get("sandbox"):
            for z in zones:
                if z["id"] == "aurelheim":
                    z["bosses"] += [dict(b, at=l["id"]) for b in boss_at.get(l["id"], [])]
    # region tier -> default enemy level range by zone
    for z in zones:
        z["level"] = next(r["tier"] for r in REGIONS if r["id"] == z["region"])
    for b in boss_at:
        assert b == "" or b in ids, b
    json.dump({"regions": REGIONS}, open(os.path.join(out, "regions.json"), "w"), indent=1)
    json.dump({"zones": zones}, open(os.path.join(out, "zones.json"), "w"), indent=1)
    print(f"regions: {len(REGIONS)}, zones: {len(zones)} ({sum(1 for z in zones if z['gen']=='dungeon')} dungeons), bosses placed: {sum(len(z['bosses']) for z in zones)}")

main(sys.argv[1] if len(sys.argv) > 1 else "data")
