from dsl import convo, scene
from qdsl import quest, obj, R, offer, done
from cutscenes import cs

def cq(id, who, n, title, summary, objs, prereq, intro, outro, rewards, region="vale", gate=None, member_bond=None, on_complete=None, giver="auto", turn_in="auto", **kw):
    pre = [f"bond.{who} >= {n}"] + prereq
    cs(f"{id.lower()}_intro", intro)
    cs(f"{id.lower()}_outro", outro)
    quest(id, "companion", title, summary, objs, giver=giver, turn_in=turn_in, region=region, act=kw.pop("act", 3), prereq=pre, auto_start=True, rewards=rewards,
          on_start=[f"cutscene:{id.lower()}_intro"], on_complete=[f"cutscene:{id.lower()}_outro"] + (on_complete or []), **kw)

# ===================== BRAM: Vows of the Shield =====================
cq("CQ-Bram1", "bram", 1, "Names on the Wall", "Visit Greywall's memorial and recover the names of Bram's fallen squad.",
   [obj("explore", "greywall_memorial", 1, "Visit the Greywall memorial", "greywall_memorial"), obj("kill", "greywall_ghost", 4, "Clear the restless spirits from the memorial", "greywall_memorial")],
   ["quest.MQ11 == complete"],
   [("bram", "The memorial. I haven't been back. Forty names, and I don't remember how to spell half of them."), ("kael", "We'll find them. We'll write them down."), ("bram", "...Aye. Behind me, kid. It's thick with ghosts.")],
   [("bram", "Thank you. That's... forty names. I read them aloud. Every one. They sounded like people, instead of shapes."), ("narrator", "(Bram's shield now bears a line of small engraved names.)"), ("bram", "I'll carry them. For good this time.")],
   R(xp=1800, gold=200, items=[("greywall_names", 1), ("acc_greywall_tag", 1)], bond=[("bram", 1)]))
cq("CQ-Bram2", "bram", 2, "The Last Letter", "Deliver a fallen soldier's last letter to the family he left behind.",
   [obj("talk", "ilse", 1, "Deliver the letter to Ilse in Aurelheim", "aurelheim_market")],
   ["quest.CQ-Bram1 == complete", "quest.MQ14 == complete"],
   [("bram", "One of the letters in Greywall's ledger was never sent. Corporal Hale. His daughter lives in Aurelheim. Herbalist, I'm told."), ("kael", "Do you want me to deliver it?"), ("bram", "No. I want to hand it over myself. I just don't know if my legs will go.")],
   [("ilse", "He wrote that he was proud of me. That he wasn't afraid. That he wished the captain wouldn't blame himself."), ("bram", "I did blame myself."), ("ilse", "Then let me tell you what a daughter's allowed to say. I forgive you. Please stop.")],
   R(xp=2200, gold=250, bond=[("bram", 1)]), act=4)
cq("CQ-Bram3", "bram", 3, "The Captain's Return", "Reclaim the stolen Royal Shield Guard banner and face the officer who blamed him.",
   [obj("explore", "caravan_road", 1, "Track the raiders who looted Greywall", "caravan_road"), obj("kill", "caravan_raider", 6, "Defeat the raiders and take back the banner", "caravan_road"), obj("talk", "dunmore", 1, "Present the banner to Captain Dunmore", "aurelheim")],
   ["quest.CQ-Bram2 == complete", "quest.MQ17 == complete"],
   [("bram", "Raiders have the Shield Guard's banner. They hung it over a campfire. I can't let that stand."), ("lyra", "And then you'll finally face Dunmore."), ("bram", "She was my commanding officer. She wrote the report that blamed me. I've been afraid of that report for years.")],
   [("dunmore", "Captain Ironvale. That banner is the one I swore on. I wrote that report in anger. I should have burned it."), ("bram", "I deserved it."), ("dunmore", "No. You deserved a better commander. Welcome back to the Guard. If you'll have us.")],
   R(xp=3200, gold=400, items=[("a_tank_head_t3", 1)], bond=[("bram", 1)]), act=4)
