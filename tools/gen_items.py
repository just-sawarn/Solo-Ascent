#!/usr/bin/env python3 -I
"""Authoring script for the item content: writes data/items.json, recipes.json, enchants.json, shops.json, loot.json.
Edit THIS file (and re-run it) rather than the generated JSON, so tiers stay consistent. Usage: gen_items.py <data dir>"""
import json, sys, os

out = sys.argv[1]
TIERS = [  # (key, region, level, rarity, label)
    ("t1", "Ashbrook", 1, "common", "Iron"),
    ("t2", "Whisperwood", 5, "uncommon", "Verdant"),
    ("t3", "Sunscar", 10, "rare", "Sunglass"),
    ("t4", "Frostspire", 15, "rare", "Rimeforged"),
    ("t5", "Cinderhold", 20, "epic", "Emberforged"),
    ("t6", "Hollow Reach", 25, "legendary", "Dawnlit"),
]
ORE = {"t1": "iron_ore", "t2": "heartwood", "t3": "sun_bronze", "t4": "frost_silver", "t5": "ember_steel", "t6": "void_crystal"}
HIDE = {"t1": "wolf_pelt", "t2": "dryad_bark", "t3": "scorpion_chitin", "t4": "yeti_fur", "t5": "magma_scale", "t6": "shadow_silk"}
HERB = {"t1": "meadow_herb", "t2": "moonpetal", "t3": "desert_bloom", "t4": "frostcap", "t5": "ashroot", "t6": "hollow_lily"}
ESS = {"t1": "spark_dust", "t2": "life_essence", "t3": "time_sand", "t4": "spirit_shard", "t5": "flame_core", "t6": "dawn_fragment"}
items = []
def add(**k):
    d = dict(slot="", role="any", tier="", rarity="common", level=1, stack=1, buy=0, sell=0, stats={}, model="", effect={}, desc="", upgrade_mat="", sets="")
    d.update(k); items.append(d)

def price(base, rarity):
    return int(base * {"common": 1, "uncommon": 1.8, "rare": 3.2, "epic": 6, "legendary": 12}[rarity])

# ---------------- weapons ----------------
W_ATK = [6, 12, 20, 30, 42, 56]; W_MATK = [9, 17, 28, 40, 55, 72]
HERO_N = ["Dawnbrand", "Briarblade", "Sunglass Saber", "Rimeblade", "Emberedge", "Dawnreaver"]
HERO_M = ["short_sword", "short_sword", "long_sword", "long_sword", "katana", "katana"]
TANK_N = ["Iron Bulwark", "Heartwood Aegis", "Sunbronze Ward", "Rimeguard", "Cinder Bastion", "Hollow Ward"]
MAGE_N = ["Hedge Staff", "Moonpetal Staff", "Sandglass Staff", "Rime Spire", "Ember Scepter", "Void Spire"]
HEAL_N = ["Pilgrim's Mace", "Lifebloom Rod", "Hourglass Rod", "Choir Crook", "Forge Censer", "Dawn Scepter"]
for i, (tk, reg, lv, rar, lab) in enumerate(TIERS):
    add(id=f"w_hero_{tk}", name=HERO_N[i], type="weapon", slot="weapon", role="hero", tier=tk, rarity=rar, level=lv, stats={"atk": W_ATK[i], "crit": 0.01 * i}, model=HERO_M[i], buy=price(60 + 40 * i * i, rar), sell=price(18 + 12 * i * i, rar), upgrade_mat=ORE[tk], desc=f"A sword from {reg}.")
    add(id=f"w_tank_{tk}", name=TANK_N[i], type="weapon", slot="weapon", role="tank", tier=tk, rarity=rar, level=lv, stats={"atk": int(W_ATK[i] * 0.8), "armor": 2 + 2 * i, "hp": 20 * i}, model="long_sword", buy=price(60 + 40 * i * i, rar), sell=price(18 + 12 * i * i, rar), upgrade_mat=ORE[tk], desc=f"A shield-and-blade set from {reg}.")
    add(id=f"w_mage_{tk}", name=MAGE_N[i], type="weapon", slot="weapon", role="mage", tier=tk, rarity=rar, level=lv, stats={"matk": W_MATK[i], "mp": 10 * i}, model="staff", buy=price(60 + 45 * i * i, rar), sell=price(18 + 13 * i * i, rar), upgrade_mat=ORE[tk], desc=f"A focus carved in {reg}.")
    add(id=f"w_heal_{tk}", name=HEAL_N[i], type="weapon", slot="weapon", role="healer", tier=tk, rarity=rar, level=lv, stats={"matk": int(W_MATK[i] * 0.85), "wis": 1 + i, "mp": 12 * i}, model="staff", buy=price(60 + 45 * i * i, rar), sell=price(18 + 13 * i * i, rar), upgrade_mat=ORE[tk], desc=f"A blessed rod from {reg}.")
