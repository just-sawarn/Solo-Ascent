#!/usr/bin/env python3 -I
"""Authoring script for data/enemies.json (regular enemies and bosses). Usage: gen_enemies.py <data dir>"""
import json, sys, os
out = sys.argv[1]
E = []
def en(id, name, level, st, speed, aggro, poise, behavior, skills, xp, resist=None, scale=1.0, boss=False, top=(30, 30, 40), bot=(20, 20, 30), shoes=(10, 10, 14), eye=(200, 200, 200), pref=None, leash=26):
    E.append(dict(id=id, name=name, level=level, stats=dict(zip(("str", "int", "vit", "wis", "agi"), st)), speed=speed, aggro_range=aggro, leash=leash, poise=poise,
                  behavior=behavior, preferred_range=pref or {"melee": 1.8, "ranged": 8, "brute": 2.2}[behavior], skills=skills, xp=xp, resist=resist or {}, scale=scale, boss=boss,
                  paint=dict(top=list(top), bottom=list(bot), shoes=list(shoes), eye=list(eye))))
SH = {"shadow": 0.5, "holy": 1.5}
# the three sandbox shadows (kept as they were)
en("shadow_soldier", "Shadow Soldier", 1, (9, 4, 8, 4, 6), 3.6, 13, 60, "melee", ["shadow_slash"], 20, SH, 1.0, top=(16, 12, 30), bot=(12, 10, 22), shoes=(8, 6, 14), eye=(190, 90, 255))
en("shadow_archer", "Shadow Archer", 2, (8, 6, 6, 5, 9), 3.2, 16, 35, "ranged", ["shadow_arrow"], 28, SH, 0.97, top=(24, 14, 44), bot=(14, 10, 26), shoes=(8, 6, 14), eye=(120, 200, 255))
en("shadow_brute", "Shadow Brute", 4, (18, 3, 20, 6, 3), 2.6, 12, 220, "brute", ["brute_smash", "shadow_slash"], 70, {**SH, "phys": 0.8}, 1.25, top=(40, 10, 34), bot=(22, 8, 20), shoes=(10, 6, 12), eye=(255, 90, 90))
# act 1
en("demon_imp", "Demon Imp", 1, (7, 4, 5, 3, 10), 4.4, 14, 25, "melee", ["imp_claw"], 12, {"fire": 0.5, "holy": 1.5}, 0.7, top=(110, 20, 30), bot=(70, 14, 22), shoes=(40, 10, 14), eye=(255, 200, 60))
en("bandit_thug", "Road Bandit", 2, (10, 3, 8, 4, 6), 3.6, 12, 70, "melee", ["shadow_slash"], 16, {}, 1.0, top=(90, 70, 50), bot=(60, 48, 36), shoes=(40, 30, 24), eye=(120, 90, 60))
en("goblin_grunt", "Goblin Grunt", 2, (8, 3, 6, 3, 8), 4.0, 14, 40, "melee", ["imp_claw"], 14, {}, 0.75, top=(70, 100, 50), bot=(60, 70, 40), shoes=(40, 40, 30), eye=(255, 220, 80))
en("goblin_archer", "Goblin Shortbowman", 2, (7, 3, 5, 3, 10), 3.6, 16, 30, "ranged", ["shadow_arrow"], 16, {}, 0.75, top=(60, 110, 60), bot=(50, 70, 40), shoes=(40, 40, 30), eye=(255, 220, 80))
en("grukk", "Goblin Chieftain Grukk", 4, (16, 3, 18, 4, 5), 3.0, 18, 300, "brute", ["brute_smash", "shadow_slash"], 160, {}, 1.4, True, top=(80, 120, 50), bot=(80, 70, 40), shoes=(40, 40, 30), eye=(255, 90, 40))
en("demon_marked_goblin", "Demon-Marked Goblin", 3, (11, 5, 8, 4, 8), 4.0, 14, 60, "melee", ["shadow_slash"], 30, SH, 0.8, top=(90, 40, 90), bot=(60, 30, 60), shoes=(40, 30, 40), eye=(190, 90, 255))
en("skarn_1", "Skarn, the Demon Scout", 6, (14, 12, 14, 8, 14), 4.4, 20, 320, "melee", ["shadow_slash", "brute_smash"], 220, {"fire": 0.7, "holy": 1.3}, 1.1, True, top=(60, 14, 40), bot=(40, 10, 30), shoes=(20, 8, 20), eye=(255, 60, 60))
en("cult_zealot", "Hollow Cultist", 5, (9, 8, 8, 6, 7), 3.6, 14, 55, "melee", ["shadow_slash"], 36, SH, 1.0, top=(60, 20, 80), bot=(30, 14, 40), shoes=(20, 10, 24), eye=(210, 120, 255))
en("cult_acolyte", "Hollow Acolyte", 5, (5, 12, 6, 8, 6), 3.2, 16, 40, "ranged", ["shadow_arrow"], 38, SH, 0.95, top=(80, 30, 110), bot=(36, 16, 50), shoes=(20, 10, 24), eye=(210, 120, 255))
# act 3
en("rootling", "Rootling", 6, (10, 3, 10, 3, 5), 3.0, 14, 70, "melee", ["shadow_slash"], 30, {"fire": 1.6, "ice": 0.8}, 0.9, top=(50, 90, 40), bot=(40, 60, 30), shoes=(30, 40, 20), eye=(160, 255, 120))
en("thorn_minion", "Thorn Minion", 7, (12, 4, 9, 4, 8), 3.8, 14, 60, "melee", ["imp_claw"], 34, {"fire": 1.6}, 0.8, top=(70, 110, 40), bot=(50, 70, 30), shoes=(30, 40, 20), eye=(255, 120, 160))
en("wolf", "Whisperwood Wolf", 5, (11, 2, 8, 3, 12), 5.0, 15, 50, "melee", ["imp_claw"], 22, {}, 0.9, top=(100, 100, 100), bot=(80, 80, 80), shoes=(50, 50, 50), eye=(255, 220, 80))
en("greywall_ghost", "Greywall Soldier-Ghost", 7, (12, 5, 10, 8, 6), 3.2, 14, 90, "melee", ["shadow_slash"], 40, {"phys": 0.7, "holy": 1.6}, 1.0, top=(120, 130, 160), bot=(90, 100, 130), shoes=(70, 80, 100), eye=(150, 220, 255))
en("library_guardian", "Library Guardian", 8, (16, 20, 16, 14, 6), 2.8, 18, 400, "brute", ["brute_smash", "shadow_arrow"], 260, {"lightning": 0.5, "phys": 0.8}, 1.5, True, top=(60, 70, 140), bot=(40, 50, 100), shoes=(30, 30, 70), eye=(120, 220, 255))
en("shadow_self", "The Lonely Self", 9, (16, 12, 14, 10, 14), 4.2, 20, 450, "melee", ["shadow_slash", "brute_smash"], 300, {"shadow": 0.5, "holy": 1.5}, 1.0, True, top=(220, 190, 70), bot=(60, 50, 20), shoes=(40, 34, 16), eye=(255, 230, 120))
en("corrupted_beast", "Cult-Corrupted Beast", 7, (14, 3, 12, 3, 9), 4.2, 14, 90, "melee", ["imp_claw", "shadow_slash"], 44, SH, 1.0, top=(70, 30, 90), bot=(50, 20, 60), shoes=(30, 14, 40), eye=(230, 90, 255))
en("vessara", "Vessara, the Thornqueen", 10, (18, 20, 20, 16, 8), 3.0, 22, 700, "brute", ["brute_smash", "shadow_arrow"], 600, {"fire": 1.8, "ice": 0.7, "lightning": 0.9}, 1.6, True, top=(70, 130, 60), bot=(50, 90, 40), shoes=(30, 60, 20), eye=(255, 150, 190))
# act 4
en("scorpion", "Sand Scorpion", 9, (14, 2, 10, 3, 9), 4.2, 14, 80, "melee", ["imp_claw"], 40, {"ice": 1.5}, 0.9, top=(150, 110, 50), bot=(120, 80, 40), shoes=(80, 60, 30), eye=(255, 120, 40))
en("sand_serpent", "Sand Serpent", 10, (15, 4, 12, 4, 12), 5.0, 16, 100, "melee", ["shadow_slash"], 55, {"ice": 1.4}, 1.2, top=(190, 150, 70), bot=(150, 110, 50), shoes=(100, 80, 40), eye=(255, 220, 60))
en("tomb_wraith", "Tomb Wraith", 11, (12, 14, 10, 12, 8), 3.4, 16, 80, "ranged", ["shadow_arrow"], 60, {"phys": 0.6, "holy": 1.6}, 1.0, top=(150, 170, 150), bot=(110, 130, 110), shoes=(80, 100, 80), eye=(120, 255, 180))
en("caravan_raider", "Caravan Raider", 9, (14, 3, 11, 3, 9), 3.8, 14, 90, "melee", ["shadow_slash"], 42, {}, 1.0, top=(160, 60, 50), bot=(100, 50, 40), shoes=(60, 40, 30), eye=(255, 190, 60))
en("khemet_ur", "Khemet-Ur, the Hourglass Tyrant", 14, (22, 20, 24, 18, 10), 3.2, 24, 900, "brute", ["brute_smash", "shadow_slash"], 900, {"phys": 0.8, "holy": 1.2}, 1.7, True, top=(210, 170, 70), bot=(130, 100, 40), shoes=(80, 60, 30), eye=(255, 230, 120))
en("frost_wolf", "Frost Wolf", 12, (16, 2, 12, 3, 13), 5.2, 16, 80, "melee", ["imp_claw"], 55, {"fire": 1.5, "ice": 0.3}, 1.0, top=(200, 220, 240), bot=(160, 190, 220), shoes=(120, 150, 180), eye=(120, 220, 255))
en("ice_wraith", "Ice Wraith", 13, (12, 16, 10, 12, 9), 3.4, 18, 80, "ranged", ["shadow_arrow"], 65, {"fire": 1.6, "ice": 0.2}, 1.0, top=(170, 210, 240), bot=(130, 170, 210), shoes=(100, 140, 180), eye=(180, 240, 255))
en("choir_husk", "Choir Husk", 13, (14, 6, 14, 10, 6), 3.0, 15, 120, "melee", ["shadow_slash"], 70, {"holy": 1.4, "ice": 0.5}, 1.0, top=(180, 190, 220), bot=(130, 140, 180), shoes=(100, 110, 150), eye=(200, 220, 255))
en("isrevna", "Isrevna, the Winter Choir", 16, (18, 26, 22, 20, 12), 3.2, 24, 950, "ranged", ["shadow_arrow", "brute_smash"], 1000, {"fire": 1.6, "ice": 0.2}, 1.6, True, top=(190, 220, 255), bot=(120, 160, 220), shoes=(90, 120, 190), eye=(220, 245, 255), pref=9)
en("caldrin", "Archbishop Caldrin", 14, (14, 20, 16, 22, 8), 3.0, 22, 600, "ranged", ["shadow_arrow", "brute_smash"], 700, {"shadow": 0.7, "holy": 0.5}, 1.05, True, top=(240, 235, 225), bot=(200, 190, 170), shoes=(150, 130, 90), eye=(255, 220, 120), pref=7)
en("darius_duel", "Darius Valecrest", 5, (13, 8, 11, 6, 12), 4.4, 20, 260, "melee", ["shadow_slash", "brute_smash"], 180, {}, 1.0, True, top=(170, 40, 50), bot=(40, 30, 40), shoes=(30, 24, 30), eye=(255, 140, 60))
# act 5
en("ember_imp", "Ember Imp", 17, (14, 8, 10, 4, 14), 4.6, 15, 80, "melee", ["imp_claw"], 70, {"fire": 0.1, "ice": 1.6}, 0.7, top=(230, 90, 30), bot=(160, 50, 20), shoes=(100, 30, 12), eye=(255, 230, 80))
en("magma_golem", "Magma Golem", 18, (22, 2, 28, 4, 3), 2.4, 14, 420, "brute", ["brute_smash"], 140, {"fire": 0.1, "ice": 1.7, "phys": 0.8}, 1.7, top=(200, 70, 30), bot=(120, 40, 20), shoes=(80, 30, 14), eye=(255, 220, 80))
en("pyrrhax", "Pyrrhax, the Molten Warden", 20, (28, 12, 32, 14, 6), 2.8, 24, 1300, "brute", ["brute_smash", "shadow_slash"], 1400, {"fire": 0.1, "ice": 1.6, "phys": 0.7}, 2.0, True, top=(230, 100, 40), bot=(150, 60, 24), shoes=(90, 36, 14), eye=(255, 240, 120))
en("demon_legionary", "Demon Legionary", 22, (22, 8, 18, 8, 10), 3.8, 16, 150, "melee", ["shadow_slash"], 90, {"shadow": 0.5, "holy": 1.5}, 1.1, top=(70, 20, 60), bot=(40, 14, 40), shoes=(24, 10, 24), eye=(255, 70, 90))
en("hollow_archer", "Hollow Marksman", 22, (16, 10, 12, 8, 14), 3.4, 18, 90, "ranged", ["shadow_arrow"], 90, {"shadow": 0.5, "holy": 1.5}, 1.0, top=(60, 20, 70), bot=(36, 14, 44), shoes=(24, 10, 24), eye=(255, 70, 90))
en("skarn_final", "Skarn, Herald of the Hollow King", 24, (26, 20, 22, 14, 16), 4.8, 24, 1400, "melee", ["shadow_slash", "brute_smash"], 1800, {"fire": 0.7, "holy": 1.3}, 1.3, True, top=(70, 10, 50), bot=(46, 8, 34), shoes=(24, 6, 24), eye=(255, 40, 70))
en("vorathis", "Vorathis, the Hollow King", 28, (34, 30, 34, 24, 14), 3.4, 30, 3000, "brute", ["brute_smash", "shadow_slash"], 5000, {"shadow": 0.3, "holy": 1.4}, 2.1, True, top=(20, 14, 40), bot=(14, 10, 30), shoes=(8, 6, 16), eye=(180, 140, 255))