cq("CQ-Bram4", "bram", 4, "Hold the Line", "Defend Emberforge's refugees in a wave defense while the party buys time.",
   [obj("explore", "emberforge", 1, "Take up position at Emberforge's south gate", "emberforge"), obj("kill", "ember_imp", 10, "Hold off the imp waves", "emberforge"), obj("kill", "magma_golem", 1, "Break the golem's charge", "emberforge")],
   ["quest.CQ-Bram3 == complete", "quest.MQ21 == complete"],
   [("bram", "Refugees on the south road. A couple hundred. Imps are going to hit them in an hour. I won't let this one slip."), ("kael", "How can I help?"), ("bram", "Behind me, kid. All of you. We hold, and we hold, and we don't stop until the last cart is through.")],
   [("bram", "Last cart's through. No one left behind. Nobody."), ("narrator", "(Bram plants his shield and, for the first time, smiles.)"), ("bram", "That's what the vow was for. Not to carry the dead. To protect the living. I get it now.")],
   R(xp=7000, gold=800, skills=[("bram", "unbroken_vow")], bond=[("bram", 1)]), act=4)

# ===================== LYRA: The Girl Who Held the Storm =====================
cq("CQ-Lyra1", "lyra", 1, "Ink and Ashes", "Recover Lyra's burnt research notes from the Academy ruins.",
   [obj("explore", "spire_academy", 1, "Search the burnt east wing", "spire_academy"), obj("collect", "lyra_notes", 5, "Gather five charred pages of Lyra's research", "spire_academy")],
   ["quest.MQ12 == complete"],
   [("lyra", "My notes. Four years of research, and I torched them with the east wing. I want them back."), ("kael", "Where do we look?"), ("lyra", "The cultists from the festival had pages. Hunt them down. Don't read them. They're embarrassing.")],
   [("lyra", "Five pages. Four years. I thought it would hurt to read them."), ("lyra", "It doesn't. They're just... a girl, trying very hard."), ("kael", "A very smart girl."), ("lyra", "Don't flatter me. I'll start believing you.")],
   R(xp=2000, gold=200, items=[("tome_fireball", 1)], bond=[("lyra", 1)]))
cq("CQ-Lyra2", "lyra", 2, "The Academy's Apology", "Confront Dean Voss about Lyra's exile.",
   [obj("explore", "spire_academy", 1, "Return to the Spire Academy", "spire_academy"), obj("talk", "voss", 1, "Confront Dean Voss", "spire_academy")],
   ["quest.CQ-Lyra1 == complete", "quest.MQ15 == complete"],
   [("lyra", "I'm going to talk to Voss. If I go alone I'll start a fire. If you come, I'll start a speech."), ("kael", "I'll come.")],
   [("voss", "I exiled you because I was afraid of what you might become. I was wrong. The only danger in that wing was my reluctance to ask for help."), ("lyra", "That's the most words you've ever said to me."), ("voss", "I practiced.")],
   R(xp=2800, gold=300, bond=[("lyra", 1)]), act=4)
cq("CQ-Lyra3", "lyra", 3, "Control Is Not Fear", "A meditation and duel challenge in a mana storm at the Singing Ice.",
   [obj("explore", "singing_ice", 1, "Reach the Singing Ice", "singing_ice"), obj("kill", "ice_wraith", 5, "Hold your focus against the storm-wraiths", "singing_ice")],
   ["quest.CQ-Lyra2 == complete", "quest.MQ18 == complete"],
   [("lyra", "Everyone says control means gripping harder. I think they're wrong. I think control is letting go and trusting someone to catch it."), ("bram", "I'll catch it."), ("lyra", "I know. That's why I'm trying.")],
   [("lyra", "I didn't clamp down. I opened up. The storm just... sat in my hands, like a cat."), ("kael", "A very large cat."), ("lyra", "A very polite cat. Don't hold back or I'll be insulted. I'm not holding back either.")],
   R(xp=4500, gold=500, bond=[("lyra", 1)]), act=4)
