#!/usr/bin/env python3 -I
"""Authoring source for data/bosses.json and the boss skill kits (patched into data/skills.json, idempotent).
Run after gen_enemies.py:  python3 -I tools/gen_bosses.py data

Mechanic kinds (implemented in src/core/boss.c):
  adds       every s: spawn `count` of `enemy` around the boss (at most `max` alive)
  vines      every s: if no vine lives, spawn `count`; living vines heal the boss `power` of max HP per second each
  rewind     every s: telegraph, then the boss's HP returns to what it was 10 s ago (never lower than now)
  zones      every s: `count` hazard circles near party members; hazard=sand (slow) | rift | fire (damage `power` of max HP per s)
  song       every s: silence the party for `dur` s and freeze one member (slowed 85%) until cleansed or `dur` passes
  armor      reflecting armor: damage x0.25 and `reflect` back, until `power` (fraction of boss max HP) of ice damage / block-breaks
             has been dealt; then exposed for `dur` s (damage x1.4)
  barrier    every s: a magic-reflecting barrier for `dur` s; two different elements within 3 s shatter it
  curse      every s: Hollow Curse on a random member for `dur` s
  waves      every s: an expanding void ring; jump or roll through it (damage `power` of max HP)
  gravity    the Solo Ascent: allies go down, boss is untouchable, Kael alone; Call Out (after `every` s) revives one ally at a time
  finisher   at `power` of max HP the boss holds and the Quartet Strike becomes available
"""
import json, os, sys

out = sys.argv[1] if len(sys.argv) > 1 else "data"

def skill(id, name, kind, element, power, cd, rng, arc, radius, hit, total, stagger, status="", st_time=0, st_power=0, speed=0, desc=""):
    return {"id": id, "name": name, "role": "enemy", "kind": kind, "element": element, "magic": False, "power": power, "mp": 0, "cooldown": cd,
            "range": rng, "arc": arc, "radius": radius, "hit_time": hit, "total_time": total, "stagger": stagger, "proj_speed": speed, "dash": 0,
            "status": status, "status_time": st_time, "status_power": st_power, "awakening": 0, "gauge": 0, "basic": False, "passive": False,
            "anim": "Sword_Attack", "desc": desc}

SKILLS = [
    skill("thorn_lash", "Thorn Lash", "aoe_circle", "phys", 1.2, 5.0, 1.5, 360, 3.4, 0.8, 1.4, 60, "poison", 6, 6, desc="Thorns erupt around the Thornqueen."),
    skill("time_slash", "Time Slash", "melee_arc", "phys", 1.4, 3.0, 2.8, 140, 0, 0.6, 1.1, 60, "slow", 3, 0.3, desc="A stroke that drags the air."),
    skill("sand_lash", "Sand Lash", "projectile", "phys", 0.9, 4.0, 2.4, 100, 0, 0.6, 1.1, 20, speed=16, desc="Compressed sand, fired like a bolt."),
    skill("frost_shard", "Frost Shard", "projectile", "ice", 1.0, 3.0, 2.4, 100, 0, 0.5, 1.0, 20, "slow", 2, 0.3, speed=15, desc="A shard of the Winter Choir."),
    skill("ice_burst", "Ice Burst", "aoe_circle", "ice", 1.3, 7.0, 2.0, 360, 4.2, 0.9, 1.5, 70, desc="Frost erupts underfoot."),
    skill("molten_slam", "Molten Slam", "aoe_circle", "fire", 2.2, 6.0, 2.2, 360, 4.6, 1.1, 1.9, 140, "burn", 5, 8, desc="A giant overhead slam that splashes magma."),
    skill("void_cleave", "Void Cleave", "melee_arc", "shadow", 2.0, 3.0, 3.4, 170, 0, 0.7, 1.3, 90, desc="A heavy cleave of hollow steel."),
    skill("void_rend", "Void Rend", "melee_arc", "shadow", 1.5, 1.6, 3.0, 150, 0, 0.35, 0.7, 40, desc="A fast combo stroke."),
    skill("void_orb", "Void Orb", "projectile", "shadow", 1.2, 3.5, 2.4, 100, 0, 0.6, 1.1, 30, speed=12, desc="A slow sphere of nothing."),
    skill("gravity_slam", "Gravity Slam", "aoe_circle", "shadow", 1.8, 8.0, 3.0, 360, 5.8, 1.2, 2.0, 120, desc="The air itself comes down."),
]

