from dsl import convo
from qdsl import quest, obj, R, offer, done

def SQ(id, title, summary, giver, region, objs, rewards, offer_lines, done_lines, prereq=None, type="side", turn_in=None, progress=None, act=2, accept="I'll take care of it.", decline="Not right now."):
    quest(id, type, title, summary, objs, giver=giver, turn_in=turn_in or giver, region=region, act=act, prereq=prereq or [], rewards=rewards,
          offer_lines=offer_lines, done_lines=done_lines, progress_lines=progress, accept=accept, decline=decline, level=act * 3)

G1 = ["quest.MQ05 == complete"]     # Aurelheim / Ashbrook
G2 = ["quest.MQ10 == complete"]     # Whisperwood
G3 = ["quest.MQ16 == complete"]     # Sunscar
G4 = ["quest.MQ18 == complete"]     # Frostspire
G5 = ["quest.MQ21 == complete"]     # Cinderhold
G6 = ["quest.MQ24 == complete"]     # Hollow Reach

# ============ Ashbrook / Aurelheim (10) ============
SQ("SQ-Lamb", "The Missing Lamb", "Farmer Eldric's youngest lamb has wandered off again. Find it before the wolves do.", "eldric", "ashbrook",
   [obj("explore", "lamb_meadow", 1, "Search Lamb Meadow", "lamb_meadow"), obj("kill", "wolf", 2, "Chase off the wolves circling the meadow", "lamb_meadow")],
   R(xp=200, gold=40, items=[("food_bread", 3)], rep=("ashbrook", 3)),
   ["The little one's gone again. Last time she was in the well. Before that, the chimney. Please? My knees can't do meadows."], ["She's fine! She's eaten half a pie. Don't ask. Take some bread."], prereq=["quest.MQ01 == complete"], act=1)
SQ("SQ-Ledger", "Pip's Lost Ledger", "Pip misplaced the guild's rank-up ledger, and Hanna doesn't know. Recover it quietly.", "pip", "aurelheim",
   [obj("kill", "bandit_thug", 4, "Track down the bandits who swiped the ledger", "whisperwood_edge"), obj("collect", "pip_ledger", 1, "Recover Pip's ledger", "whisperwood_edge")],
   R(xp=350, gold=120, items=[("hp_medium", 3)], rep=("aurelheim", 3)),
   ["Okay, so, I had a ledger. Big, leather, important. Contained every rank-up in the Guild. And I may have lost it. In the city. To bandits. Don't tell Hanna.", "They went toward the Whisperwood edge. If you could just... retrieve it. Quietly. Very quietly."],
   ["My ledger! Oh, my ledger! You're a saint. A short-tempered saint. Please, never mention this."], prereq=G1)
SQ("SQ-Rivals", "The Rival Adventurers", "Silverwing's captain bets you can't clear shadow soldiers faster than her team.", "kestrel", "aurelheim",
   [obj("kill", "shadow_soldier", 6, "Out-hunt Silverwing against the shadow soldiers", "aurelheim_arena")],
   R(xp=450, gold=200, items=[("acc_swift_anklet", 1)], rep=("aurelheim", 3)),
   ["Silverwing: fifty-two bounties, no failures. You: a ceremony and a fire. Let's see who clears six shadows first.", "Don't take it personally. I just want to be better. It's a sickness."],
   ["Six. Done. Hm. We took seven. Fine. You win this one. I'll buy you a drink. And then I'll start practicing."], prereq=G1)
SQ("SQ-RareOre", "Garrick's Rare Ore", "Garrick is out of iron ore and too proud to say so.", "garrick", "aurelheim",
   [obj("collect", "iron_ore", 10, "Bring Garrick 10 Iron Ore", "aurelheim_market")],
   R(xp=300, gold=100, items=[("acc_oath_knot", 1)], rep=("aurelheim", 2)),
   ["I'm not out of iron. I'm... between orders of iron. If an adventurer happened to have ten pieces on them, it'd be a favor."], ["Ten. Good grain. You've done this before. Take the Oath Knot. It was my father's. Don't tell Tobin."], prereq=G1)
SQ("SQ-MiraWish", "Little Mira's Wish", "Mira wants a bouquet of meadow herbs for the stars to smell.", "mira", "aurelheim",
   [obj("collect", "meadow_herb", 3, "Collect three bundles of meadow herb", "aurelheim_market"), obj("talk", "petra", 1, "Ask Petra to tie them into a bouquet", "aurelheim_market")],
   R(xp=250, gold=0, items=[("hp_small", 3)], bond=[], rep=("aurelheim", 4)),
   ["If I give the stars flowers, they'll look after you. Please? I made a list. It says: flowers. That's the whole list."], ["The stars are going to love them. And you. And me. Everyone is going to be so happy."], prereq=G1, accept="Let's make a bouquet.")
