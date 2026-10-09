from dsl import S, convo, scene
from qdsl import quest, obj, R, offer, done

MQ = lambda n: f"MQ{n:02d}"
def done_mq(n): return f"quest.{MQ(n)} == complete"

# ---------------- ACT 1: AWAKENING ----------------
quest("MQ01", "main", "The Day of the Crystal", "Every sixteen-year-old in Veyrath faces the Awakening Ceremony. Help Tilly with the morning chores, then climb the shrine steps.",
      [obj("flag", "tilly_chores", 1, "Help Tilly with the morning chores", "ashbrook"), obj("flag", "ceremony_done", 1, "Attend the Awakening Ceremony at the shrine", "ashbrook_shrine")],
      number=1, act=1, region="ashbrook", auto_start=True, rewards=R(xp=100, skills=[("kael", "heroic_slash")]), on_complete=["flag.set:s_rank_revealed"])
scene("tilly", "scene.mq01.tilly", ["quest.MQ01 == active", "!flag.tilly_chores"], [
    ("tilly", "There you are! You're late for chores and the ceremony's in an hour. Here, grab a bucket. Move, strike, dodge the goose. Standard stuff."),
    ("tilly", "Left stick to walk, camera with the mouse. Try not to run into the well. It's got feelings."),
    ("tilly", "Good. Now the goose. Hit it with the broom. Gently. It's an old goose."),
    ("kael", "It's biting my ankle!"),
    ("tilly", "That's normal. Dodge! Tap the roll button. Nice! See? You're a natural."),
    ("tilly", "Okay. Chores done. Go on, the shrine's waiting. I'll be right behind you. I'm just going to stand somewhere safe.", {"do": ["flag.set:tilly_chores"]}),
])
scene("haldor", "scene.mq01.haldor", ["quest.MQ01 == active", "flag.tilly_chores", "!flag.ceremony_done"], [
    ("haldor", "Ah, {player_name}. You're here. Good. Stand there. No, there. Hands behind your back until I say."),
    ("haldor", "Every child of Veyrath places a hand on the crystal on their sixteenth year. It answers with a rank. Most are D. A few are B. Nobody is S."),
    ("haldor", "Ready? Then place your hand."),
    ("narrator", "(The crystal begins to glow...)", {"do": ["cutscene:awakening", "flag.set:ceremony_done"]}),
])
quest("MQ02", "main", "Smoke on the Horizon", "A demon scout has followed the golden light to Ashbrook. Protect the village.",
      [obj("kill", "demon_imp", 6, "Defeat the imps attacking Ashbrook", "ashbrook_outskirts"), obj("explore", "ashbrook_outskirts", 1, "Reach the outskirts and face the demon scout", "ashbrook_outskirts")],
      number=2, act=1, region="ashbrook", prereq=[f"quest.MQ01 == complete"], auto_start=True, rewards=R(xp=140), on_start=["cutscene:skarn_arrives"], on_complete=["cutscene:village_burns", "flag.set:learned_dodge"])
quest("MQ03", "main", "Ashes and Oaths", "Orwyn presses his sword into your hands and tells you the truth about the Seals.",
      [obj("explore", "ashbrook_shrine", 1, "Find Elder Orwyn at the shrine", "ashbrook_shrine"), obj("flag", "orwyn_sacrifice", 1, "Escape with the survivors", "ashbrook")],
      number=3, act=1, region="ashbrook", prereq=[done_mq(2)], auto_start=True, rewards=R(xp=200, items=[("w_hero_t1", 1)], skills=[("kael", "resolve")]), on_complete=["flag.set:ashbrook_burned", "flag.set:fast_travel_hint"])