# variety and legendaries
add(id="w_hero_axe_t3", name="Dune Cleaver", type="weapon", slot="weapon", role="hero", tier="t3", rarity="rare", level=10, stats={"atk": 26, "str": 2}, model="axe", buy=700, sell=210, upgrade_mat="sun_bronze", desc="Heavy and slow, hits hard.")
add(id="w_hero_spear_t4", name="Rime Pike", type="weapon", slot="weapon", role="hero", tier="t4", rarity="rare", level=15, stats={"atk": 35, "agi": 2}, model="spear", buy=1100, sell=330, upgrade_mat="frost_silver", desc="Long reach.")
add(id="w_hero_dagger_t2", name="Whisper Dagger", type="weapon", slot="weapon", role="hero", tier="t2", rarity="uncommon", level=5, stats={"atk": 9, "crit": 0.06}, model="dagger", buy=260, sell=78, upgrade_mat="heartwood", desc="Quick and quiet.")
add(id="w_hero_club_t1", name="Village Club", type="weapon", slot="weapon", role="hero", tier="t1", rarity="common", level=1, stats={"atk": 5, "str": 1}, model="club", buy=30, sell=9, upgrade_mat="iron_ore", desc="Better than nothing.")
add(id="w_hero_zenith", name="Dawnbrand: Zenith", type="weapon", slot="weapon", role="hero", tier="t6", rarity="legendary", level=22, stats={"atk": 80, "crit": 0.10, "str": 4, "agi": 4}, model="katana", buy=0, sell=0, upgrade_mat="void_crystal", desc="Orwyn's sword, awakened: Ascendant gold runs along the edge.")
add(id="w_tank_oaths", name="Bulwark of Oaths", type="weapon", slot="weapon", role="tank", tier="t5", rarity="legendary", level=15, stats={"atk": 40, "armor": 22, "hp": 180, "vit": 5}, model="long_sword", buy=0, sell=0, upgrade_mat="ember_steel", desc="Bram's shield, engraved with every name he failed to save.")
add(id="w_mage_starcaller", name="Starcaller", type="weapon", slot="weapon", role="mage", tier="t5", rarity="legendary", level=18, stats={"matk": 82, "mp": 90, "int": 6}, model="staff", buy=0, sell=0, upgrade_mat="ember_steel", desc="Lyra's focus. It hums when she's calm.")
add(id="w_heal_dawnsong", name="Dawnsong Staff", type="weapon", slot="weapon", role="healer", tier="t5", rarity="legendary", level=18, stats={"matk": 70, "wis": 8, "mp": 80}, model="staff", buy=0, sell=0, upgrade_mat="ember_steel", desc="Mirelle's staff, lit by her own light.")

# ---------------- armor ----------------
BODY = [4, 9, 15, 23, 33, 45]
ROLE_ARMOR = {"hero": ("Traveler's Coat", "Ranger Coat", "Dune Mantle", "Rime Overcoat", "Ember Greatcoat", "Hollow Coat"),
              "tank": ("Iron Cuirass", "Heartwood Plate", "Sunbronze Plate", "Rime Plate", "Cinder Plate", "Hollow Plate"),
              "mage": ("Hedge Robe", "Moonpetal Robe", "Sandglass Robe", "Rime Robe", "Ember Robe", "Void Robe"),
              "healer": ("Pilgrim Robe", "Lifebloom Robe", "Hourglass Robe", "Choir Robe", "Forge Vestments", "Dawn Vestments")}