SQ("SQ-Bread", "Bess's Morning Rush", "Bess's ovens are rebuilt, but the flour sacks never arrived.", "bess", "ashbrook",
   [obj("collect", "wild_flour", 6, "Find six sacks of wild flour", "ashbrook")],
   R(xp=250, gold=60, items=[("food_bread", 6), ("food_stew", 2)], rep=("ashbrook", 3)),
   ["The merchants swear they sent flour. Somebody ate it. Probably the geese. Could you find six sacks? Anywhere. I'm not picky."], ["Six! Bless you, dear. The oven will sing. Take bread. Take all the bread."], prereq=["quest.MQ03 == complete"], act=1)
SQ("SQ-Soup", "Marta's Soup Stock", "The Gilded Lantern's famous soup needs river fish and a handful of herbs.", "marta", "aurelheim",
   [obj("collect", "river_fish", 4, "Fetch four river fish", "gilded_lantern"), obj("collect", "meadow_herb", 2, "Fetch two meadow herbs", "gilded_lantern")],
   R(xp=280, gold=80, items=[("food_stew", 4)], rep=("aurelheim", 2)),
   ["The soup is a legend. It's also running out. If you bring fish and herbs, I'll keep a bowl for you until the end of time."], ["Perfect. Smell that. That's the smell of eight generations of stubbornness."], prereq=G1)
SQ("SQ-Foal", "Wick's Runaway Foal", "A horse foal bolted from Wick's stables, and the bandits on the road aren't friendly.", "wick", "aurelheim",
   [obj("kill", "bandit_thug", 3, "Drive off the bandits near the road", "aurelheim_gates"), obj("explore", "lamb_meadow", 1, "Find the foal in the meadow", "lamb_meadow")],
   R(xp=320, gold=100, items=[("acc_swift_anklet", 1)], rep=("aurelheim", 3)),
   ["Little Star's gone. Bit through the rope and ran. If bandits get her... I'll never forgive myself. Please."], ["There she is. Covered in burrs and proud as a queen. You're a good egg. Take her anklet. It's lucky."], prereq=G1)
SQ("SQ-TobinSword", "Tobin's First Sword", "Garrick lets Tobin forge his first sword, if he can find the materials.", "tobin", "aurelheim",
   [obj("collect", "iron_ore", 6, "Collect 6 iron ore", "aurelheim_market"), obj("collect", "coal", 4, "Collect 4 coal", "aurelheim_market")],
   R(xp=300, gold=70, items=[("w_hero_dagger_t2", 1)], rep=("aurelheim", 2)),
   ["It's my first real sword! Master says I can do it if I bring my own ore and coal. I only have enough savings for... zero ore. Can you help?"], ["It's crooked. It's wonderful. I'm never going to stop looking at it. Here, take this dagger I made last week. It's... also crooked."], prereq=G1)
SQ("SQ-VollDrill", "Sergeant Voll's Drill", "Voll orders six shadow soldiers cleared to prove your footwork.", "voll", "aurelheim",
   [obj("kill", "shadow_soldier", 6, "Defeat six shadow soldiers without breaking stride", "aurelheim_arena")],
   R(xp=500, gold=100, skills=[("kael", "rising_slash")], rep=("aurelheim", 3)),
   ["Footwork is a disgrace. Six shadow soldiers. No retreating, no dithering. Elbows in, chin up. Go!"], ["Better. Not good. Better. Take the Rising Slash. It's footwork with a spin on it. Don't tell the others it's easy."], prereq=G1, type="side")

# ============ Whisperwood (10) ============
SQ("SQ-Remedy", "The Hermit's Remedy", "Old Fenn needs ingredients for a tonic against the creeping corruption.", "fenn", "whisperwood",
   [obj("collect", "moonpetal", 4, "Gather four moonpetals", "moonpetal_glade"), obj("collect", "meadow_herb", 2, "Gather two meadow herbs", "whisperwood")],
   R(xp=500, gold=100, items=[("holy_water", 3)], rep=("whisperwood", 4)),
   ["The moss tells me the sickness is spreading from the Hollow. I can brew a tonic. If I had moonpetal. Which I do not, because they don't like me anymore."], ["Mm. The moonpetal is pleased. So am I. This will hold the corruption at bay a few days."], prereq=G2, act=3)
