"""NPC roster: 27 named characters (24 from the story bible + the three recruitable companions) and 44 archetype townsfolk.
Each NPC has a personality line, a 3-slot schedule, four stage-dependent greetings and a unique chat conversation."""
import zlib
from dsl import S, convo, multi, nodes

def h(s): return zlib.crc32(s.encode())

NPCS = []
STAGE_STD = [("post", ["quest.MQ24 == complete"]), ("late", ["quest.MQ20 == complete"]), ("mid", ["quest.MQ09 == complete"]), ("early", [])]
STAGE_ASH = [("post", ["quest.MQ24 == complete"]), ("late", ["quest.MQ20 == complete"]), ("mid", ["quest.MQ03 == complete"]), ("early", [])]

# schedule helpers: work spot per archetype region; home row behind the buildings; tavern = the inn
INN = {"aurelheim": (-12, 26), "ashbrook": (-6, 20), "whisperwood": (4, 16), "qasr_amun": (-8, 22), "hrimvald": (-10, 22), "emberforge": (10, 24), "front_camp": (-4, 20)}

def npc(id, name, region, role, arch, sex, hair, hc, top, bot, shoes, pers, work, g, chat, stage="std", shop="", trainer="", skin=0, scale=1.0, required=False, eye=(70, 60, 50), home=None, tavern=None, night=None, visible=None):
    hx, hz = home if home else (work[0] + (3 if work[0] < 0 else -3), 33 + (h(id) % 5))
    tv = tavern or INN.get(region, (-10, 24))
    nightplace = night or ("tavern" if role in ("shopkeeper", "innkeeper", "smith", "merchant", "guard", "miner", "rival", "bard", "adventurer", "cook", "fisher", "farmer", "stable", "hunter", "apprentice") else "home")
    sched = [{"place": "home", "x": hx, "z": hz}, {"place": "work", "x": work[0], "z": work[1]},
             {"place": nightplace, "x": tv[0] + (h(id) % 3) - 1, "z": tv[1] + (h(id) % 3) - 1} if nightplace == "tavern" else {"place": "home", "x": hx, "z": hz}]
    pid = S(f"npc.{id}.personality", pers)
    # reusable chat conversation
    chat_first = nodes(f"chat.{id}", chat)
    variants = []
    for sname, conds in (STAGE_ASH if stage == "ash" else STAGE_STD):
        line = g[{"early": 0, "mid": 1, "late": 2, "post": 3}[sname]]
        choices = [("Got a moment to talk?", {"goto": chat_first})]
        if shop: choices.append(("Let me see your wares.", {"do": [f"shop.open:{shop}"]}))
        if trainer: choices.append(("Can you train me?", {"then": [(id, f"Prove yourself first. Look for the trial that goes with '{trainer.replace('_', ' ')}' and come back with a story to tell.")]}))
        choices.append(("Take care.", {}))
        variants.append((conds, [(id, line), ("choice", choices)]))
    tree = multi(f"greet.{id}", variants)
    NPCS.append({"id": id, "name": name, "region": region, "role": role, "archetype": arch, "personality": pid, "shop": shop, "trainer": trainer, "tree": tree,
                 "sex": sex, "hair": hair, "hair_col": list(hc), "top": list(top), "bottom": list(bot), "shoes": list(shoes), "eye": list(eye), "skin": skin,
                 "scale": scale, "required": required, "schedule": sched, "visible_if": visible or []})

# colour shorthands
BLK, BRN, GRY, WHT, RED, GLD, SLV, AUB = (28, 24, 30), (88, 62, 40), (150, 150, 160), (225, 220, 205), (140, 50, 40), (200, 160, 70), (190, 196, 215), (120, 60, 40)
def P(*a): return tuple(a)
