"""Tiny authoring DSL for story content. Everything player-visible becomes a string id in strings_en.json.

convo(tree_id, steps, when=[...])  registers a dialogue tree with one entry.
  step forms:   ("speaker", "text")                 a line
                ("speaker", "text", {"do": [...], "if": [...]})   a line with actions / a condition on showing it
                ("choice", [("text", {"do": [...], "if": [...], "then": [steps...]}), ...])   attaches choices to the previous line
Sub-conversations under "then" are lists of steps (no nested choices needed, but allowed).
"""
STR = {}
NODES = []
TREES = []

def S(sid, text):
    assert sid not in STR, "duplicate string id " + sid
    STR[sid] = text
    return sid

def _chain(prefix, steps, counter):
    """Create nodes for steps; returns (first_node_id or "", last_node_obj or None)."""
    first, prev = "", None
    for st in steps:
        if st[0] == "choice":
            assert prev is not None, "choice needs a preceding line"
            prev["choices"] = []
            for ci, (ctext, copts) in enumerate(st[1]):
                cid = S(f"{prefix}.c{counter[0]}_{ci}", ctext)
                nxt = copts.get("goto", "")
                if copts.get("then"):
                    counter[0] += 1
                    nxt, _ = _chain(prefix, copts["then"], counter)
                ch = {"text": cid, "next": nxt}
                if copts.get("if"): ch["if"] = copts["if"]
                if copts.get("do"): ch["do"] = copts["do"]
                prev["choices"].append(ch)
            continue
        sp, text = st[0], st[1]
        opts = st[2] if len(st) > 2 else {}
        counter[0] += 1
        nid = f"{prefix}.{counter[0]}"
        node = {"id": nid, "speaker": sp, "text": S(nid, text), "next": ""}
        if opts.get("if"): node["if"] = opts["if"]
        if opts.get("do"): node["do"] = opts["do"]
        NODES.append(node)
        if prev is not None and "choices" not in prev: prev["next"] = nid
        if not first: first = nid
        prev = node
    return first, prev

def nodes(prefix, steps):
    """Create a reusable chain of nodes (no tree); returns the first node id."""
    first, _ = _chain(prefix, steps, [0])
    return first

def convo(tid, steps, when=None):
    first, _ = _chain(tid, steps, [0])
    TREES.append({"id": tid, "entries": [{"if": when or [], "node": first}]})
    return tid

def multi(tid, variants):
    """variants: list of (conds, steps); first matching wins (put the fallback last with conds=[])."""
    entries = []
    for vi, (conds, steps) in enumerate(variants):
        first, _ = _chain(f"{tid}.v{vi}", steps, [0])
        entries.append({"if": conds, "node": first})
    TREES.append({"id": tid, "entries": entries})
    return tid


def scene(npc_id, tid, conds, steps):
    """Prepend a quest-specific scene to an NPC's greeting tree (first matching entry wins)."""
    first, _ = _chain(tid, steps, [0])
    for t in TREES:
        if t["id"] == f"greet.{npc_id}":
            t["entries"].insert(0, {"if": conds, "node": first})
            return tid
    raise KeyError("no greeting tree for " + npc_id)