SQ("SQ-Bark", "Whispers in the Bark", "Ranger Moss hears the trees whispering a name. Find out whose.", "moss", "whisperwood",
   [obj("explore", "whisperwood", 1, "Listen to the oldest tree", "whisperwood"), obj("kill", "rootling", 4, "Quiet the rootlings that interrupt", "whisperwood")],
   R(xp=550, gold=100, items=[("acc_moonpetal_locket", 1)], rep=("whisperwood", 4)),
   ["The oldest tree has been whispering a name for a week. I can almost hear it. If you stand there and don't talk, maybe we'll both hear it."], ["It says 'Vessara.' And then 'sorry.' Trees apologize a lot, it turns out."], prereq=G2, act=3)
SQ("SQ-MoonHarvest", "Moonpetal Harvest", "Ilse is behind on her moonpetal harvest and the moon is waxing.", "ilse", "aurelheim",
   [obj("collect", "moonpetal", 8, "Collect eight moonpetals", "moonpetal_glade")],
   R(xp=420, gold=120, items=[("elixir_vigor", 2)], rep=("whisperwood", 2)),
   ["Moonpetal only blooms three nights a month. This is one of them. If you pick eight, I'll make you a potion that smells like clean laundry."], ["Eight perfect petals. You don't pull. You ask. Where did you learn that?"], prereq=G2, act=3)
SQ("SQ-Wolf", "The Wolf Who Wasn't", "A strange wolf shadows travelers. Ravi thinks it's not what it seems.", "ravi", "whisperwood",
   [obj("kill", "corrupted_beast", 2, "Drive off the corrupted beasts", "whisperwood_edge"), obj("explore", "moonpetal_glade", 1, "Follow the strange wolf's tracks", "moonpetal_glade")],
   R(xp=600, gold=150, items=[("wolf_pelt", 4)], rep=("whisperwood", 4)),
   ["There's a wolf that doesn't hunt. It follows. It looks at people like it's trying to remember something. I want to know why."], ["It was a man once. A traveler. The corruption took the rest. I buried what I could. He seemed to stop... looking."], prereq=G2, act=3)
SQ("SQ-Monk", "The Monk's Trial", "Monk Seren will teach Focus Strike to anyone who can hold still while forest spirits test them.", "seren", "whisperwood",
   [obj("explore", "monks_clearing", 1, "Reach the Monk's Clearing", "monks_clearing"), obj("kill", "rootling", 6, "Pass the trial of rootlings", "monks_clearing")],
   R(xp=700, gold=100, skills=[("kael", "focus_strike")], rep=("whisperwood", 3)),
   ["I do not teach. I wait. If you stand in my clearing and let six rootlings strike at you, and do not strike back until you see the pause... I will teach you."], ["You saw the pause. Good. The strike lives inside it. Now go. Return when you can be quieter."], prereq=G2, type="side", act=3)
SQ("SQ-LostPages", "Lost Pages of the Whisperwood", "Penn needs five scattered pages of a mage's grimoire.", "penn", "aurelheim",
   [obj("collect", "lost_tome_page", 5, "Recover five lost grimoire pages", "whisperwood")],
   R(xp=800, gold=200, items=[("tome_meteor", 1)], rep=("aurelheim", 3)),
   ["A grimoire was scattered across the Whisperwood. Five pages. They glow faintly. If you gather them I'll let you borrow the tome. Gently. It bites."], ["Five pages. The ink is humming. This is... Meteor. I'm not allowed to give it to you. I'll look away. Take it."], prereq=G2, act=3)
SQ("SQ-RootScout", "Rootheart Reconnaissance", "Moss wants the thorn minions thinned before the Thornqueen's army grows.", "moss", "whisperwood",
   [obj("kill", "thorn_minion", 6, "Cut down six thorn minions", "rootheart_hollow")],
   R(xp=600, gold=150, items=[("dryad_bark", 4)], rep=("whisperwood", 3)),
   ["The thorns around the Hollow are moving. I don't like it when plants hold grudges. Six minions. Please."], ["Six down. They were more scared than I expected. That's a bad sign."], prereq=G2, act=3)
SQ("SQ-Glade", "The Quiet Glade", "A forgotten glade hums a dryad lullaby. Fenn wants someone to hear it.", "fenn", "whisperwood",
   [obj("explore", "moonpetal_glade", 1, "Sit in the Moonpetal Glade", "moonpetal_glade")],
   R(xp=300, gold=0, items=[("acc_thorn_crown", 1)], rep=("whisperwood", 5)), type="lore",
   offer_lines=["There's a glade where the moonpetals grow in a circle. Sit in the middle. Don't speak. If you hear a song, let it finish."], done_lines=["You heard it? The one about the first light? ...Of course you did. So did I. So did she. Hush."], prereq=G2, act=3)