scene("orwyn", "scene.mq03.orwyn", ["quest.MQ03 == active", "!flag.orwyn_sacrifice"], [
    ("orwyn", "Kael. Here. Take Dawnbrand. It was mine, and my teacher's before that. Plain iron. It has never asked anyone to be a hero."),
    ("narrator", "(You received Dawnbrand.)", {"do": ["item.give:w_hero_t1:1"]}),
    ("orwyn", "Listen carefully. I won't have time to repeat it. Three hundred years ago a man named Aurel sealed the Demon King with a crown. The Crown broke into four Seals. They are failing."),
    ("orwyn", "The demons woke early because of you. Not your fault. A light that bright can be seen from the other side of the world."),
    ("narrator", "(Something crashes outside. The scout is coming.)", {"do": ["cutscene:orwyn_sacrifice", "flag.set:orwyn_sacrifice"]}),
])
quest("MQ04", "main", "The Road to Aurelheim", "Lead the refugees through bandit country to the guild capital.",
      [obj("kill", "bandit_thug", 5, "Drive off the road bandits", "whisperwood_edge"), obj("escort", "mira@aurelheim_gates", 1, "Escort Little Mira to Aurelheim", "aurelheim_gates"), obj("explore", "aurelheim_gates", 1, "Reach the gates of Aurelheim", "aurelheim_gates")],
      number=4, act=1, region="vale", prereq=[done_mq(3)], auto_start=True, rewards=R(xp=240, gold=60, bond=[]), on_complete=["cutscene:darius_mocks", "flag.set:fast_travel", "flag.set:aurelheim_open"])

# ---------------- ACT 2: GUILD RISE ----------------
quest("MQ05", "main", "Registered at Last", "Register at the Adventurer Guild and learn how the quest board, bounty system and shops work.",
      [obj("talk", "pip", 1, "Fill in the registration form with Pip", "guild_hall"), obj("talk", "garrick", 1, "Visit Garrick Thorne's forge", "aurelheim_plaza"), obj("talk", "selwyn", 1, "Visit Madame Selwyn's potion shop", "aurelheim_market")],
      giver="hanna", turn_in="hanna", number=5, act=2, region="aurelheim", prereq=[done_mq(4)], rewards=R(xp=300, gold=150, items=[("guild_card", 1), ("acc_guild_ring", 1)], rep=("aurelheim", 5)),
      on_complete=["flag.set:bounty_board_open", "flag.set:f_rank"],
      offer_lines=["Name: {player_name}. Origin: a village that doesn't exist anymore. Talent: gold. Ah. Yes. I read the festival reports.", "I'm registering you as F-rank. Publicly. The S-rank stays sealed in my desk until you've earned the weight of it. For your own safety.", "Go through the motions. Pip will stamp your card. Garrick will measure your hands. Selwyn will try to sell you something. Come back when you've met them all."],
      done_lines=["Card, ring, quest board, bounty list. You're a real adventurer now. Congratulations.", "One piece of advice. Strong heroes die alone. Remember that.", "I've seen too many gold-glowing prodigies in my time. None of them finished the story."],
      accept="Understood.", decline="Give me a moment.")
quest("MQ06", "main", "Trial of the Proud", "The Rank-Up Tournament: beat Darius Valecrest in a duel to reach C-rank.",
      [obj("boss", "darius_duel", 1, "Defeat Darius Valecrest in the arena", "aurelheim_arena")],
      giver="hanna", turn_in="auto", number=6, act=2, region="aurelheim", prereq=[done_mq(5)], rewards=R(xp=380, gold=200, skills=[("kael", "flame_parry")], rep=("aurelheim", 5)),
      on_start=["cutscene:duel_intro"], on_complete=["flag.set:c_rank", "flag.set:darius_rival"],
      offer_lines=["Rank-ups at Aurelheim are settled in the arena. You want C-rank? Beat a C-rank. I've arranged a sparring partner.", "Darius Valecrest. Noble family, A-rank prodigy, mouth like a rusty hinge. He asked for you by name."],
      accept="Bring him on.", decline="I need to prepare.")