cq("CQ-Lyra4", "lyra", 4, "The Spell Without a Name", "Help Lyra create her own signature spell.",
   [obj("explore", "convergence_shrine", 1, "Meet Lyra at the Convergence Shrine", "convergence_shrine"), obj("collect", "spirit_shard", 2, "Gather two Spirit Shards to anchor the spell", "singing_ice")],
   ["quest.CQ-Lyra3 == complete", "quest.MQ22 == complete"],
   [("lyra", "I want to make a spell. Not copy one. My own. I need you to hold the other end of the rope."), ("kael", "I thought that was Bram's job."), ("bram", "I'm the rope. You're the knot.")],
   [("lyra", "Stars, falling. Not as a weapon. As a... conversation."), ("kael", "Starfall."), ("lyra", "Starfall. Yes. Hold on, I'm going to cry a little. Don't look.")],
   R(xp=9000, gold=1000, skills=[("lyra", "starfall")], bond=[("lyra", 1)]), act=4)

# ===================== MIRELLE: A Light Worth Keeping =====================
cq("CQ-Mirelle1", "mirelle", 1, "Cost of the Light", "Investigate the records of the Vital Tithe.",
   [obj("talk", "penn", 1, "Ask Archivist Penn for the Tithe ledgers", "aurelheim_archive"), obj("explore", "aurelheim_archive", 1, "Read the Church's records in the Archive", "aurelheim_archive")],
   ["quest.MQ13 == complete"],
   [("mirelle", "I'd like to see the ledgers. Not for you. For me. I think I've been told something that isn't quite the truth."), ("kael", "I'll come with you."), ("mirelle", "Please.")],
   [("mirelle", "...Every Tithe. Every priest and priestess for nine centuries. A column of names, and next to it, a number. Years. They subtracted our years."), ("bram", "That's theft."), ("mirelle", "It's worse. They called it love.")],
   R(xp=2200, gold=200, bond=[("mirelle", 1)]), act=3)
cq("CQ-Mirelle2", "mirelle", 2, "The Village She Healed", "Revisit a village that was healed by Mirelle years ago.",
   [obj("explore", "refugee_camp", 1, "Travel to the refugee camp", "refugee_camp"), obj("talk", "wren", 1, "Meet Granny Wren, who remembers her", "refugee_camp")],
   ["quest.CQ-Mirelle1 == complete", "quest.MQ16 == complete"],
   [("mirelle", "Years ago I healed a fever in a village near the coast. They survived. I never learned their names. I'd like to meet someone."), ("kael", "Granny Wren says she knows a grandmother from there."), ("mirelle", "Oh. All right. Yes.")],
   [("wren", "You healed my sister's boy. He grew up. He has children. They're named after the dawn."), ("mirelle", "I... I didn't know."), ("wren", "That is what you gave. Don't forget the part that's yours.")],
   R(xp=3000, gold=300, bond=[("mirelle", 1)]), act=4)
cq("CQ-Mirelle3", "mirelle", 3, "A Day Off", "Take Mirelle to the festival so she can live as herself for a day.",
   [obj("talk", "petra", 1, "Buy a flower with Mirelle", "aurelheim_market"), obj("talk", "bao", 1, "Try Bao's dumplings", "aurelheim_market"), obj("talk", "lio", 1, "Listen to Lio's song", "aurelheim_plaza")],
   ["quest.CQ-Mirelle2 == complete", "quest.MQ19 == complete"],
   [("lyra", "Mirelle has never taken a day off. We are going to fix that."), ("mirelle", "I... I'm not sure I know how."), ("kael", "That's what we're for.")],
   [("mirelle", "Three pastries. A flower I'll never let wilt. A song about a girl who learned to laugh. Is this what living feels like?"), ("lyra", "Mostly. Add some arguing."), ("mirelle", "Light will find a way. And so, it seems, will pastries.")],
   R(xp=5000, gold=100, items=[("food_festival_cake", 2)], bond=[("mirelle", 1)]), act=4)