SQ("SQ-FennTea", "Honey for the Hermit", "Fenn is out of tea and honey and is getting cranky.", "fenn", "whisperwood",
   [obj("collect", "moonpetal", 2, "Bring two moonpetals", "moonpetal_glade"), obj("collect", "food_bread", 2, "Bring two loaves of bread", "whisperwood")],
   R(xp=250, gold=50, items=[("elixir_focus", 2)], rep=("whisperwood", 2)),
   ["Tea. Bread. Moonpetal. I'm not fussy. I only ask that they exist."], ["Hm. Mm. This is acceptable. Tell the moss it was your idea."], prereq=G2, act=3)
SQ("SQ-RaviHunt", "Ravi's Hunt", "The wolf packs have multiplied. Ravi can't thin them alone.", "ravi", "whisperwood",
   [obj("kill", "wolf", 6, "Thin the wolf packs", "whisperwood")],
   R(xp=450, gold=130, items=[("wolf_pelt", 4)], rep=("whisperwood", 3)),
   ["Six wolves. Nothing fancy. I'll take the left ridge. You take the right. We'll meet at the creek."], ["Quiet work. Nobody died. That's a good hunt."], prereq=G2, act=3)

# ============ Sunscar Sands (10) ============
SQ("SQ-Thieves", "The Caravan Thieves", "Raiders have been ambushing Mayor Edda's caravans on the Caravan Road.", "edda", "qasr_amun",
   [obj("kill", "caravan_raider", 6, "Defeat the raiders on the Caravan Road", "caravan_road")],
   R(xp=1200, gold=300, items=[("hp_large", 3)], rep=("qasr_amun", 5)),
   ["Trade is the oasis's blood. Someone is cutting the vein. I need the raiders stopped. Quietly, preferably. Loudly, if necessary."], ["The council thanks you. In writing. I'll have it drafted."], prereq=G3, act=4)
SQ("SQ-Serpent", "Sand Serpent Hunt", "Sand serpents have driven off the scouts of Qasr Amun.", "idris", "qasr_amun",
   [obj("kill", "sand_serpent", 4, "Slay four sand serpents", "serpent_dunes")],
   R(xp=1100, gold=250, items=[("scorpion_chitin", 3)], rep=("qasr_amun", 4)),
   ["Serpents don't hunt this far north. Something pushed them. I'd like four of them gone before something else does."], ["The dunes are calm. You move like someone who's listened to sand. Not a compliment I give lightly."], prereq=G3, act=4)
SQ("SQ-Mural", "The Lost Mural", "Anwen's team found a mural that shows a single golden figure surrounded by shadows. Find the rest of the wall.", "anwen", "khemet_ruins",
   [obj("explore", "lost_mural", 1, "Find the Lost Mural", "lost_mural")],
   R(xp=900, gold=200, items=[("acc_sandglass_charm", 1)], rep=("qasr_amun", 4)), type="lore",
   offer_lines=["The mural shows a lone hero, golden, facing a crowd of shadows. I need the rest. I think there's an inscription."], done_lines=["There's a name beneath the figure. It's been scratched out. Four letters. The first is 'A'."], prereq=G3, act=4)
SQ("SQ-Spice", "Zaira's Rare Spice", "Zaira wants desert bloom for a recipe hidden in the mural.", "zaira", "qasr_amun",
   [obj("collect", "desert_bloom", 4, "Gather four desert blooms", "caravan_road"), obj("explore", "lost_mural", 1, "Copy the recipe from the mural", "lost_mural")],
   R(xp=1000, gold=250, items=[("spice_blend", 3), ("food_desert_flatbread", 3)], rep=("qasr_amun", 4)),
   ["My grandmother's spice blend. The recipe is in a mural in Khemet. Desert bloom is the base. If you find both, I'll cook you something illegal."], ["Smell that. That's four generations of arguments in one pot. Eat."], prereq=G3, act=4)
SQ("SQ-Well", "The Poisoned Well", "Scorpions are fouling a well outside Qasr Amun.", "kharim", "qasr_amun",
   [obj("kill", "scorpion", 5, "Clear scorpions from the well", "caravan_road")],
   R(xp=900, gold=200, items=[("antidote", 4)], rep=("qasr_amun", 4)),
   ["Water is life. Scorpions don't care. Five of them have made the western well their home. Evict them politely."], ["Politely, you say? Hm. The well says thank you."], prereq=G3, act=4)