quest("MQ07", "main", "The Goblin Warrens", "A goblin nest is strangling the trade routes. Clear it, and find out why they're so organized.",
      [obj("explore", "goblin_warrens", 1, "Enter the Goblin Warrens", "goblin_warrens"), obj("kill", "goblin_grunt", 8, "Defeat goblin grunts", "goblin_warrens"), obj("kill", "demon_marked_goblin", 1, "Find the demon-marked goblin", "goblin_warrens"), obj("boss", "grukk", 1, "Defeat Goblin Chieftain Grukk", "goblin_warrens")],
      giver="hanna", turn_in="hanna", number=7, act=2, region="vale", prereq=[done_mq(6)], rewards=R(xp=500, gold=300, items=[("hp_medium", 3)], rep=("aurelheim", 6)), on_complete=["flag.set:b_rank", "flag.set:seal_clue_1"],
      offer_lines=["Goblins have taken the south road. Caravans are paying tolls in blood. B-rank bounty, and it's yours if you want it.", "Take a party. I know you're alone. Take a party anyway."],
      done_lines=["A demon mark on a goblin. Not the first. Not the last. The report is going straight to my desk.", "B-rank. Well earned. Don't let it go to your head."],
      accept="I'll clear them out.", decline="Give me time.")
quest("MQ08", "main", "The Hollow Mark", "The demon mark leads back to Skarn. Learn what the Seals are, then hunt the scout down.",
      [obj("talk", "penn", 1, "Consult Archivist Penn about the Seals", "aurelheim_archive"), obj("explore", "ashbrook_outskirts", 1, "Follow Skarn's trail", "ashbrook_outskirts"), obj("boss", "skarn_1", 1, "Defeat Skarn (round one)", "ashbrook_outskirts")],
      giver="hanna", turn_in="auto", number=8, act=2, region="vale", prereq=[done_mq(7)], rewards=R(xp=700, gold=350, skills=[("kael", "sunder")]), on_complete=["flag.set:a_rank", "flag.set:skarn_escaped"],
      offer_lines=["Sit down. This is not for the board. Three hundred years ago the hero Aurel sealed the Demon King with a crown. It shattered into four Seals: Life, Time, Spirit, Flame.", "The Seals are weakening. The Demon King's heirs are waking. The mark you found on that goblin is theirs.", "There's a scout. Skarn. He burned your village. Penn has the old records. After that, find him."],
      accept="I'll find him.", decline="Tell me more first.")
quest("MQ09", "main", "Crown of Crisis", "A demon cult attacks Aurelheim during the Founders' Festival. Stop them and keep your power in check.",
      [obj("kill", "cult_zealot", 10, "Defeat the cultists in the plaza", "aurelheim_plaza"), obj("flag", "festival_survived", 1, "Regain control of the Ascendant power", "aurelheim_plaza")],
      giver="auto", turn_in="hanna", number=9, act=2, region="aurelheim", prereq=[done_mq(8)], auto_start=True, rewards=R(xp=900, gold=500, items=[("hp_large", 4)], rep=("aurelheim", 10)),
      on_start=["cutscene:festival_disaster"], on_complete=["awaken:1", "flag.set:s_rank", "flag.set:party_required", "flag.set:skill_tree_open"],
      done_steps=[("hanna", "Gather round. Everyone. By the authority of the Adventurer Guild, I hereby announce the first S-rank in three hundred years."),
                  ("hanna", "{player_name} of Ashbrook. Ascendant. You will retrieve the four Seal Fragments of the Crown. And you will do it with a party."),
                  ("kael", "A party? I don't need--"),
                  ("hanna", "That wasn't a request. Strong heroes die alone. Pick three friends. Start with the ones who aren't afraid of you.", {"do": ["quest.complete:MQ09"]})])
# the cultists' last words are the festival turn-in trigger: Hanna ends the scene
scene("hanna", "scene.mq09.calm", ["quest.MQ09 == active", "!flag.festival_survived"], [
    ("hanna", "Breathe. The light is going down. Good. Count to ten. You're all right."),
    ("hanna", "That wasn't your fault.", {"do": ["flag.set:festival_survived"]}),
])

# ---------------- ACT 3: FORMING THE PARTY ----------------
quest("MQ10", "main", "Into the Whisperwood", "The first Seal Fragment, Life, lies somewhere in the Whisperwood. The trees whisper your name.",
      [obj("explore", "whisperwood_edge", 1, "Enter the Whisperwood", "whisperwood_edge"), obj("kill", "wolf", 4, "Fend off the corrupted wolves", "whisperwood"), obj("talk", "fenn", 1, "Speak to Old Fenn", "fenns_hut")],
      giver="hanna", turn_in="fenn", number=10, act=3, region="vale", prereq=[done_mq(9)], rewards=R(xp=1000, gold=300, items=[("meadow_herb", 6)]), on_complete=["flag.set:forest_map"],
      offer_lines=["The Seal of Life is in Rootheart Hollow, in the heart of the Whisperwood. The forest is sick. Corruption is spreading from the Hollow.", "There's a hermit named Fenn. He talks to plants. He'll know the way. Be polite to the moss."],
      done_lines=["Hmm. The trees say your name differently than the others. They say it like it's a question.", "A knight's tracks lead to Greywall. Follow them. He'll be the one drinking."],
      accept="I'm on my way.", decline="Not today.")