def mech(kind, **kw):
    d = {"kind": kind}
    d.update(kw)
    return d

BOSSES = [
    {"id": "grukk", "enemy": "grukk", "title": "Goblin Chieftain Grukk", "arena": 20, "enrage": 240,
     "phases": [{"hp_below": 1.0, "name": "Chieftain"},
                {"hp_below": 0.5, "name": "Warcry", "damage": 1.15, "speed": 1.1, "enter": ["toast:Grukk bellows for the warband!"],
                 "mech": [mech("adds", enemy="goblin_grunt", count=2, every=14, max=4)]}]},
    {"id": "darius_duel", "enemy": "darius_duel", "title": "Darius Valecrest", "arena": 14, "enrage": 180,
     "phases": [{"hp_below": 1.0, "name": "Prince's Form"}, {"hp_below": 0.4, "name": "Second Place", "damage": 1.2, "speed": 1.15, "enter": ["toast:Darius stops holding back."]}]},
    {"id": "skarn_1", "enemy": "skarn_1", "title": "Skarn, the Demon Scout", "arena": 22, "enrage": 240,
     "phases": [{"hp_below": 1.0, "name": "Scout"},
                {"hp_below": 0.5, "name": "Marked", "damage": 1.2, "speed": 1.2, "enter": ["toast:Skarn's mark burns."], "mech": [mech("curse", every=16, dur=8)]}]},
    {"id": "library_guardian", "enemy": "library_guardian", "title": "Library Guardian", "arena": 20, "enrage": 300,
     "phases": [{"hp_below": 1.0, "name": "Sentinel", "mech": [mech("armor", power=0.08, dur=7, reflect=0.1)]},
                {"hp_below": 0.5, "name": "Overload", "damage": 1.2, "mech": [mech("barrier", every=22, dur=8), mech("armor", power=0.06, dur=6, reflect=0.1)]}]},
    {"id": "shadow_self", "enemy": "shadow_self", "title": "The Lonely Self", "arena": 18, "enrage": 240,
     "phases": [{"hp_below": 1.0, "name": "Reflection", "mech": [mech("adds", enemy="shadow_clone", count=1, every=18, max=2)]},
                {"hp_below": 0.4, "name": "Alone", "damage": 1.2, "speed": 1.15, "enter": ["toast:It knows every move you make."]}]},
    {"id": "caldrin", "enemy": "caldrin", "title": "Archbishop Caldrin", "arena": 22, "enrage": 300,
     "phases": [{"hp_below": 1.0, "name": "Sermon", "mech": [mech("curse", every=18, dur=8)]},
                {"hp_below": 0.5, "name": "Heresy", "damage": 1.2, "enter": ["cutscene:betrayal"], "mech": [mech("barrier", every=20, dur=8), mech("curse", every=14, dur=8)]}]},
    {"id": "vessara", "enemy": "vessara", "title": "Vessara, the Thornqueen", "arena": 24, "enrage": 330,
     "phases": [{"hp_below": 1.0, "name": "Thorned Court", "mech": [mech("vines", count=3, every=30, power=0.012), mech("adds", enemy="thorn_minion", count=2, every=22, max=4)]},
                {"hp_below": 0.5, "name": "Rootstorm", "damage": 1.25, "speed": 1.1, "enter": ["toast:The roots split the ground!"],
                 "mech": [mech("vines", count=4, every=24, power=0.015), mech("zones", hazard="rift", count=2, every=12, radius=3.0, dur=7, power=0.06)]}]},
    {"id": "khemet_ur", "enemy": "khemet_ur", "title": "Khemet-Ur, the Hourglass Tyrant", "arena": 24, "enrage": 360,
     "phases": [{"hp_below": 1.0, "name": "Sands Running", "mech": [mech("rewind", every=28), mech("zones", hazard="sand", count=2, every=14, radius=4.0, dur=9, power=0)]},
                {"hp_below": 0.5, "name": "Sands Falling", "damage": 1.2, "speed": 1.1, "enter": ["toast:Time runs thin."],
                 "mech": [mech("rewind", every=22), mech("zones", hazard="sand", count=3, every=11, radius=4.0, dur=9, power=0)]}]},
    {"id": "isrevna", "enemy": "isrevna", "title": "Isrevna, the Winter Choir", "arena": 24, "enrage": 360,
     "phases": [{"hp_below": 1.0, "name": "First Verse", "mech": [mech("song", every=24, dur=6), mech("adds", enemy="choir_husk", count=2, every=26, max=3)]},
                {"hp_below": 0.5, "name": "Crescendo", "damage": 1.25, "enter": ["toast:The choir swells."], "mech": [mech("song", every=17, dur=7), mech("zones", hazard="rift", count=2, every=13, radius=3.0, dur=7, power=0.05)]}]},
    {"id": "pyrrhax", "enemy": "pyrrhax", "title": "Pyrrhax, the Molten Warden", "arena": 24, "enrage": 360,
     "phases": [{"hp_below": 1.0, "name": "Molten Armor", "mech": [mech("armor", power=0.08, dur=9, reflect=0.25)]},
                {"hp_below": 0.5, "name": "Furnace Heart", "damage": 1.25, "speed": 1.1, "enter": ["toast:The furnace roars."],
                 "mech": [mech("armor", power=0.06, dur=7, reflect=0.3), mech("zones", hazard="fire", count=2, every=12, radius=3.2, dur=7, power=0.06)]}]},
    {"id": "skarn_final", "enemy": "skarn_final", "title": "Skarn, Herald of the Hollow King", "arena": 24, "enrage": 360,
     "phases": [{"hp_below": 1.0, "name": "Herald", "mech": [mech("curse", every=18, dur=8)]},
                {"hp_below": 0.5, "name": "Last Dance", "damage": 1.3, "speed": 1.2, "enter": ["toast:Skarn drops the pretense."], "mech": [mech("waves", every=14, power=0.05)]}]},
    {"id": "vorathis", "enemy": "vorathis", "title": "Vorathis, the Hollow King", "arena": 26, "enrage": 900,
     "phases": [
         {"hp_below": 1.0, "name": "The Weary King", "mech": [mech("curse", every=15, dur=10), mech("adds", enemy="void_blade", count=2, every=20, max=4)]},
         {"hp_below": 0.72, "name": "Shield of Despair", "damage": 1.15, "enter": ["toast:The Hollow King raises his shield of despair."],
          "mech": [mech("barrier", every=18, dur=10), mech("zones", hazard="rift", count=3, every=11, radius=3.0, dur=8, power=0.07), mech("adds", enemy="shadow_clone", count=2, every=16, max=3)]},
         {"hp_below": 0.45, "name": "The Solo Ascent", "damage": 1.0, "enter": ["toast:Gravity collapses. Kael stands alone."],
          "skills": ["gravity_slam", "void_cleave"], "mech": [mech("gravity", every=15, power=0.6, dur=60)]},
         {"hp_below": 0.0, "name": "Bond Resonance", "damage": 1.3, "speed": 1.2, "enter": ["toast:Vorathis shows his true form.", "gauge:100"],
          "skills": ["void_rend", "void_cleave", "void_orb"], "mech": [mech("waves", every=13, power=0.06), mech("finisher", power=0.12)]},
     ]},
    {"id": "aurel_unbound", "enemy": "aurel_unbound", "title": "Aurel, Unbound", "arena": 28, "enrage": 600, "super": True,
     "phases": [{"hp_below": 1.0, "name": "Unbound", "mech": [mech("curse", every=14, dur=10), mech("waves", every=16, power=0.06)]},
                {"hp_below": 0.6, "name": "Unburdened", "damage": 1.25, "speed": 1.15, "mech": [mech("barrier", every=18, dur=9), mech("zones", hazard="rift", count=3, every=10, radius=3.2, dur=8, power=0.07), mech("waves", every=12, power=0.07)]},
                {"hp_below": 0.25, "name": "Carried", "damage": 1.4, "speed": 1.25, "enter": ["toast:Aurel stops running."], "mech": [mech("waves", every=9, power=0.08), mech("adds", enemy="shadow_clone", count=2, every=14, max=4)]}]},
]

def main():
    p = os.path.join(out, "skills.json")
    data = json.load(open(p))
    arr = data["skills"] if isinstance(data, dict) else data
    ids = {s["id"] for s in SKILLS}
    arr[:] = [s for s in arr if s["id"] not in ids] + SKILLS
    json.dump({"skills": arr} if isinstance(data, dict) else arr, open(p, "w"), indent=1)
    json.dump({"bosses": BOSSES}, open(os.path.join(out, "bosses.json"), "w"), indent=1)
    print(len(BOSSES), "bosses,", len(SKILLS), "boss skills added; skills total", len(arr))

main()