SQ("SQ-Weave", "Fatima's Chitin", "Fatima weaves armor from scorpion chitin, if you can supply it.", "fatima", "qasr_amun",
   [obj("collect", "scorpion_chitin", 4, "Collect four scorpion chitin plates", "caravan_road")],
   R(xp=850, gold=200, items=[("a_hero_body_t3", 1)], rep=("qasr_amun", 3)),
   ["Light, strong, ugly. That's chitin armor. I'd weave it for you if I had the plates. You can find them on the desert scorpions."], ["There. A coat that fits like a promise. And a little bit of a stinger, for style."], prereq=G3, act=4)
SQ("SQ-MountStray", "Jalil's Stray", "One of Jalil's sand-striders has wandered into the dunes.", "jalil", "qasr_amun",
   [obj("explore", "serpent_dunes", 1, "Search the Serpent Dunes", "serpent_dunes"), obj("kill", "sand_serpent", 2, "Rescue her from two serpents", "serpent_dunes")],
   R(xp=800, gold=180, items=[("acc_swift_anklet", 1)], rep=("qasr_amun", 3)),
   ["My best mare. She bites. She's also family. If you find her, don't try to ride her. Offer her an apple."], ["She likes you. That's worrying. Don't let it go to your head."], prereq=G3, act=4)
SQ("SQ-Dig", "Nashu's Dig", "Nashu needs time sand from the tomb wraiths to date a chamber.", "nashu", "khemet_ruins",
   [obj("collect", "time_sand", 2, "Collect two vials of time sand", "khemet_ruins")],
   R(xp=950, gold=200, items=[("acc_sage_pendant", 1)], rep=("qasr_amun", 3)),
   ["The chamber is four thousand years old. Or four hundred. Or thirty minutes. Time sand tells. Bring me some."], ["Fascinating! It's sixty-three years old. I'm shocked. I'm going to write this up."], prereq=G3, act=4)
SQ("SQ-Flatbread", "Sahar's Best Lemonade", "Sahar needs flour and spice for her famous flatbread.", "sahar", "qasr_amun",
   [obj("collect", "wild_flour", 4, "Collect four sacks of wild flour", "qasr_amun"), obj("collect", "spice_blend", 1, "Collect one spice blend", "qasr_amun")],
   R(xp=700, gold=150, items=[("food_desert_flatbread", 4)], rep=("qasr_amun", 3)),
   ["Flatbread: the single dish that makes tourists weep. Flour, spice, patience. I'm out of two of the three."], ["There. That's the smell of home. Eat before the camels notice."], prereq=G3, act=4)
SQ("SQ-ScoutWarning", "The Scout's Warning", "Idris saw armored footprints near the catacombs and wants the wraiths cleared.", "idris", "qasr_amun",
   [obj("kill", "tomb_wraith", 4, "Banish the tomb wraiths", "scorpion_catacombs")],
   R(xp=1100, gold=250, items=[("time_sand", 2)], rep=("qasr_amun", 4)),
   ["Something is waking the dead. I don't know what. I know four wraiths came out of the catacombs last night and one of them waved at me."], ["Thanks. I've never been waved at by a ghost before. I'd like it never to happen again."], prereq=G3, act=4)

# ============ Frostspire Peaks (10) ============
SQ("SQ-Climbers", "The Stranded Climbers", "A team is stuck at the Climbers' Camp without supplies.", "torvik", "hrimvald",
   [obj("collect", "hp_medium", 4, "Gather four Healing Draughts", "hrimvald"), obj("explore", "climbers_camp", 1, "Reach the Climbers' Camp", "climbers_camp")],
   R(xp=1500, gold=300, items=[("food_frost_broth", 3)], rep=("hrimvald", 5)),
   ["Four of them, stuck above the pass. No food. One broken leg. Bring potions. I'd go myself, but my knees are older than the mountain."], ["All four alive. Frostbitten, ungrateful, and alive. I'll take it."], prereq=G4, act=4)
SQ("SQ-SongsSnow", "Songs in the Snow", "Elowen hears Isrevna's songs in the Singing Ice and wants to know what she's saying.", "elowen", "hrimvald",
   [obj("explore", "singing_ice", 1, "Listen at the Singing Ice", "singing_ice")],
   R(xp=1200, gold=200, items=[("acc_frost_circlet_pin", 1)], rep=("hrimvald", 5)), type="lore",
   offer_lines=["There's a place where the ice resonates. Her songs echo there. I want to know if they're lullabies or pleas."], done_lines=["She sings 'the first light.' Over and over. As if she's waiting for someone to answer."], prereq=G4, act=4)