for role, names in ROLE_ARMOR.items():
    for i, (tk, reg, lv, rar, lab) in enumerate(TIERS):
        mult = {"tank": 1.5, "hero": 1.0, "mage": 0.6, "healer": 0.7}[role]
        arm = max(1, int(BODY[i] * mult)); hp = int({"tank": 40, "hero": 25, "mage": 12, "healer": 18}[role] * (i + 1))
        st = {"armor": arm, "hp": hp}
        if role == "mage": st["mp"] = 15 * (i + 1); st["int"] = 1 + i // 2
        if role == "healer": st["mp"] = 12 * (i + 1); st["wis"] = 1 + i // 2
        if role == "hero": st["agi"] = 1 + i // 3
        if role == "tank": st["vit"] = 1 + i // 2
        add(id=f"a_{role}_body_{tk}", name=names[i], type="armor", slot="body", role=role, tier=tk, rarity=rar, level=lv, stats=st, buy=price(50 + 35 * i * i, rar), sell=price(15 + 10 * i * i, rar), upgrade_mat=HIDE[tk] if role in ("hero", "mage", "healer") else ORE[tk], desc=f"{lab} body armor.")
        head_n = {"hero": "Cap", "tank": "Helm", "mage": "Hood", "healer": "Circlet"}[role]
        boot_n = {"hero": "Boots", "tank": "Greaves", "mage": "Slippers", "healer": "Sandals"}[role]
        add(id=f"a_{role}_head_{tk}", name=f"{lab} {head_n}", type="armor", slot="head", role=role, tier=tk, rarity=rar, level=lv, stats={"armor": max(1, int(arm * 0.6)), "hp": int(hp * 0.4), **({"mp": 8 * (i + 1)} if role in ("mage", "healer") else {})}, buy=price(30 + 20 * i * i, rar), sell=price(9 + 6 * i * i, rar), upgrade_mat=HIDE[tk], desc=f"{lab} headgear.")
        add(id=f"a_{role}_boots_{tk}", name=f"{lab} {boot_n}", type="armor", slot="boots", role=role, tier=tk, rarity=rar, level=lv, stats={"armor": max(1, int(arm * 0.5)), "agi": 1 + i // 2}, buy=price(28 + 18 * i * i, rar), sell=price(8 + 5 * i * i, rar), upgrade_mat=HIDE[tk], desc=f"{lab} footwear.")

# ---------------- accessories ----------------
ACC = [
    ("acc_guild_ring", "Guild Ring", "common", 1, {"hp": 25}, "Issued to every registered adventurer."),
    ("acc_ember_charm", "Ember Charm", "uncommon", 5, {"atk": 4}, "Warm to the touch."),
    ("acc_sage_pendant", "Sage's Pendant", "uncommon", 5, {"matk": 5, "mp": 15}, "Orwyn wore one like it."),
    ("acc_moonpetal_locket", "Moonpetal Locket", "uncommon", 6, {"hp": 40, "wis": 1}, "Never wilts."),
    ("acc_sandglass_charm", "Sandglass Charm", "rare", 10, {"agi": 3, "crit": 0.03}, "The sand inside flows upward."),
    ("acc_scorpion_band", "Scorpion Band", "rare", 10, {"atk": 8, "crit": 0.02}, "Chitin ring from the catacombs."),
    ("acc_tithe_ward", "Tithe Ward", "rare", 12, {"hp": 90, "wis": 3}, "A holy charm that does not feed on its wearer."),
    ("acc_frost_circlet_pin", "Frostsong Pin", "rare", 15, {"matk": 12, "mp": 30}, "Isrevna's songs, in silver."),
    ("acc_rime_ring", "Rime Ring", "rare", 15, {"armor": 6, "hp": 70}, "Cold, and oddly comforting."),
    ("acc_forge_signet", "Forge Signet", "epic", 20, {"atk": 14, "armor": 6}, "Forgemaster Brandt's mark."),
    ("acc_cinder_amulet", "Cinder Amulet", "epic", 20, {"matk": 16, "hp": 120}, "Smoulders without burning."),
    ("acc_greywall_tag", "Greywall Tag", "epic", 12, {"armor": 8, "vit": 3}, "A dog tag from Fort Greywall. It hums when the squad is near.", ),
    ("acc_thorn_crown", "Thornqueen's Tear", "epic", 15, {"hp": 150, "wis": 4}, "Vessara's last tear, crystallized."),
    ("acc_hollow_band", "Hollow Band", "epic", 25, {"atk": 18, "matk": 18}, "Cold, empty, powerful."),
    ("acc_dawn_ring", "Dawn Ring", "legendary", 22, {"atk": 20, "matk": 20, "hp": 200, "crit": 0.06}, "Warm gold. It remembers the first light."),
    ("acc_oath_knot", "Oath Knot", "rare", 12, {"vit": 4, "hp": 100}, "A soldier's promise, braided."),
    ("acc_swift_anklet", "Swift Anklet", "uncommon", 8, {"agi": 4}, "Light on the feet."),
    ("acc_focus_bead", "Focus Bead", "uncommon", 8, {"int": 3, "mp": 25}, "Clears the mind."),
    ("acc_warding_charm", "Warding Charm", "uncommon", 8, {"armor": 4, "wis": 2}, "Wards off small curses."),
    ("acc_lucky_coin", "Lucky Coin", "common", 3, {"crit": 0.02}, "Heads, usually."),
    ("acc_pilgrims_beads", "Pilgrim's Beads", "common", 3, {"wis": 2, "mp": 10}, "Each bead, a quiet prayer."),
    ("acc_hunters_tooth", "Hunter's Tooth", "common", 3, {"atk": 3}, "Wolf fang on a cord."),
    ("acc_ash_scarf", "Ashen Scarf", "rare", 18, {"hp": 80, "armor": 4, "vit": 2}, "Survivors of Emberforge wear them."),
    ("acc_lio_pick", "Lio's Lucky Pick", "rare", 10, {"mp": 30, "wis": 2, "agi": 2}, "A bard's lute pick; it plays itself when you sleep."),
    ("acc_voidstone", "Voidstone Shard", "epic", 24, {"matk": 22, "mp": 60}, "Looking at it too long hurts."),
]
for iid, n, rar, lv, st, d in ACC:
    add(id=iid, name=n, type="accessory", slot="accessory", rarity=rar, level=lv, stats=st, buy=price(40 + 6 * lv * lv // 3, rar), sell=price(12 + 2 * lv * lv // 3, rar), desc=d)

# ---------------- consumables ----------------
def cons(i, n, rar, hp=0, mp=0, status="", spow=0.0, stime=0, cleanse=False, revive=0.0, buy=10, desc="", stack=30, cd=1.5):
    add(id=i, name=n, type="consumable", rarity=rar, stack=stack, buy=buy, sell=max(1, buy // 3), effect={"hp": hp, "mp": mp, "status": status, "status_power": spow, "status_time": stime, "cleanse": cleanse, "revive": revive, "cooldown": cd}, desc=desc)
cons("hp_small", "Small Healing Draught", "common", hp=60, buy=15, desc="Restores 60 HP.")
cons("hp_medium", "Healing Draught", "common", hp=150, buy=40, desc="Restores 150 HP.")
cons("hp_large", "Greater Healing Draught", "uncommon", hp=320, buy=110, desc="Restores 320 HP.")
cons("hp_grand", "Grand Healing Draught", "rare", hp=650, buy=300, desc="Restores 650 HP.")
cons("hp_elixir", "Elixir of Dawn", "epic", hp=1400, mp=200, buy=900, desc="Restores a great deal of HP and MP.")
cons("mp_small", "Small Mana Tonic", "common", mp=40, buy=20, desc="Restores 40 MP.")
cons("mp_medium", "Mana Tonic", "common", mp=100, buy=55, desc="Restores 100 MP.")
cons("mp_large", "Greater Mana Tonic", "uncommon", mp=220, buy=140, desc="Restores 220 MP.")
cons("mp_grand", "Grand Mana Tonic", "rare", mp=450, buy=380, desc="Restores 450 MP.")
cons("mp_elixir", "Elixir of Stars", "epic", mp=900, hp=300, buy=1000, desc="Restores a great deal of MP.")
cons("antidote", "Antidote", "common", cleanse=True, buy=25, desc="Removes poison, burn and slow.")
cons("holy_water", "Holy Water", "uncommon", cleanse=True, status="regen", spow=5, stime=8, buy=90, desc="Cleanses and gives brief regeneration.")
cons("curse_salt", "Curse-Breaking Salt", "rare", cleanse=True, buy=200, desc="Cleanses all negative effects.")
cons("phoenix_feather", "Phoenix Feather", "epic", revive=0.5, buy=1200, stack=10, desc="Revives a fallen ally with 50% HP.")
cons("dawn_crystal", "Dawn Crystal", "rare", revive=0.25, buy=500, stack=10, desc="Revives a fallen ally with 25% HP.")
cons("elixir_might", "Elixir of Might", "uncommon", status="buff_atk", spow=0.2, stime=60, buy=120, desc="+20% attack for a minute.")
cons("elixir_guard", "Elixir of Ironskin", "uncommon", status="buff_def", spow=0.25, stime=60, buy=120, desc="-25% damage taken for a minute.")
cons("elixir_focus", "Elixir of Focus", "uncommon", status="mana_regen", spow=4, stime=60, buy=110, desc="Faster mana regeneration for a minute.")
cons("elixir_vigor", "Elixir of Vigor", "uncommon", status="regen", spow=8, stime=45, buy=110, desc="Regenerate HP for 45 seconds.")
cons("food_bread", "Village Bread", "common", hp=40, buy=6, stack=40, desc="Tilly's mother bakes it.")
cons("food_stew", "Hearty Stew", "common", hp=120, status="regen", spow=3, stime=20, buy=25, desc="Warm and filling.")
cons("food_grilled_fish", "Grilled River Fish", "common", hp=90, mp=30, buy=22, desc="Simple and good.")
cons("food_desert_flatbread", "Spiced Flatbread", "uncommon", hp=200, mp=50, buy=60, desc="Zaira's rare spice makes it sing.")
cons("food_frost_broth", "Frost Broth", "uncommon", hp=260, status="buff_def", spow=0.1, stime=120, buy=80, desc="Keeps the cold away.")
cons("food_lava_fish", "Lava-Fish Special", "rare", hp=500, status="buff_atk", spow=0.15, stime=180, buy=220, desc="Dangerously spicy.")
cons("food_festival_cake", "Founders' Festival Cake", "rare", hp=400, mp=200, buy=0, stack=10, desc="Served once a year in Aurelheim.")
cons("bomb_fire", "Fire Bomb", "uncommon", status="burn", spow=8, stime=5, buy=60, desc="Thrown: sets a target on fire (used from the quick menu).")

# ---------------- materials ----------------
def mat(i, n, rar, buy, desc="", stack=99):
    add(id=i, name=n, type="material", rarity=rar, stack=stack, buy=buy, sell=max(1, buy // 3), desc=desc)
for tk, reg, lv, rar, lab in TIERS:
    mrar = rar if rar != "legendary" else "epic"
    mat(ORE[tk], {"t1": "Iron Ore", "t2": "Heartwood Plank", "t3": "Sun Bronze Ingot", "t4": "Frost Silver Ore", "t5": "Ember Steel Ingot", "t6": "Void Crystal"}[tk], mrar, 10 + 12 * int(tk[1]) ** 2, f"Smithing material from {reg}.")
    mat(HIDE[tk], {"t1": "Wolf Pelt", "t2": "Dryad Bark", "t3": "Scorpion Chitin", "t4": "Yeti Fur", "t5": "Magma Scale", "t6": "Shadow Silk"}[tk], mrar, 8 + 10 * int(tk[1]) ** 2, f"Leatherwork material from {reg}.")
    mat(HERB[tk], {"t1": "Meadow Herb", "t2": "Moonpetal", "t3": "Desert Bloom", "t4": "Frostcap", "t5": "Ashroot", "t6": "Hollow Lily"}[tk], mrar, 5 + 6 * int(tk[1]) ** 2, f"Alchemy herb from {reg}.")
    mat(ESS[tk], {"t1": "Spark Dust", "t2": "Life Essence", "t3": "Time Sand", "t4": "Spirit Shard", "t5": "Flame Core", "t6": "Dawn Fragment"}[tk], "rare" if tk != "t1" else "uncommon", 30 + 25 * int(tk[1]) ** 2, f"Enchanting essence from {reg}.")
for i, n, r, b, d in [("coal", "Coal", "common", 6, "Fuel for the forge."), ("linen_thread", "Linen Thread", "common", 5, "For stitching."), ("crystal_vial", "Crystal Vial", "common", 8, "Alchemy glassware."),
                      ("river_fish", "River Fish", "common", 6, "Fresh."), ("wild_flour", "Wild Flour", "common", 4, "Ground by hand."), ("spice_blend", "Zaira's Spice Blend", "uncommon", 40, "A caravan secret."),
                      ("lava_fish_raw", "Raw Lava-Fish", "uncommon", 50, "Caught in Emberforge's channels."), ("goblin_ear", "Goblin Ear", "common", 4, "Guild bounty proof."), ("shadow_residue", "Shadow Residue", "uncommon", 35, "What a shadow soldier leaves behind."),
                      ("polished_gem", "Polished Gem", "rare", 120, "For fine settings.")]:
    mat(i, n, r, b, d)

# ---------------- quest items ----------------
for i, n, d in [("pip_ledger", "Guild Rank-Up Ledger", "Pip's very important, very leather-bound ledger."), ("lyra_notes", "Charred Research Page", "A page of Lyra's research, singed at the edges."),
                ("lost_tome_page", "Lost Grimoire Page", "A page that hums faintly in your hand.")]:
    add(id=i, name=n, type="key", rarity="uncommon", stack=20, desc=d)

# ---------------- tomes and key items ----------------
for i, n, skill, role, lv in [("tome_focus_strike", "Tome: Focus Strike", "focus_strike", "hero", 5), ("tome_wind_cutter", "Tome: Wind Cutter", "wind_cutter", "hero", 8), ("tome_fireball", "Tome: Fireball", "fireball", "mage", 3),
                              ("tome_starfall", "Tome: Starfall", "starfall", "mage", 18), ("tome_sanctuary", "Tome: Sanctuary", "sanctuary", "healer", 10), ("tome_radiant", "Tome: Radiant Renewal", "radiant_renewal", "healer", 20),
                              ("tome_unbroken_vow", "Tome: Unbroken Vow", "unbroken_vow", "tank", 20), ("tome_meteor", "Tome: Meteor", "meteor", "mage", 14)]:
    add(id=i, name=n, type="tome", role=role, level=lv, rarity="rare", stack=1, buy=0, sell=100, effect={"teach": skill}, desc=f"Teaches {skill.replace('_', ' ')}.")
for i, n, d in [("seal_life", "Seal Fragment of Life", "Vessara's fragment."), ("seal_time", "Seal Fragment of Time", "Khemet-Ur's fragment."), ("seal_spirit", "Seal Fragment of Spirit", "Isrevna's fragment."), ("seal_flame", "Seal Fragment of Flame", "Pyrrhax's fragment."),
                ("sealing_crown", "The Sealing Crown", "Reassembled. It remembers Aurel."), ("guild_card", "Adventurer Guild Card", "Proof of rank."), ("orwyn_letter", "Orwyn's Last Letter", "Words he did not get to say."), ("greywall_names", "Greywall Names", "Bram's squad, in his own hand.")]:
    add(id=i, name=n, type="key", rarity="legendary", stack=1, desc=d)

json.dump({"items": items}, open(os.path.join(out, "items.json"), "w"), indent=1)

# ---------------- enchants ----------------
ENCH = [
    ("flame", "Flame", ["weapon"], "fire", 0.15, {}, "spark_dust", 1), ("frost", "Frost", ["weapon"], "ice", 0.15, {}, "spirit_shard", 4), ("storm", "Storm", ["weapon"], "lightning", 0.15, {}, "time_sand", 3),
    ("radiance", "Radiance", ["weapon"], "holy", 0.15, {}, "dawn_fragment", 6), ("umbra", "Umbra", ["weapon"], "shadow", 0.15, {}, "flame_core", 5),
    ("keen", "Keen", ["weapon"], "", 0, {"crit": 0.05}, "spark_dust", 1), ("might", "Might", ["weapon", "accessory"], "", 0, {"atk": 8}, "life_essence", 2), ("sorcery", "Sorcery", ["weapon", "accessory"], "", 0, {"matk": 10}, "life_essence", 2),
    ("vitality", "Vitality", ["armor", "accessory"], "", 0, {"hp": 80}, "life_essence", 2), ("warding", "Warding", ["armor"], "", 0, {"armor": 5}, "time_sand", 3), ("clarity", "Clarity", ["armor", "accessory"], "", 0, {"mp": 50}, "spirit_shard", 4),
    ("swiftness", "Swiftness", ["armor", "accessory"], "", 0, {"agi": 4}, "flame_core", 5),
]
json.dump({"enchants": [dict(id=a, name=b, applies=c, element=d, element_boost=e, stats=f, essence=g, min_tier=h) for a, b, c, d, e, f, g, h in ENCH]}, open(os.path.join(out, "enchants.json"), "w"), indent=1)

# ---------------- recipes ----------------
rec = []
def recipe(rid, out_id, qty, inputs, station, gold, level=1):
    rec.append(dict(id=rid, output=out_id, qty=qty, inputs=[{"item": a, "qty": b} for a, b in inputs], station=station, gold=gold, level=level))
for i, (tk, reg, lv, rar, lab) in enumerate(TIERS[:5]):
    n = i + 1
    for role in ("hero", "tank", "mage", "healer"):
        wk = {"hero": "hero", "tank": "tank", "mage": "mage", "healer": "heal"}[role]
        recipe(f"craft_w_{wk}_{tk}", f"w_{wk}_{tk}", 1, [(ORE[tk], 4 + 2 * n), (HIDE[tk], 2 + n), (ESS[tk], 1 + n // 2)], "forge", 40 * n * n, lv)
        mat_ = HIDE[tk] if role in ("hero", "mage", "healer") else ORE[tk]
        recipe(f"craft_a_{role}_body_{tk}", f"a_{role}_body_{tk}", 1, [(mat_, 5 + 2 * n), (HIDE[tk], 3 + n), ("linen_thread", 2 + n)], "forge", 30 * n * n, lv)
        recipe(f"craft_a_{role}_head_{tk}", f"a_{role}_head_{tk}", 1, [(HIDE[tk], 3 + n), ("linen_thread", 1 + n)], "forge", 15 * n * n, lv)
        recipe(f"craft_a_{role}_boots_{tk}", f"a_{role}_boots_{tk}", 1, [(HIDE[tk], 3 + n), ("linen_thread", 1 + n)], "forge", 14 * n * n, lv)
recipe("craft_hp_small", "hp_small", 3, [("meadow_herb", 2), ("crystal_vial", 1)], "alchemy", 5)
recipe("craft_hp_medium", "hp_medium", 2, [("moonpetal", 2), ("meadow_herb", 1), ("crystal_vial", 1)], "alchemy", 15, 4)
recipe("craft_hp_large", "hp_large", 2, [("desert_bloom", 3), ("moonpetal", 1), ("crystal_vial", 1)], "alchemy", 40, 9)
recipe("craft_hp_grand", "hp_grand", 1, [("frostcap", 4), ("desert_bloom", 2), ("crystal_vial", 1)], "alchemy", 120, 14)
recipe("craft_hp_elixir", "hp_elixir", 1, [("ashroot", 4), ("hollow_lily", 2), ("flame_core", 1), ("crystal_vial", 1)], "alchemy", 400, 20)
recipe("craft_mp_small", "mp_small", 3, [("spark_dust", 1), ("meadow_herb", 1), ("crystal_vial", 1)], "alchemy", 8)
recipe("craft_mp_medium", "mp_medium", 2, [("life_essence", 1), ("moonpetal", 1), ("crystal_vial", 1)], "alchemy", 20, 4)
recipe("craft_mp_large", "mp_large", 2, [("time_sand", 1), ("desert_bloom", 2), ("crystal_vial", 1)], "alchemy", 60, 9)
recipe("craft_mp_grand", "mp_grand", 1, [("spirit_shard", 2), ("frostcap", 2), ("crystal_vial", 1)], "alchemy", 160, 14)
recipe("craft_mp_elixir", "mp_elixir", 1, [("flame_core", 2), ("hollow_lily", 2), ("dawn_fragment", 1), ("crystal_vial", 1)], "alchemy", 500, 20)
recipe("craft_antidote", "antidote", 2, [("meadow_herb", 2), ("crystal_vial", 1)], "alchemy", 6)
recipe("craft_holy_water", "holy_water", 1, [("moonpetal", 2), ("spark_dust", 1), ("crystal_vial", 1)], "alchemy", 30, 5)
recipe("craft_curse_salt", "curse_salt", 1, [("time_sand", 2), ("spirit_shard", 1)], "alchemy", 90, 12)
recipe("craft_phoenix", "phoenix_feather", 1, [("flame_core", 3), ("dawn_fragment", 1), ("polished_gem", 1)], "alchemy", 600, 22)
recipe("craft_dawn_crystal", "dawn_crystal", 1, [("spirit_shard", 2), ("polished_gem", 1)], "alchemy", 200, 14)
recipe("craft_elixir_might", "elixir_might", 1, [("life_essence", 1), ("wolf_pelt", 1), ("crystal_vial", 1)], "alchemy", 40, 5)
recipe("craft_elixir_guard", "elixir_guard", 1, [("life_essence", 1), ("dryad_bark", 1), ("crystal_vial", 1)], "alchemy", 40, 5)
recipe("craft_elixir_focus", "elixir_focus", 1, [("spark_dust", 2), ("moonpetal", 1), ("crystal_vial", 1)], "alchemy", 40, 5)
recipe("craft_elixir_vigor", "elixir_vigor", 1, [("life_essence", 1), ("meadow_herb", 2), ("crystal_vial", 1)], "alchemy", 40, 5)
recipe("craft_bomb_fire", "bomb_fire", 3, [("coal", 2), ("spark_dust", 1)], "alchemy", 20, 3)
recipe("craft_food_bread", "food_bread", 4, [("wild_flour", 2)], "cooking", 0)
recipe("craft_food_stew", "food_stew", 2, [("wolf_pelt", 0), ("meadow_herb", 1), ("river_fish", 1)], "cooking", 5)
recipe("craft_food_fish", "food_grilled_fish", 2, [("river_fish", 2), ("meadow_herb", 1)], "cooking", 4)
recipe("craft_food_flatbread", "food_desert_flatbread", 2, [("wild_flour", 2), ("spice_blend", 1)], "cooking", 12, 8)
recipe("craft_food_broth", "food_frost_broth", 2, [("frostcap", 1), ("river_fish", 1)], "cooking", 14, 14)
recipe("craft_food_lava", "food_lava_fish", 1, [("lava_fish_raw", 1), ("spice_blend", 1), ("ashroot", 1)], "cooking", 30, 18)
recipe("craft_polished_gem", "polished_gem", 1, [("spark_dust", 3), ("coal", 2)], "forge", 80, 6)
for r in rec:
    r["inputs"] = [i for i in r["inputs"] if i["qty"] > 0]
json.dump({"recipes": rec}, open(os.path.join(out, "recipes.json"), "w"), indent=1)

# ---------------- shops ----------------
shops = [
    {"id": "garrick", "name": "Garrick Thorne's Forge", "keeper": "Garrick Thorne", "region": "Aurelheim", "sells": ["w_hero_t1", "w_hero_club_t1", "w_hero_dagger_t2", "w_tank_t1", "w_mage_t1", "w_heal_t1"] + [f"a_{r}_{s}_t1" for r in ("hero", "tank", "mage", "healer") for s in ("body", "head", "boots")] + ["coal", "iron_ore", "linen_thread"], "buys": ["weapon", "armor", "accessory", "material"]},
    {"id": "selwyn", "name": "Madame Selwyn's Potions", "keeper": "Madame Selwyn", "region": "Aurelheim", "sells": ["hp_small", "hp_medium", "hp_large", "mp_small", "mp_medium", "mp_large", "antidote", "holy_water", "crystal_vial", "meadow_herb", "elixir_might", "elixir_guard"], "buys": ["consumable", "material"]},
    {"id": "guild_store", "name": "Guild Supply", "keeper": "Pip Marlowe", "region": "Aurelheim", "sells": ["acc_guild_ring", "acc_lucky_coin", "acc_hunters_tooth", "acc_pilgrims_beads", "food_bread", "food_stew", "bomb_fire"], "buys": ["accessory", "consumable"]},
    {"id": "zaira", "name": "Zaira's Caravan", "keeper": "Zaira", "region": "Sunscar Sands", "sells": ["w_hero_t3", "w_tank_t3", "w_mage_t3", "w_heal_t3", "w_hero_axe_t3", "acc_sandglass_charm", "acc_scorpion_band", "spice_blend", "food_desert_flatbread", "hp_large", "mp_large", "desert_bloom", "sun_bronze"], "buys": ["weapon", "armor", "accessory", "material", "consumable"]},
    {"id": "hrimvald", "name": "Hrimvald Outfitters", "keeper": "Skald Torvik", "region": "Frostspire Peaks", "sells": ["w_hero_t4", "w_tank_t4", "w_mage_t4", "w_heal_t4", "w_hero_spear_t4", "acc_rime_ring", "food_frost_broth", "hp_grand", "mp_grand", "frostcap", "frost_silver"], "buys": ["weapon", "armor", "accessory", "material", "consumable"]},
    {"id": "brandt", "name": "Brandt Cinder's Forge", "keeper": "Forgemaster Brandt Cinder", "region": "Cinderhold", "sells": ["w_hero_t5", "w_tank_t5", "w_mage_t5", "w_heal_t5", "acc_forge_signet", "acc_cinder_amulet", "ember_steel", "coal", "food_lava_fish", "hp_elixir", "mp_elixir"], "buys": ["weapon", "armor", "accessory", "material"]},
    {"id": "old_fenn", "name": "Old Fenn's Hut", "keeper": "Old Fenn", "region": "Whisperwood", "sells": ["w_hero_t2", "w_tank_t2", "w_mage_t2", "w_heal_t2", "moonpetal", "meadow_herb", "dryad_bark", "heartwood", "acc_moonpetal_locket", "life_essence"], "buys": ["material", "consumable"]},
]
json.dump({"shops": shops}, open(os.path.join(out, "shops.json"), "w"), indent=1)

# ---------------- loot ----------------
loot = [
    {"enemy": "shadow_soldier", "gold": [8, 18], "drops": [{"item": "shadow_residue", "chance": 0.45, "qty": [1, 2]}, {"item": "iron_ore", "chance": 0.30, "qty": [1, 2]}, {"item": "hp_small", "chance": 0.20, "qty": [1, 1]}, {"item": "spark_dust", "chance": 0.08, "qty": [1, 1]}, {"item": "acc_lucky_coin", "chance": 0.01, "qty": [1, 1]}]},
    {"enemy": "shadow_archer", "gold": [12, 26], "drops": [{"item": "shadow_residue", "chance": 0.50, "qty": [1, 2]}, {"item": "linen_thread", "chance": 0.30, "qty": [1, 3]}, {"item": "mp_small", "chance": 0.20, "qty": [1, 1]}, {"item": "wolf_pelt", "chance": 0.20, "qty": [1, 1]}, {"item": "acc_swift_anklet", "chance": 0.02, "qty": [1, 1]}]},
    {"enemy": "bandit_thug", "gold": [6, 16], "drops": [{"item": "pip_ledger", "chance": 0.35, "qty": [1, 1]}, {"item": "hp_small", "chance": 0.2, "qty": [1, 1]}, {"item": "iron_ore", "chance": 0.25, "qty": [1, 2]}]},
    {"enemy": "goblin_grunt", "gold": [4, 10], "drops": [{"item": "goblin_ear", "chance": 0.6, "qty": [1, 1]}, {"item": "iron_ore", "chance": 0.3, "qty": [1, 2]}, {"item": "coal", "chance": 0.3, "qty": [1, 2]}]},
    {"enemy": "wolf", "gold": [0, 4], "drops": [{"item": "wolf_pelt", "chance": 0.5, "qty": [1, 1]}, {"item": "meadow_herb", "chance": 0.15, "qty": [1, 1]}]},
    {"enemy": "rootling", "gold": [6, 14], "drops": [{"item": "dryad_bark", "chance": 0.4, "qty": [1, 2]}, {"item": "lost_tome_page", "chance": 0.22, "qty": [1, 1]}, {"item": "moonpetal", "chance": 0.2, "qty": [1, 1]}]},
    {"enemy": "thorn_minion", "gold": [8, 18], "drops": [{"item": "dryad_bark", "chance": 0.4, "qty": [1, 2]}, {"item": "lost_tome_page", "chance": 0.2, "qty": [1, 1]}, {"item": "life_essence", "chance": 0.1, "qty": [1, 1]}]},
    {"enemy": "cult_acolyte", "gold": [10, 24], "drops": [{"item": "lyra_notes", "chance": 0.3, "qty": [1, 1]}, {"item": "shadow_residue", "chance": 0.4, "qty": [1, 2]}]},
    {"enemy": "cult_zealot", "gold": [10, 24], "drops": [{"item": "shadow_residue", "chance": 0.5, "qty": [1, 2]}, {"item": "lyra_notes", "chance": 0.12, "qty": [1, 1]}, {"item": "mp_small", "chance": 0.2, "qty": [1, 1]}]},
    {"enemy": "scorpion", "gold": [8, 18], "drops": [{"item": "scorpion_chitin", "chance": 0.5, "qty": [1, 2]}, {"item": "desert_bloom", "chance": 0.12, "qty": [1, 1]}]},
    {"enemy": "sand_serpent", "gold": [10, 22], "drops": [{"item": "scorpion_chitin", "chance": 0.25, "qty": [1, 1]}, {"item": "time_sand", "chance": 0.06, "qty": [1, 1]}]},
    {"enemy": "tomb_wraith", "gold": [12, 26], "drops": [{"item": "time_sand", "chance": 0.35, "qty": [1, 1]}, {"item": "spice_blend", "chance": 0.12, "qty": [1, 1]}]},
    {"enemy": "caravan_raider", "gold": [12, 28], "drops": [{"item": "spice_blend", "chance": 0.2, "qty": [1, 1]}, {"item": "hp_medium", "chance": 0.25, "qty": [1, 1]}, {"item": "sun_bronze", "chance": 0.15, "qty": [1, 1]}]},
    {"enemy": "frost_wolf", "gold": [10, 22], "drops": [{"item": "yeti_fur", "chance": 0.5, "qty": [1, 1]}, {"item": "frostcap", "chance": 0.12, "qty": [1, 1]}]},
    {"enemy": "ice_wraith", "gold": [14, 30], "drops": [{"item": "spirit_shard", "chance": 0.3, "qty": [1, 1]}, {"item": "frostcap", "chance": 0.2, "qty": [1, 2]}]},
    {"enemy": "choir_husk", "gold": [14, 30], "drops": [{"item": "spirit_shard", "chance": 0.25, "qty": [1, 1]}, {"item": "frost_silver", "chance": 0.15, "qty": [1, 1]}]},
    {"enemy": "magma_golem", "gold": [30, 60], "drops": [{"item": "magma_scale", "chance": 0.6, "qty": [1, 2]}, {"item": "ember_steel", "chance": 0.4, "qty": [1, 2]}, {"item": "coal", "chance": 0.6, "qty": [2, 4]}]},
    {"enemy": "ember_imp", "gold": [14, 30], "drops": [{"item": "coal", "chance": 0.5, "qty": [1, 3]}, {"item": "flame_core", "chance": 0.07, "qty": [1, 1]}, {"item": "lava_fish_raw", "chance": 0.08, "qty": [1, 1]}]},
    {"enemy": "demon_legionary", "gold": [20, 44], "drops": [{"item": "shadow_residue", "chance": 0.5, "qty": [1, 3]}, {"item": "void_crystal", "chance": 0.08, "qty": [1, 1]}, {"item": "hp_grand", "chance": 0.1, "qty": [1, 1]}]},
    {"enemy": "greywall_ghost", "gold": [8, 20], "drops": [{"item": "spark_dust", "chance": 0.2, "qty": [1, 1]}, {"item": "iron_ore", "chance": 0.3, "qty": [1, 2]}]},
    {"enemy": "shadow_brute", "gold": [40, 80], "drops": [{"item": "shadow_residue", "chance": 0.80, "qty": [2, 4]}, {"item": "iron_ore", "chance": 0.60, "qty": [2, 4]}, {"item": "hp_medium", "chance": 0.40, "qty": [1, 2]}, {"item": "spark_dust", "chance": 0.30, "qty": [1, 2]}, {"item": "w_hero_dagger_t2", "chance": 0.04, "qty": [1, 1]}, {"item": "acc_oath_knot", "chance": 0.03, "qty": [1, 1]}]},
]
json.dump({"loot": loot}, open(os.path.join(out, "loot.json"), "w"), indent=1)
print(len(items), "items,", len(rec), "recipes,", len(ENCH), "enchants,", len(shops), "shops")