quest("MQ11", "main", "The Knight Who Dropped His Shield", "A wounded knight's trail leads to the ruins of Fort Greywall. Help him face his old squad.",
      [obj("explore", "fort_greywall", 1, "Reach Fort Greywall", "fort_greywall"), obj("kill", "greywall_ghost", 8, "Lay the undead Greywall soldiers to rest", "fort_greywall"), obj("flag", "bram_joined", 1, "Convince Bram to take up his shield", "fort_greywall")],
      number=11, act=3, region="vale", prereq=[done_mq(10)], auto_start=True, rewards=R(xp=1100, gold=250, bond=[("bram", 1)]), on_complete=["cutscene:bram_shield", "flag.set:bram_joined"])
scene("bram", "scene.mq11.bram", ["quest.MQ11 == active", "!flag.bram_joined"], [
    ("bram", "Go away. I'm not for hire. I told you."),
    ("kael", "The ghosts at the gate were asking for you."),
    ("bram", "...What did they say?"),
    ("kael", "They asked if you got out."),
    ("bram", "I got out. That's the problem."),
    ("choice", [("Then come out again. With us.", {"then": [("bram", "...Fine. But I'm not doing it for a medal. Behind me, kid."), ("narrator", "(Sir Bram Ironvale joins the party!)", {"do": ["flag.set:bram_joined"]})]}),
                ("I'll wait.", {"then": [("bram", "Take your time. I'm not going anywhere.")]})]),
])
quest("MQ12", "main", "The Girl in the Forbidden Library", "A prodigy's mana is awakening a library guardian. Survive her storm and calm her.",
      [obj("explore", "forbidden_library", 1, "Enter the Forbidden Library", "forbidden_library"), obj("boss", "library_guardian", 1, "Survive the awakened guardian", "forbidden_library"), obj("flag", "lyra_joined", 1, "Reach Lyra Vesperine", "forbidden_library")],
      giver="voss", turn_in="auto", number=12, act=3, region="vale", prereq=[done_mq(11)], rewards=R(xp=1300, gold=300, bond=[("lyra", 1)]), on_complete=["cutscene:lyra_storm", "flag.set:lyra_joined"],
      offer_lines=["I would prefer you not know I asked. The Forbidden Library has woken up. Lyra Vesperine, a former student, hides there. Her mana is... unstable.", "The guardian responds to her fear. I cannot reach her. I have, shall we say, a history. You are strong enough to stand in it.", "Please. She is not a danger. She believes she is."],
      accept="I'll talk to her.", decline="Let me think.")
scene("lyra", "scene.mq12.lyra", ["quest.MQ12 == active", "!flag.lyra_joined"], [
    ("lyra", "Stay back! I can't stop it! The shelves, the lightning, it just keeps coming!"),
    ("kael", "I'm not afraid."),
    ("lyra", "You should be! I took out a wing of the Academy at fifteen!"),
    ("kael", "Then it's about time somebody stood in it."),
    ("choice", [("Come with us.", {"then": [("lyra", "...You're the one with more raw power than me, aren't you? Fine. Don't hold back or I'll be insulted."), ("narrator", "(Lyra Vesperine joins the party!)", {"do": ["flag.set:lyra_joined"]})]}),
                ("Take your time.", {"then": [("lyra", "I need a minute. Go away. Come back in an hour. I'll be less explosive.")]})]),
])
quest("MQ13", "main", "The Priestess's Secret", "Cult-corrupted monsters attack the Sanctum of Dawn. A priestess heals the wounded and pays a price she hides.",
      [obj("explore", "sanctum_of_dawn", 1, "Reach the Sanctum of Dawn", "sanctum_of_dawn"), obj("kill", "corrupted_beast", 6, "Defend the pilgrims", "sanctum_of_dawn"), obj("flag", "mirelle_joined", 1, "Convince Sister Mirelle to travel with you", "sanctum_of_dawn")],
      number=13, act=3, region="vale", prereq=[done_mq(12)], auto_start=True, rewards=R(xp=1500, gold=350, bond=[("mirelle", 1)]), on_complete=["cutscene:mirelle_fading", "flag.set:mirelle_joined"])