SQ("SQ-Lighthouse", "The Frozen Lighthouse", "Sigrid wants to explore the old lighthouse. It's haunted by ice wraiths.", "sigrid", "hrimvald",
   [obj("explore", "frozen_lighthouse", 1, "Reach the Frozen Lighthouse", "frozen_lighthouse"), obj("kill", "ice_wraith", 4, "Banish the ice wraiths", "frozen_lighthouse")],
   R(xp=1600, gold=300, items=[("frostcap", 4)], rep=("hrimvald", 5)),
   ["The lighthouse hasn't shone for a hundred years. I want to see the lamp. Torvik says no. So obviously we're going."], ["The lamp is still there. Frozen. Beautiful. I took a pebble. For the memory."], prereq=G4, act=4)
SQ("SQ-SkaldLast", "Skald's Last Climb", "Torvik asks you to bring something to the Lighthouse in his daughter's memory.", "torvik", "hrimvald",
   [obj("explore", "frozen_lighthouse", 1, "Leave Torvik's token at the Lighthouse", "frozen_lighthouse")],
   R(xp=1500, gold=0, items=[("acc_rime_ring", 1)], bond=[], rep=("hrimvald", 8)), type="lore", prereq=G4 + ["quest.SQ-Lighthouse == complete"],
   offer_lines=["My daughter's scarf. I've kept it for ten years. I'd like it left where she fell. I can't climb that high. Will you?"], done_lines=["Did the wind take it? Good. She always wanted to fly."], act=4)
SQ("SQ-Bell", "Halvar's Bell", "Halvar needs frost silver to recast the cathedral bell.", "halvar", "hrimvald",
   [obj("collect", "frost_silver", 3, "Collect three Frost Silver Ore", "hrimvald")],
   R(xp=1300, gold=300, rep=("hrimvald", 4)),
   ["The cathedral bell cracked. It has to be recast, and the only metal that sings right is frost silver. Three pieces. Please."], ["Listen. That's the first clean note I've heard in years. Thank you."], prereq=G4, act=4)
SQ("SQ-FrostWolves", "Marek's Wolves", "Frost wolves threaten the Hrimvald patrols.", "marek", "hrimvald",
   [obj("kill", "frost_wolf", 6, "Defeat six frost wolves", "whiteout_pass")],
   R(xp=1500, gold=350, items=[("yeti_fur", 3)], rep=("hrimvald", 4)),
   ["Six wolves on the north ridge. My patrols are spread thin. I'd like them thinned before the next snowfall."], ["All present. Count's good. That's the best news I've had this month."], prereq=G4, act=4)
SQ("SQ-Broth", "Gudrun's Stew Stock", "Gudrun needs frostcap and fish for her winter broth.", "gudrun", "hrimvald",
   [obj("collect", "frostcap", 4, "Gather four frostcaps", "hrimvald"), obj("collect", "river_fish", 2, "Fetch two river fish", "hrimvald")],
   R(xp=1000, gold=200, items=[("food_frost_broth", 4)], rep=("hrimvald", 3)),
   ["Winter's coming. The broth is the only thing that keeps the miners from mutiny. Bring me frostcap and fish."], ["That's the one. Now sit. Eat. You look thin."], prereq=G4, act=4)
SQ("SQ-Patrol", "The Silent Patrol", "Captain Marek's patrol vanished in Whiteout Pass.", "marek", "hrimvald",
   [obj("explore", "whiteout_pass", 1, "Search Whiteout Pass for the patrol", "whiteout_pass"), obj("kill", "ice_wraith", 3, "Free the patrol from the wraiths", "whiteout_pass")],
   R(xp=1500, gold=300, items=[("acc_warding_charm", 1)], rep=("hrimvald", 4)),
   ["Six of mine went into the pass four days ago. I'd like them back. Even if it's just their hats."], ["All six. Cold. Cranky. Alive. I'm buying the first round."], prereq=G4, act=4)
SQ("SQ-Fur", "Sigrid's Warm Coat", "Sigrid needs yeti fur to stitch a coat for a client.", "sigrid", "hrimvald",
   [obj("collect", "yeti_fur", 3, "Gather three pieces of yeti fur", "hrimvald")],
   R(xp=1200, gold=250, items=[("a_hero_body_t4", 1)], rep=("hrimvald", 3)),
   ["My client's freezing. I can fix that with three yeti furs. Frost wolves drop them if you're nice about it."], ["Beautiful. Warm as a hug. You'll never be cold again, at least for a week."], prereq=G4, act=4)
