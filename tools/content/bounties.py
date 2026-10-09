from qdsl import quest, obj, R

def BQ(id, title, summary, objs, rewards, prereq, region, act):
    quest(id, "bounty", title, summary, objs, giver="none", turn_in="auto", region=region, act=act, prereq=prereq, rewards=rewards, repeatable=True, level=act * 3)

B = ["flag.bounty_board_open"]
BQ("BQ-Goblins", "Bounty: Goblin Cull", "Guild board: thin the goblin raiders harassing the south road. Repeatable.", [obj("kill", "goblin_grunt", 6, "Defeat 6 goblin grunts", "goblin_warrens")], R(xp=300, gold=120, rep=("aurelheim", 1)), B, "vale", 2)
BQ("BQ-Wolves", "Bounty: Wolf Pack", "Guild board: wolves are menacing the Whisperwood roads. Repeatable.", [obj("kill", "wolf", 5, "Defeat 5 wolves", "whisperwood")], R(xp=400, gold=150, rep=("aurelheim", 1)), B + ["flag.c_rank"], "vale", 2)
BQ("BQ-Shadows", "Bounty: Shadow Sweep", "Guild board: shadow soldiers keep coming back. Repeatable.", [obj("kill", "shadow_soldier", 6, "Defeat 6 shadow soldiers", "aurelheim_arena")], R(xp=500, gold=180, rep=("aurelheim", 1)), B, "vale", 2)
BQ("BQ-Herbs", "Bounty: Herb Gathering", "Guild board: the apothecaries need meadow herbs. Repeatable.", [obj("collect", "meadow_herb", 6, "Gather 6 meadow herbs", "aurelheim_market")], R(xp=200, gold=90, rep=("aurelheim", 1)), B, "vale", 2)
BQ("BQ-Ore", "Bounty: Ore Haul", "Guild board: the smiths want iron ore. Repeatable.", [obj("collect", "iron_ore", 8, "Gather 8 iron ore", "aurelheim_market")], R(xp=250, gold=110, rep=("aurelheim", 1)), B, "vale", 2)
BQ("BQ-Scorpions", "Bounty: Scorpion Sweep", "Guild board: scorpions on the Caravan Road. Repeatable.", [obj("kill", "scorpion", 5, "Defeat 5 sand scorpions", "caravan_road")], R(xp=900, gold=260, rep=("qasr_amun", 1)), B + ["quest.MQ16 == complete"], "sunscar", 4)
BQ("BQ-FrostWolves", "Bounty: Frost Wolf Hunt", "Guild board: frost wolves threaten Hrimvald's patrols. Repeatable.", [obj("kill", "frost_wolf", 5, "Defeat 5 frost wolves", "whiteout_pass")], R(xp=1300, gold=340, rep=("hrimvald", 1)), B + ["quest.MQ18 == complete"], "frostspire", 4)
BQ("BQ-Imps", "Bounty: Imp Extermination", "Guild board: ember imps in the Ashen Mines. Repeatable.", [obj("kill", "ember_imp", 5, "Defeat 5 ember imps", "ashen_mines")], R(xp=2200, gold=480, rep=("emberforge", 1)), B + ["quest.MQ21 == complete"], "cinderhold", 4)