scene("mirelle", "scene.mq13.mirelle", ["quest.MQ13 == active", "!flag.mirelle_joined"], [
    ("mirelle", "Blessings of the dawn, traveler. The wounded come first, then questions. Please. Anyone who can hold a bandage."),
    ("kael", "Sister, your hair is turning white."),
    ("mirelle", "It's the light. It's nothing. Please, the next patient."),
    ("kael", "That wasn't light. That was you."),
    ("choice", [("Come with us. We'll find another way.", {"then": [("mirelle", "...Light will find a way. Yes. I'll come. I don't know why I'm saying yes."), ("narrator", "(Sister Mirelle Dawnsong joins the party!)", {"do": ["flag.set:mirelle_joined"]})]}),
                ("I'll help here first.", {"then": [("mirelle", "Thank you. Hold this one steady.")]})]),
])
quest("MQ14", "main", "Four Hearts, One Blade", "At the Convergence Shrine, the party faces mirrors of its fears and learns to share a single light.",
      [obj("explore", "convergence_shrine", 1, "Reach the Convergence Shrine", "convergence_shrine"), obj("boss", "shadow_self", 1, "Defeat the Lonely Self", "convergence_shrine")],
      number=14, act=3, region="vale", prereq=[done_mq(13)], auto_start=True, rewards=R(xp=1700, gold=400, bond=[("bram", 1), ("lyra", 1), ("mirelle", 1)]), on_complete=["cutscene:resonance_born", "flag.set:resonance_unlocked"])
quest("MQ15", "main", "The Thornqueen Falls", "Fight Vessara in Rootheart Hollow to claim the Seal of Life.",
      [obj("explore", "rootheart_hollow", 1, "Descend into Rootheart Hollow", "rootheart_hollow"), obj("kill", "thorn_minion", 6, "Cut through the thorn minions", "rootheart_hollow"), obj("boss", "vessara", 1, "Defeat Vessara, the Thornqueen", "rootheart_hollow")],
      number=15, act=3, region="vale", prereq=[done_mq(14)], auto_start=True, rewards=R(xp=2400, gold=600, items=[("seal_life", 1), ("hp_large", 4)]), on_complete=["cutscene:vessara_farewell", "awaken:2", "flag.set:seal_life_found"])

# ---------------- ACT 4: THE DEMON GENERALS ----------------
quest("MQ16", "main", "Into the Sunscar", "Cross the desert, win the oasis tribes' trust, and learn of the Hourglass Vault.",
      [obj("explore", "qasr_amun", 1, "Cross the desert to Qasr Amun", "qasr_amun"), obj("kill", "sand_serpent", 4, "Survive the Serpent Dunes", "serpent_dunes"), obj("talk", "edda", 1, "Meet Mayor Edda", "qasr_amun"), obj("talk", "kharim", 1, "Earn Kharim al-Sayyid's trust", "qasr_amun")],
      giver="auto", turn_in="kharim", number=16, act=4, region="sunscar", prereq=[done_mq(15)], auto_start=True, rewards=R(xp=2600, gold=500, items=[("food_desert_flatbread", 4)], rep=("qasr_amun", 10)), on_complete=["flag.set:mount_unlocked", "flag.set:desert_gear"],
      done_lines=["You have walked our sands and kept your temper. That is more than most kings. The hourglass is in the Vault. The tribes will guide you.", "And you may take a mount. Be nice to her. She bites people who are not."])