SQ("SQ-ChoirHusks", "The Choir's Husks", "Elowen wants the husks of the old choir put to rest.", "elowen", "hrimvald",
   [obj("kill", "choir_husk", 5, "Lay five choir husks to rest", "choir_cathedral")],
   R(xp=1500, gold=300, items=[("spirit_shard", 2)], rep=("hrimvald", 4)),
   ["They sing sometimes. Not words. A hollow note. I think they're the singers who stayed too long in the silence."], ["They didn't resist. Thank you. I think they were waiting for a kind hand."], prereq=G4, act=4)

# ============ Cinderhold (10) ============
SQ("SQ-Strike", "Ashen Miners Strike", "The miners' guild has downed tools. Ysolde needs coal moved to keep the city warm.", "ysolde", "emberforge",
   [obj("talk", "magnus", 1, "Talk to Stoker Magnus about the furnaces", "emberforge"), obj("collect", "coal", 10, "Haul ten loads of coal", "ashen_mines")],
   R(xp=2500, gold=400, items=[("acc_ash_scarf", 1)], rep=("emberforge", 6)),
   ["The strike's a fair one. The city's cold, and my miners are the only ones who understand why. Move coal for them. Then help me argue with the foremen."], ["The strike's settled. Better pay, better lamps. Not perfect. We'll keep shouting."], prereq=G5, act=4)
SQ("SQ-Anvil", "The Singing Anvil", "Brandt says the old anvil in the forgotten forge sings at night.", "brandt", "emberforge",
   [obj("explore", "singing_forge", 1, "Listen to the Singing Anvil", "singing_forge"), obj("collect", "ember_steel", 2, "Bring two ember steel ingots to strike on it", "singing_forge")],
   R(xp=2800, gold=400, items=[("polished_gem", 2)], rep=("emberforge", 6)),
   ["There's an anvil in the old forge that rings with no hammer. Forge-songs say it was Aurel's. A knight's. A tired knight's."], ["It rang when you struck it. Twelve notes. The last one... trails off. Like a man who ran out of breath."], prereq=G5, act=4)
SQ("SQ-Apprentice", "Forgemaster's Apprentice", "Cinda's first masterwork needs ember steel and iron.", "cinda", "emberforge",
   [obj("collect", "ember_steel", 1, "Bring Cinda an ember steel ingot", "emberforge"), obj("collect", "iron_ore", 4, "Bring four iron ore", "emberforge")],
   R(xp=2000, gold=300, items=[("acc_forge_signet", 1)], rep=("emberforge", 4)),
   ["It's my first real piece. Master Brandt says it has to be perfect. I think it has to be merely good. Can you bring supplies?"], ["It's wonky. It's mine. Brandt didn't laugh. I think that counts."], prereq=G5, act=4)
SQ("SQ-LavaFish", "Lava-Fish Special", "Vesna's signature dish needs three raw lava-fish.", "vesna", "emberforge",
   [obj("collect", "lava_fish_raw", 3, "Catch three raw lava-fish", "lava_channels")],
   R(xp=2200, gold=300, items=[("food_lava_fish", 4)], rep=("emberforge", 4)),
   ["I catch them myself usually. My back is out. Three. They're in the channels. They bite back. It's good practice."], ["Wait. Try this. ...Breathe. Breathe. Drink water. I'm so sorry. It's delicious."], prereq=G5, act=4)
SQ("SQ-ForgeSong", "The Forge-Song", "Doro hums the last verse of the forge-song and can't remember how it ends.", "doro", "emberforge",
   [obj("explore", "ashen_mines", 1, "Descend to where the song is loudest", "ashen_mines")],
   R(xp=2000, gold=0, items=[("acc_cinder_amulet", 1)], rep=("emberforge", 5)), type="lore",
   offer_lines=["There's a song in the mines. I know the verses but not the end. If you go down to the lowest gallery and listen, tell me how it ends."], done_lines=["'And the knight set down his sword, and no one came.' ...That's it? That's the end? Oh. Oh, that's terrible."], prereq=G5, act=4)
SQ("SQ-Golems", "Magnus's Fire", "Magnus's furnaces are getting raided by magma golems.", "magnus", "emberforge",
   [obj("kill", "magma_golem", 2, "Smash two magma golems", "ashen_mines")],
   R(xp=2500, gold=500, items=[("magma_scale", 3)], rep=("emberforge", 5)),
   ["The golems love my coal. I can't blame them. But it's mine."], ["Two down. Thank you. Now they know there's an angry man behind the shovel."], prereq=G5, act=4)
