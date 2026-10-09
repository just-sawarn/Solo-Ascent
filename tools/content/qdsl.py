"""Quest authoring helpers."""
from dsl import S, convo

QUESTS = []

def obj(kind, target, count, text, marker=""):
    return (kind, target, count, text, marker)

def offer(npc, lines, qid, accept="I'll take care of it.", decline="Not right now."):
    steps = [(npc, t) for t in lines]
    steps.append(("choice", [(accept, {"do": [f"quest.start:{qid}"], "then": [(npc, "Good. I'm counting on you.")]}), (decline, {})]))
    return steps

def done(npc, lines, qid):
    steps = [(npc, t) for t in lines]
    last = list(steps[-1]) + [{}] if len(steps[-1]) == 2 else list(steps[-1])
    last[2] = dict(last[2]); last[2]["do"] = [f"quest.complete:{qid}"] + last[2].get("do", [])
    steps[-1] = tuple(last)
    return steps

def quest(id, type, title, summary, objs, giver="auto", turn_in="auto", region="", act=1, number=0, prereq=None, rewards=None,
          offer_lines=None, progress_lines=None, done_lines=None, auto_start=False, repeatable=False, on_complete=None, on_start=None,
          level=1, accept="I'll take care of it.", decline="Not right now.", done_steps=None, offer_steps=None):
    S(f"q.{id}.title", title); S(f"q.{id}.sum", summary)
    q = {"id": id, "type": type, "act": act, "number": number, "giver": giver, "turn_in": turn_in, "region": region, "auto_start": auto_start,
         "repeatable": repeatable, "level": level, "title": f"q.{id}.title", "summary": f"q.{id}.sum", "prereq": prereq or [], "objectives": [],
         "rewards": rewards or {}, "on_complete": on_complete or [], "on_start": on_start or []}
    for i, (kind, target, count, text, marker) in enumerate(objs):
        S(f"q.{id}.o{i}", text)
        q["objectives"].append({"kind": kind, "target": target, "count": count, "text": f"q.{id}.o{i}", "marker": marker})
    if giver not in ("auto", "none") and not auto_start:
        steps = offer_steps or offer(giver, offer_lines or ["I need a hand with something."], id, accept, decline)
        q["offer_tree"] = convo(f"{id}.offer", steps)
        q["progress_tree"] = convo(f"{id}.progress", [(giver, line) for line in (progress_lines or ["Still at it? I'll be here when you're done."])])
    if turn_in not in ("auto", "none"):
        steps = done_steps or done(turn_in, done_lines or ["Well done. That's exactly what I needed."], id)
        q["complete_tree"] = convo(f"{id}.done", steps)
        if "progress_tree" not in q:
            q["progress_tree"] = convo(f"{id}.progress", [(turn_in, line) for line in (progress_lines or ["Not done yet? Take your time."])])
    QUESTS.append(q)
    return q

def R(xp=0, gold=0, items=None, skills=None, bond=None, rep=None):
    r = {"xp": xp, "gold": gold}
    if items: r["items"] = [{"item": i, "qty": n} for i, n in items]
    if skills: r["skills"] = [{"member": m, "skill": s} for m, s in skills]
    if bond: r["bond"] = [{"name": n, "n": v} for n, v in bond]
    if rep: r["rep"] = {"region": rep[0], "n": rep[1]}
    return r