# ---- milestone 10: boss mechanic adds and the super-boss; boss skill kits ----
en("thorn_vine", "Living Vine", 8, (4, 2, 12, 2, 1), 0.0, 3, 30, "melee", ["imp_claw"], 10, {"fire": 2.0}, 1.0, top=(60, 120, 40), bot=(40, 80, 30), shoes=(30, 50, 20), eye=(160, 255, 120), leash=1)
en("void_blade", "Shadow Blade", 24, (18, 4, 6, 4, 18), 5.6, 30, 40, "melee", ["shadow_slash"], 0, SH, 0.8, top=(10, 6, 24), bot=(8, 6, 18), shoes=(6, 4, 12), eye=(200, 100, 255))
en("shadow_clone", "Shadow Clone", 24, (16, 10, 12, 8, 12), 4.2, 30, 90, "melee", ["shadow_slash"], 0, SH, 1.0, top=(30, 24, 60), bot=(20, 16, 40), shoes=(10, 8, 20), eye=(220, 120, 255))
en("aurel_unbound", "Aurel, Unbound", 40, (44, 40, 44, 30, 18), 3.8, 36, 4000, "brute", ["void_cleave", "gravity_slam"], 9000, {"shadow": 0.3, "holy": 1.0}, 2.2, True, top=(230, 200, 120), bot=(60, 50, 90), shoes=(30, 24, 50), eye=(255, 230, 150))
KITS = {
    "vessara": ["thorn_lash", "brute_smash", "shadow_arrow"], "khemet_ur": ["time_slash", "brute_smash", "sand_lash"],
    "isrevna": ["frost_shard", "ice_burst", "shadow_arrow"], "pyrrhax": ["molten_slam", "brute_smash"],
    "vorathis": ["void_cleave", "brute_smash", "void_orb"], "skarn_final": ["void_rend", "shadow_slash", "brute_smash"],
}
for e_ in E:
    if e_["id"] in KITS: e_["skills"] = KITS[e_["id"]]
json.dump({"enemies": E}, open(os.path.join(out, "enemies.json"), "w"), indent=1)
print(len(E), "enemies")
