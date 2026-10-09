"""Locations. map = position on the world map (0..1), local = (x, z, radius) in the hosting scene."""
LOCS = []
def L(id, name, region, kind, mx, my, local=(0, 0, 6), sandbox=False):
    LOCS.append({"id": id, "name": name, "region": region, "kind": kind, "map": [mx, my], "local": list(local), "sandbox": sandbox})

REGIONS = [
    ("vale", "Ashbrook Vale and the Whisperwood", 0.27, 0.48),
    ("sunscar", "Sunscar Sands", 0.48, 0.82),
    ("frostspire", "Frostspire Peaks", 0.56, 0.17),
    ("cinderhold", "Cinderhold", 0.79, 0.52),
    ("hollow", "The Hollow Reach", 0.89, 0.22),
]

# --- Ashbrook Vale / Whisperwood ---
L("ashbrook", "Ashbrook Village", "vale", "town", 0.18, 0.62)
L("ashbrook_shrine", "Ashbrook Shrine", "vale", "poi", 0.17, 0.60)
L("ashbrook_outskirts", "Ashbrook Outskirts", "vale", "poi", 0.15, 0.66)
L("refugee_camp", "Refugee Camp", "vale", "town", 0.22, 0.57)
L("aurelheim", "Aurelheim", "vale", "town", 0.30, 0.50)
L("aurelheim_gates", "Aurelheim Gates", "vale", "poi", 0.28, 0.54, (0, 40, 6), True)
L("aurelheim_plaza", "Aurelheim Plaza", "vale", "poi", 0.30, 0.50, (0, 18, 7), True)
L("guild_hall", "Adventurer Guild Hall", "vale", "poi", 0.31, 0.49, (-12, 20, 4), True)
L("aurelheim_cathedral", "Cathedral of the Dawn", "vale", "poi", 0.32, 0.48, (12, 22, 5), True)
L("aurelheim_market", "Aurelheim Market", "vale", "poi", 0.29, 0.51, (0, 27, 6), True)
L("aurelheim_arena", "Rank-Up Arena", "vale", "poi", 0.33, 0.52, (-14, 8, 6), True)
L("aurelheim_archive", "Aurelheim Archive", "vale", "poi", 0.30, 0.47, (14, 12, 4), True)
L("gilded_lantern", "The Gilded Lantern Inn", "vale", "poi", 0.29, 0.50, (-12, 26, 3), True)
L("convergence_shrine", "Convergence Shrine", "vale", "shrine", 0.36, 0.50)
L("sanctum_of_dawn", "Sanctum of Dawn", "vale", "poi", 0.28, 0.56)
L("spire_academy", "Spire Academy", "vale", "town", 0.40, 0.40)
L("forbidden_library", "The Forbidden Library", "vale", "dungeon", 0.41, 0.38)
L("whisperwood_edge", "Whisperwood Edge", "vale", "poi", 0.24, 0.42)
L("whisperwood", "Whisperwood Forest", "vale", "town", 0.22, 0.38)
L("fenns_hut", "Old Fenn's Hut", "vale", "poi", 0.21, 0.40)
L("fort_greywall", "Fort Greywall", "vale", "poi", 0.34, 0.30)
L("greywall_memorial", "Greywall Memorial", "vale", "poi", 0.34, 0.29)
L("goblin_warrens", "Goblin Warrens", "vale", "dungeon", 0.26, 0.66)
L("rootheart_hollow", "Rootheart Hollow", "vale", "dungeon", 0.20, 0.30)
L("moonpetal_glade", "Moonpetal Glade", "vale", "poi", 0.19, 0.35)
L("monks_clearing", "The Monk's Clearing", "vale", "poi", 0.23, 0.33)
L("lamb_meadow", "Lamb Meadow", "vale", "poi", 0.20, 0.58)
# --- Sunscar ---
L("qasr_amun", "Qasr Amun", "sunscar", "town", 0.38, 0.78)
L("oasis_market", "Qasr Amun Bazaar", "sunscar", "poi", 0.39, 0.77)
L("khemet_ruins", "Sunken Ruins of Khemet", "sunscar", "town", 0.46, 0.86)
L("lost_mural", "The Lost Mural", "sunscar", "poi", 0.47, 0.88)
L("scorpion_catacombs", "Scorpion Catacombs", "sunscar", "dungeon", 0.52, 0.80)
L("hourglass_vault", "The Hourglass Vault", "sunscar", "dungeon", 0.55, 0.90)
L("caravan_road", "Caravan Road", "sunscar", "poi", 0.42, 0.72)
L("serpent_dunes", "Serpent Dunes", "sunscar", "poi", 0.44, 0.80)
# --- Frostspire ---
L("hrimvald", "Hrimvald", "frostspire", "town", 0.55, 0.18)
L("whiteout_pass", "Whiteout Pass", "frostspire", "dungeon", 0.48, 0.24)
L("choir_cathedral", "The Silent Choir Cathedral", "frostspire", "dungeon", 0.62, 0.10)
L("frozen_lighthouse", "The Frozen Lighthouse", "frostspire", "poi", 0.52, 0.12)
L("climbers_camp", "Climbers' Camp", "frostspire", "poi", 0.50, 0.21)
L("singing_ice", "The Singing Ice", "frostspire", "poi", 0.58, 0.14)
# --- Cinderhold ---
L("emberforge", "Emberforge", "cinderhold", "town", 0.78, 0.52)
L("ashen_mines", "Ashen Mines", "cinderhold", "dungeon", 0.72, 0.62)
L("molten_keep", "The Molten Keep", "cinderhold", "dungeon", 0.86, 0.44)
L("singing_forge", "The Singing Anvil Forge", "cinderhold", "poi", 0.80, 0.54)
L("lava_channels", "Lava Channels", "cinderhold", "poi", 0.76, 0.46)
# --- Hollow Reach ---
L("hollow_gate", "The Hollow Gate", "hollow", "poi", 0.86, 0.22)
L("bone_bridge", "Bone Bridge Approach", "hollow", "dungeon", 0.90, 0.30)
L("nox_hideout", "Nox's Hideout", "hollow", "poi", 0.88, 0.26)
L("front_camp", "The Front Camp", "hollow", "poi", 0.84, 0.25)
L("voidspire_castle", "Voidspire Castle", "hollow", "dungeon", 0.94, 0.14)
L("voidspire_throne", "The Voidspire Throne", "hollow", "poi", 0.95, 0.12)