quest("MQ17", "main", "The Hourglass Tyrant", "Fight Khemet-Ur, who can rewind time to undo damage and wounds.",
      [obj("explore", "hourglass_vault", 1, "Enter the Hourglass Vault", "hourglass_vault"), obj("boss", "khemet_ur", 1, "Defeat Khemet-Ur, the Hourglass Tyrant", "hourglass_vault")],
      number=17, act=4, region="sunscar", prereq=[done_mq(16)], auto_start=True, rewards=R(xp=3600, gold=800, items=[("seal_time", 1), ("hp_grand", 3)], bond=[("mirelle", 1)]), on_complete=["flag.set:seal_time_found"])
quest("MQ18", "main", "The Frostspire Ascent", "Climb through the whiteout. An avalanche tests the party's trust.",
      [obj("explore", "hrimvald", 1, "Reach Hrimvald", "hrimvald"), obj("talk", "torvik", 1, "Speak with Skald Torvik", "hrimvald"), obj("explore", "whiteout_pass", 1, "Climb Whiteout Pass", "whiteout_pass"), obj("flag", "avalanche_survived", 1, "Survive the avalanche together", "whiteout_pass")],
      number=18, act=4, region="frostspire", prereq=[done_mq(17)], auto_start=True, rewards=R(xp=3800, gold=700, items=[("food_frost_broth", 4)], bond=[("lyra", 1), ("bram", 1)]), on_complete=["flag.set:cold_gear", "flag.set:mana_anchor_learned", "learn:lyra:mana_anchor"],
      on_start=[])
scene("torvik", "scene.mq18.torvik", ["quest.MQ18 == active", "!flag.avalanche_survived"], [
    ("torvik", "Whiteout Pass in this weather? You're braver than the guides. Or dumber. I haven't decided."),
    ("torvik", "Stay on the rope. Don't shout. Don't cast anything bigger than a candle. The mountain listens."),
    ("lyra", "Understood. I'll keep it small."),
    ("narrator", "(They begin the climb. A low rumble rolls down the slope...)", {"do": ["cutscene:avalanche", "flag.set:avalanche_survived"]}),
])
quest("MQ19", "main", "The Winter Choir", "Fight Isrevna in the Silent Choir Cathedral, where spells fall silent.",
      [obj("explore", "choir_cathedral", 1, "Enter the Silent Choir Cathedral", "choir_cathedral"), obj("kill", "choir_husk", 6, "Pass through the husks of the choir", "choir_cathedral"), obj("boss", "isrevna", 1, "Defeat Isrevna, the Winter Choir", "choir_cathedral")],
      number=19, act=4, region="frostspire", prereq=[done_mq(18)], auto_start=True, rewards=R(xp=5000, gold=1000, items=[("seal_spirit", 1), ("mp_grand", 3)], bond=[("lyra", 1), ("mirelle", 1)]), on_complete=["awaken:3", "flag.set:seal_spirit_found"])
quest("MQ20", "main", "Betrayal at Aurelheim", "Return to the capital and stop Archbishop Caldrin before he completes the ritual.",
      [obj("explore", "aurelheim_cathedral", 1, "Enter the Cathedral of the Dawn", "aurelheim_cathedral"), obj("boss", "caldrin", 1, "Defeat Archbishop Caldrin", "aurelheim_cathedral"), obj("flag", "mirelle_freed", 1, "Break the Vital Tithe with the Ascendant light", "aurelheim_cathedral")],
      number=20, act=4, region="aurelheim", prereq=[done_mq(19)], auto_start=True, rewards=R(xp=6500, gold=1500, bond=[("mirelle", 2), ("darius", 1)]), on_start=["cutscene:betrayal"], on_complete=["flag.set:tithe_broken", "flag.set:darius_guest", "flag.set:mirelle_freed", "flag.set:caldrin_arrested"])
