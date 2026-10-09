#!/usr/bin/env python3 -I
"""Story content generator: writes data/locations.json, npcs.json, quests.json, dialogue.json and strings_en.json.
Edit the modules in tools/content/, then run:  python3 -I tools/gen_story.py data"""
import json, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "content"))
out = sys.argv[1]
import dsl, world, npcs, npc_named, npc_town, cutscenes, main_quests, companion_quests, side_quests, bounties, qdsl

def dump(name, obj):
    json.dump(obj, open(os.path.join(out, name), "w"), indent=1, ensure_ascii=False)

dump("locations.json", {"locations": world.LOCS})
dump("npcs.json", {"npcs": npcs.NPCS})
dump("quests.json", {"quests": qdsl.QUESTS})
dump("dialogue.json", {"trees": dsl.TREES, "nodes": dsl.NODES})
dump("strings_en.json", dsl.STR)
by_type = {}
for q in qdsl.QUESTS: by_type[q["type"]] = by_type.get(q["type"], 0) + 1
print(len(world.LOCS), "locations,", len(npcs.NPCS), "npcs,", len(qdsl.QUESTS), "quests", by_type, ",", len(dsl.TREES), "dialogue trees,", len(dsl.NODES), "nodes,", len(dsl.STR), "strings")