cq("CQ-Mirelle4", "mirelle", 4, "Dawn Without Debt", "After the Tithe is broken, Mirelle learns pure holy magic.",
   [obj("explore", "sanctum_of_dawn", 1, "Return to the Sanctum of Dawn", "sanctum_of_dawn"), obj("flag", "pure_light_learned", 1, "Learn healing that costs nothing", "sanctum_of_dawn")],
   ["quest.CQ-Mirelle3 == complete", "quest.MQ20 == complete"],
   [("mirelle", "For the first time, I'm going to cast it knowing it won't cost me a year. Hold my hand. In case I faint from joy."), ("kael", "In case?"), ("mirelle", "When.")],
   [("mirelle", "It's warm. It's mine. It doesn't take anything. Radiant Renewal. That's what I'm calling it."), ("perpetua", "Sister Mirelle. You are teaching us all how to do this."), ("mirelle", "Gladly. Light will find a way. It just took us a long time to look.")],
   R(xp=10000, gold=1000, skills=[("mirelle", "radiant_renewal")], bond=[("mirelle", 1)]), act=4)
scene("perpetua", "scene.cq_mirelle4.pure", ["quest.CQ-Mirelle4 == active", "!flag.pure_light_learned"], [
    ("perpetua", "Sister Mirelle. Kneel. No: don't kneel. Stand. Teachers don't kneel to students and you stopped being a student today."),
    ("narrator", "(Light gathers around Mirelle, gentle and gold.)", {"do": ["flag.set:pure_light_learned"]}),
])

# ===================== DARIUS: The Second-Place Prince =====================
quest("CQ-Darius1", "companion", "Rivalry Duels", "Darius wants a rematch. Not for revenge. For accuracy.",
      [obj("kill", "shadow_soldier", 8, "Out-fight Darius against the shadow soldiers", "aurelheim_arena"), obj("talk", "darius", 1, "Compare scores with Darius", "aurelheim_arena")],
      giver="darius", turn_in="darius", region="aurelheim", act=2, prereq=["quest.MQ06 == complete"], rewards=R(xp=900, gold=150, bond=[("darius", 1)]),
      offer_lines=["I've been thinking about our duel. The ending, specifically. I'd like to correct it.", "Count the shadows. First to eight wins. I'll let you start. No. I won't. Start!"],
      done_lines=["Eight to seven. For you. By one stroke. I counted twice. Fine. Fine!", "I'm not bitter. I'm... improving. Don't look at me."], accept="Counting starts now.", decline="Later.")
quest("CQ-Darius2", "companion", "A Family Secret", "Darius finds Valecrest signatures in a ledger he was never meant to read.",
      [obj("talk", "verrin", 1, "Ask Lady Verrin about the Valecrest ledgers", "aurelheim"), obj("explore", "aurelheim_archive", 1, "Search the Archive for the ledger", "aurelheim_archive")],
      giver="darius", turn_in="darius", region="aurelheim", act=3, prereq=["quest.MQ12 == complete", "quest.CQ-Darius1 == complete"], rewards=R(xp=1900, gold=250, bond=[("darius", 1)]),
      offer_lines=["I found a name in my father's study. Caldrin. Next to a payment I can't explain. I'd like a witness when I look into it.", "Not a friend. A witness. There's a difference. Come along."],
      done_lines=["Valecrest money funded the Tithe. My father knew. My grandfather knew.", "I'm not going to cry. I'm going to... quietly stop speaking to a few people. For a decade. Thank you."], accept="I'll be your witness.", decline="Not yet.")
quest("CQ-Darius3", "companion", "Redemption Mission", "Darius asks to join the assault on Caldrin's cult and set the family name right.",
      [obj("explore", "aurelheim_cathedral", 1, "Ride with Darius to the cathedral", "aurelheim_cathedral"), obj("kill", "cult_zealot", 8, "Break the cultists guarding the nave", "aurelheim_cathedral")],
      giver="darius", turn_in="darius", region="aurelheim", act=4, prereq=["quest.MQ20 == complete", "quest.CQ-Darius2 == complete"], rewards=R(xp=7000, gold=800, bond=[("darius", 1)]), on_complete=["flag.set:darius_siege_summon"],
      offer_lines=["The Archbishop's cell is empty. Somebody let him leave. His men are still in the nave. I'm going. I'd like you beside me.", "Not behind. Beside. It's a new feeling. Don't wreck it."],
      done_lines=["It's done. The nave's quiet. The Valecrest name is, I think, a little less heavy.", "If you need a sword at the Gate, you'll have mine. Second place is still a place on the line."], accept="Beside you.", decline="Give me a moment.")