quest("MQ21", "main", "Descent into Cinderhold", "The blacksmith guilds forge the party's final weapons while a volcano threatens the city.",
      [obj("explore", "emberforge", 1, "Reach Emberforge", "emberforge"), obj("talk", "brandt", 1, "Meet Forgemaster Brandt Cinder", "emberforge"), obj("kill", "magma_golem", 3, "Clear the Ashen Mines", "ashen_mines"), obj("collect", "ember_steel", 4, "Bring Brandt four Ember Steel Ingots", "ashen_mines")],
      giver="auto", turn_in="brandt", number=21, act=4, region="cinderhold", prereq=[done_mq(20)], auto_start=True,
      rewards=R(xp=8000, gold=1500, items=[("w_tank_oaths", 1), ("w_mage_starcaller", 1), ("w_heal_dawnsong", 1)]), on_complete=["flag.set:legendary_gear", "flag.set:blacksmith_mastery"],
      done_lines=["Four ingots of the best ember steel in the Keep, and not one scorched finger. I like you. Hold still.", "The Bulwark of Oaths for the shield-bearer, Starcaller for the witch, the Dawnsong for the priestess. Yours I'm saving for after the Warden.", "Don't die before I can finish it."])
quest("MQ22", "main", "The Molten Warden", "Fight Pyrrhax in the Molten Keep. He punishes anyone who does not guard.",
      [obj("explore", "molten_keep", 1, "Enter the Molten Keep", "molten_keep"), obj("kill", "ember_imp", 6, "Burn through the imp guard", "molten_keep"), obj("boss", "pyrrhax", 1, "Defeat Pyrrhax, the Molten Warden", "molten_keep")],
      number=22, act=4, region="cinderhold", prereq=[done_mq(21)], auto_start=True, rewards=R(xp=12000, gold=2500, items=[("seal_flame", 1), ("w_hero_zenith", 1), ("hp_elixir", 2)], bond=[("bram", 2)]),
      on_complete=["cutscene:bram_stand", "awaken:4", "flag.set:seal_flame_found", "flag.set:dawnbrand_zenith"])

# ---------------- ACT 5: THE DEMON LORD ----------------
quest("MQ23", "main", "The Truth of the Crown", "Reassemble the Sealing Crown and see what it remembers.",
      [obj("collect", "seal_life", 1, "Bring the Seal of Life", "aurelheim_archive"), obj("collect", "seal_time", 1, "Bring the Seal of Time", "aurelheim_archive"), obj("collect", "seal_spirit", 1, "Bring the Seal of Spirit", "aurelheim_archive"), obj("collect", "seal_flame", 1, "Bring the Seal of Flame", "aurelheim_archive"), obj("explore", "convergence_shrine", 1, "Let the Crown remember at the Convergence Shrine", "convergence_shrine")],
      number=23, act=5, region="vale", prereq=[done_mq(22)], auto_start=True, rewards=R(xp=16000, gold=3000, items=[("sealing_crown", 1)], bond=[("bram", 1), ("lyra", 1), ("mirelle", 1)]),
      on_complete=["cutscene:crown_truth", "awaken:5", "flag.set:truth_known", "flag.set:resonance_ultimate"])
quest("MQ24", "main", "Siege of the Hollow Gate", "The Guild, the Church, Darius and everyone you helped march on the Hollow Gate. Break through while the army holds the line.",
      [obj("explore", "front_camp", 1, "Join the army at the front camp", "front_camp"), obj("kill", "demon_legionary", 12, "Break the legion at the gate", "hollow_gate"), obj("boss", "skarn_final", 1, "Defeat Skarn, Herald of the Hollow King", "hollow_gate")],
      number=24, act=5, region="hollow", prereq=[done_mq(23)], auto_start=True, rewards=R(xp=20000, gold=4000, items=[("hp_elixir", 3), ("mp_elixir", 3)]), on_start=["cutscene:army_gathers"], on_complete=["flag.set:gate_broken", "flag.set:last_chance_to_prepare"])
quest("MQ25", "main", "The Hollow King", "Enter the Voidspire and face Vorathis. The Solo Ascent ends here.",
      [obj("explore", "voidspire_throne", 1, "Climb to the throne of Voidspire", "voidspire_throne"), obj("boss", "vorathis", 1, "Defeat Vorathis, the Hollow King", "voidspire_throne")],
      number=25, act=5, region="hollow", prereq=[done_mq(24)], auto_start=True, rewards=R(xp=60000, gold=10000, items=[("hp_elixir", 5)]),
      on_complete=["cutscene:aurel_farewell", "cutscene:ending_montage", "flag.set:game_cleared", "flag.set:newgameplus_unlocked"])