SQ("SQ-Imps", "Ysolde's Shaft", "Ember imps have taken the upper shaft. Ysolde wants them gone.", "ysolde", "emberforge",
   [obj("kill", "ember_imp", 6, "Clear six ember imps from the upper shaft", "ashen_mines")],
   R(xp=2500, gold=450, items=[("hp_grand", 2)], rep=("emberforge", 5)),
   ["Little fire gremlins. They're burning the beams. Six of them. And try not to topple the supports."], ["Quiet at last. Supports intact. I'd kiss you if I didn't have a reputation."], prereq=G5, act=4)
SQ("SQ-Scales", "Brandt's Armor Order", "Brandt wants magma scales for a commission.", "brandt", "emberforge",
   [obj("collect", "magma_scale", 3, "Collect three magma scales", "ashen_mines")],
   R(xp=2300, gold=400, items=[("a_tank_body_t5", 1)], rep=("emberforge", 4)),
   ["Magma scale plate takes a special temper. I need three. The golems have them. They don't give them up willingly."], ["Look at that grain. Like a sunset trapped in iron. Take this cuirass off my shelf. It's done."], prereq=G5, act=4)
SQ("SQ-CoalRun", "The Coal Run", "Magnus needs a coal delivery run before the furnaces fail.", "magnus", "emberforge",
   [obj("collect", "coal", 15, "Collect fifteen loads of coal", "ashen_mines")],
   R(xp=2000, gold=350, items=[("hp_large", 4)], rep=("emberforge", 4)),
   ["Furnaces are low. Fifteen loads. If we run dry, the whole city goes dark."], ["The furnaces roar. I may cry."], prereq=G5, act=4)
SQ("SQ-Petition", "The Miners' Petition", "Doro asks you to carry the miners' petition to Ysolde.", "doro", "emberforge",
   [obj("talk", "ysolde", 1, "Hand the petition to Ysolde", "emberforge")],
   R(xp=1500, gold=200, items=[("acc_ash_scarf", 1)], rep=("emberforge", 4)),
   ["It's three pages, signed by every miner on my shift. Ysolde needs to read it. I don't trust the foremen to deliver it."], ["Ysolde read it. Twice. She's going to the council. I'd cry but I'm saving it for the day it passes."], prereq=G5, act=4)

# ============ Hollow Reach (3) ============
SQ("SQ-Defector", "Cult Defector Nox", "Nox wants the cultists' remains cleaned out of his old hideout.", "nox", "front_camp",
   [obj("collect", "shadow_residue", 5, "Gather five vials of shadow residue", "nox_hideout"), obj("explore", "nox_hideout", 1, "Reach Nox's hideout", "nox_hideout")],
   R(xp=6000, gold=800, items=[("acc_voidstone", 1)], rep=("front_camp", 5)),
   ["My old brothers left things. Vials, ledgers, a favorite robe. I want it gone. Bring me the residue so I can burn it properly."], ["There. Ashes. For the first time, I can breathe in here. Take this shard. It was mine. It's yours."], prereq=G6, act=5)
SQ("SQ-LastLetters", "Last Letters from the Front", "Medic Reyna asks you to collect and deliver soldiers' last letters.", "reyna", "front_camp",
   [obj("talk", "vale_d", 1, "Collect Deserter Vale's letter", "front_camp"), obj("talk", "reyna", 1, "Deliver the letters to Reyna", "front_camp")],
   R(xp=5000, gold=500, items=[("hp_elixir", 1)], rep=("front_camp", 5)),
   ["The soldiers write letters before every battle. I carry them. If one of them doesn't come back, I mail it. It's the worst job I do. Help me with the pile."], ["There. Sorted. Sealed. I'll mail them tonight. And I'll hope I never have to."], prereq=G6, act=5)
SQ("SQ-Promise", "The Soldier's Promise", "Vale swore to protect the gate that he once ran from.", "vale_d", "front_camp",
   [obj("kill", "demon_legionary", 4, "Hold the gate beside Vale", "hollow_gate")],
   R(xp=5500, gold=600, items=[("acc_hollow_band", 1)], rep=("front_camp", 5)),
   ["I ran once. I won't again. If you stand with me, I'll hold. I need someone to see it."], ["I held. Did you see? I held. I think my daughter will hear about this one."], prereq=G6, act=5)
