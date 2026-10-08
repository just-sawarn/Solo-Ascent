# SOLO ASCENT: The S-Rank Hero
## Complete Story Bible (`/docs/STORY.md`)

> **For Claude Code:** This is the canonical narrative source. All quest, dialogue, NPC, boss and cutscene content must follow it. Convert it into the JSON data files in `/data` (quests, dialogue, npcs, enemies, cutscenes). Quest IDs here (`MQ##`, `CQ-<name>#`, `SQ-*`) are stable and must be used as the data IDs. Write all in-game dialogue yourself in the tone described in section 12.

---

## 1. Premise and Theme

**Logline:** A boy born with an S-rank talent believes strength means standing alone. He must learn that the world's greatest power is not in one hero but in the people who stand beside him, and use it to end a cycle of despair that has destroyed every hero before him.

**Theme:** *Power shared is power multiplied.* The game title is deliberately ironic. The hero starts out wanting a "solo ascent," and the story proves that wrong.

**Tone:** Classic shonen / isekai-free fantasy anime. Warm and humorous early on, emotionally heavy in the middle, triumphant at the end. Big power-up moments, rivals who become allies, and sacrifices that matter.

**Core twist (revealed in MQ23):** The Demon Lord Vorathis is **Aurel, the first S-rank hero**. He sealed the previous Demon King alone, absorbed all of its despair, and slowly became the thing he fought. The S-rank "Ascendant" talent is not a gift. It is the world's way of dumping its despair onto one chosen person. Kael breaks the cycle because he is not alone.

---

## 2. World: The Continent of Veyrath

Veyrath is a continent of five regions, held together by the **Adventurer Guild** and the **Church of the Dawn**. Magic comes from **Aether**, taught at the **Spire Academy**. Three hundred years ago, the hero Aurel sealed the Demon King using the **Sealing Crown**. The Crown shattered into four **Seal Fragments** (Life, Time, Spirit, Flame) that were scattered and guarded. Seals are now weakening, and the Demon King's heirs have begun to wake.

### Regions

| # | Region | Hub / Key locations | Mood | Dungeons (2 each) |
|---|--------|---------------------|------|-------------------|
| 1 | **Ashbrook Vale and the Whisperwood** | Ashbrook Village, **Aurelheim** (guild capital hub), Whisperwood Forest | Pastoral, then eerie green forest | Goblin Warrens; Rootheart Hollow |
| 2 | **Sunscar Sands** | Oasis town **Qasr Amun**, Sunken Ruins of Khemet | Golden, ancient, mysterious | Scorpion Catacombs; The Hourglass Vault |
| 3 | **Frostspire Peaks** | Mountain town **Hrimvald**, the Choir Cathedral | Blue, silent, majestic | Whiteout Pass; The Silent Choir Cathedral |
| 4 | **Cinderhold** | Smith-city **Emberforge**, the Molten Keep | Red, industrial, dangerous | Ashen Mines; The Molten Keep |
| 5 | **The Hollow Reach** | Demon Castle **Voidspire**, Hollow Gate | Purple-black, twisted sky | Bone Bridge Approach; Voidspire Castle |

---

## 3. Main Cast

### 3.1 Protagonist: **Kael** (default name; player may rename. Optional female variant **Kaela**, same story)
- **Age:** 16. **Origin:** Ashbrook Village, raised by his grandfather, Elder Orwyn.
- **Talent:** **Ascendant** (S-rank). It shows as a faint golden glow, and it grows when he overcomes hardship *and* when he protects others.
- **Personality:** Earnest, stubborn, secretly lonely. Hates being treated as a weapon. Early flaw: believes asking for help is weakness, because the village feared his power.
- **Arc:** From "I'll handle it myself" to leading a party that trusts him.
- **Signature:** A plain iron sword, **Dawnbrand**, which evolves through the story.

### 3.2 The Party

**Sir Bram Ironvale, the Tank**
- **Age:** 38. Former Captain of the Royal Shield Guard.
- **Backstory:** At **Fort Greywall**, Bram ordered a retreat that saved the kingdom's army but left his own squad behind. He has been drinking and drifting since, and he refuses to lift his shield for anyone.
- **Personality:** Gruff, dry humor, fatherly. Calls Kael "kid."
- **Gameplay role:** Taunt, damage reduction, party barriers, counter-block.
- **Arc:** Forgives himself and finally holds the line for people who choose to stay.
- **Weapon / Skill motif:** Shield **Bulwark of Oaths**; skills named after vows.

**Lyra Vesperine, the Mage**
- **Age:** 16. Prodigy of the Spire Academy.
- **Backstory:** Her mana is far beyond her control. She accidentally destroyed the Academy's east wing and was exiled. She now hides in the Forbidden Library, convinced she is a danger to everyone.
- **Personality:** Sharp-tongued, competitive, secretly kind. She treats Kael as a rival because he is the only person with more raw power than her, and she is thrilled by it.
- **Gameplay role:** AoE elemental burst, crowd control, shield-breaking. Limited mana.
- **Arc:** Learns control through trust, and learns that power is safe when someone is there to catch you.

**Sister Mirelle Dawnsong, the Healer**
- **Age:** 22. Priestess of the Church of the Dawn.
- **Backstory:** Her healing is powered by her own lifespan. The Church calls it a *Vital Tithe* and keeps it secret. She believes this is her duty and smiles through it.
- **Personality:** Gentle, cheerful, quietly stubborn. The party's emotional center.
- **Gameplay role:** Heals, cleanse, revive, buffs. Her strongest spells cost her a visible "lifespan" meter in cutscenes (not in gameplay).
- **Arc:** Learns she deserves to live for herself. The party finds a way to free her from the Tithe.

### 3.3 Rival and Mentors
- **Darius Valecrest:** 17, A-rank noble prodigy and Kael's rival. Proud and sarcastic, he trained his whole life and was beaten by a farm boy. Becomes a reluctant ally and fights beside the party in the finale.
- **Elder Orwyn:** Kael's grandfather and a retired sage. Gives Kael Dawnbrand, then sacrifices himself in MQ03 to hold the demon scout back. Appears later as a spirit guide in dream sequences.
- **Guildmaster Hanna "Iron Hanna" Ostrovsk:** 55, no-nonsense head of the Adventurer Guild. Warns Kael that S-rank heroes tend to die alone.
- **Archbishop Caldrin:** Public face of the Church. Secretly feeds the Vital Tithe to the Demon Lord (reveal in MQ20).

### 3.4 Antagonists

**The Four Demon Generals** (full details in section 6):
1. **Vessara, the Thornqueen** (Whisperwood)
2. **Khemet-Ur, the Hourglass Tyrant** (Sunscar Sands)
3. **Isrevna, the Winter Choir** (Frostspire Peaks)
4. **Pyrrhax, the Molten Warden** (Cinderhold)

**Skarn, the Demon Scout:** Early recurring villain who burns Ashbrook. Cunning, mocking, and later reveals he serves the Demon Lord's will.

**Vorathis, the Hollow King (the Demon Lord):** A towering armored figure with a hollow, glowing void for a face. Calm, tired, and terrifying. He does not hate the world. He wants it to end because he is exhausted. In truth he is Aurel, the first hero.

---

## 4. Story Structure: Five Acts, 25 Main Quests

Every quest lists: **Location, Summary, Key Beat, Reward**. Quest text must be expanded into full dialogue and objectives.

### ACT 1: AWAKENING (MQ01 to MQ04, Ashbrook Vale)

**MQ01: The Day of the Crystal**
- *Location:* Ashbrook Village.
- *Summary:* Tutorial. Kael does chores, learns movement and basic combat from villagers. Every 16-year-old in Veyrath undergoes the **Awakening Ceremony** at the village shrine. Kael's crystal shatters in gold light. The priest announces **S-rank**, the first in 300 years.
- *Key beat:* Villagers cheer, then go quiet and afraid. Kael feels isolated for the first time.
- *Reward:* Class: **Ascendant**. Skill: *Heroic Slash*.

**MQ02: Smoke on the Horizon**
- *Location:* Ashbrook outskirts.
- *Summary:* A demon scout, **Skarn**, arrives, drawn by the S-rank's energy. Combat tutorial 2 (dodge, block, skills). Kael fights imps and tries to protect his friends.
- *Key beat:* Kael fights well but not well enough. The village burns.
- *Reward:* *Dodge Roll*, *Guard*.

**MQ03: Ashes and Oaths**
- *Location:* Ashbrook ruins.
- *Summary:* Orwyn gives Kael his old sword **Dawnbrand** and tells him the truth about the Seals. Orwyn stays behind to hold Skarn off while Kael escapes with the survivors.
- *Key beat:* Orwyn's sacrifice. Kael swears he will never lose anyone again.
- *Reward:* Weapon: **Dawnbrand (Lv1)**. Skill: *Resolve* (survive lethal damage once per fight).

**MQ04: The Road to Aurelheim**
- *Location:* Whisperwood edge, Aurelheim gates.
- *Summary:* Kael leads refugees through bandit-infested roads, learning travel, camping and crafting basics. He arrives at **Aurelheim**, the guild capital.
- *Key beat:* First meeting with Darius, who mocks him.
- *Reward:* Fast travel unlocked; Aurelheim hub opens.

### ACT 2: THE GUILD RISE (MQ05 to MQ09, Aurelheim and surrounds)

**MQ05: Registered at Last**
- *Summary:* Kael registers at the Guild as **F-rank** (his Ascendant status is sealed by the Guildmaster for his own safety). Introduces the quest board, bounties, shops and trainers.
- *Key beat:* Hanna warns him: "Strong heroes die alone. Remember that."
- *Reward:* Guild card, 3 beginner quests.

**MQ06: Trial of the Proud**
- *Summary:* The Rank-Up Tournament. Kael must beat Darius in a duel to reach C-rank. Darius uses sword and fire arts.
- *Key beat:* Kael wins by a narrow margin. Darius is humiliated and vows revenge.
- *Reward:* C-rank, skill *Flame Parry* (from Darius's technique).

**MQ07: The Goblin Warrens**
- *Location:* Dungeon 1.
- *Summary:* Clear a goblin nest threatening trade routes. First dungeon with puzzles and a mini-boss (**Goblin Chieftain Grukk**).
- *Key beat:* Kael finds a demon-marked goblin, evidence of a larger plan.
- *Reward:* B-rank, first Seal clue.

**MQ08: The Hollow Mark**
- *Summary:* Investigate the demon mark. Hanna reveals the Seal Fragments and the Demon King's heirs. Kael finds Skarn's trail and fights him again (**boss: Skarn, Round 1**).
- *Key beat:* Skarn escapes. Kael realizes he cannot win alone.
- *Reward:* A-rank, skill *Sunder*.

**MQ09: Crown of Crisis**
- *Summary:* A demon cult attacks Aurelheim during the Founders' Festival. Kael loses control of Ascendant, and his golden power flares out of control and injures guards and bystanders.
- *Key beat:* **Ascendant Awakening I.** Afterward, Hanna names him **S-rank** publicly and sends him to retrieve the four Seal Fragments, but only with a party.
- *Reward:* S-rank, skill tree unlocked (3 branches), stat points.

### ACT 3: FORMING THE PARTY (MQ10 to MQ15, Whisperwood and Aurelheim)

**MQ10: Into the Whisperwood**
- *Summary:* Enter the forest where the first Seal Fragment (Life) is hidden. A wounded knight's trail leads to Fort Greywall.
- *Key beat:* Forest corruption is spreading, and the trees whisper Kael's name.
- *Reward:* Forest map, herbalism and crafting expansion.

**MQ11: The Knight Who Dropped His Shield** (recruits **Bram**)
- *Location:* Fort Greywall ruins.
- *Summary:* Bram is found drinking among the ruins. Kael helps him defend travelers from undead Greywall soldiers, which forces Bram to face his old squad's ghosts.
- *Key beat:* Bram raises his shield again and joins. First party member.
- *Reward:* **Bram joins.** Tank skills: *Taunt*, *Iron Wall*.

**MQ12: The Girl in the Forbidden Library** (recruits **Lyra**)
- *Location:* Spire Academy's Forbidden Library.
- *Summary:* Lyra's uncontrolled mana is awakening a library guardian. Kael and Bram must survive her magic storm and calm her.
- *Key beat:* Kael shields her with his own power, showing her she cannot hurt him. She joins out of curiosity, rivalry and relief.
- *Reward:* **Lyra joins.** Mage skills: *Fireball*, *Frost Lance*, *Arcane Shield*.

**MQ13: The Priestess's Secret** (recruits **Mirelle**)
- *Location:* Sanctum of Dawn.
- *Summary:* Cult-corrupted monsters attack the Sanctum. Mirelle heals the wounded despite visibly weakening.
- *Key beat:* Kael sees her fade after a big heal and demands she stop. She refuses, saying it is her duty, but agrees to travel with them to find a better way.
- *Reward:* **Mirelle joins.** Healer skills: *Heal*, *Cleanse*, *Blessing*.

**MQ14: Four Hearts, One Blade**
- *Location:* The Convergence Shrine.
- *Summary:* Party trial. Kael fights an illusion of his lonely self, and each member faces a mirror of their own fear. Teaches full party mechanics (role swaps, command system, resonance gauge).
- *Key beat:* **Bond Resonance** is born as a mechanic. The party shares a moment of power.
- *Reward:* **Bond Resonance** (party gauge), *Combo Link*.

**MQ15: The Thornqueen Falls** (General Boss 1)
- *Location:* Rootheart Hollow.
- *Summary:* Fight **Vessara** to claim the Seal of Life.
- *Key beat:* Vessara is a corrupted dryad. When defeated she weeps and thanks Kael, and she fades with a warning: "He is waiting at the end of the road, and he is tired."
- *Reward:* **Seal Fragment of Life**, Ascendant Awakening II.

### ACT 4: THE DEMON GENERALS (MQ16 to MQ22)

**MQ16: Into the Sunscar**
- *Location:* Sunscar Sands, Qasr Amun.
- *Summary:* Cross the desert, win the trust of the oasis tribes, and learn of the Hourglass Vault.
- *Key beat:* Party banter and camping scenes deepen bonds.
- *Reward:* Desert survival gear, mount unlocked.

**MQ17: The Hourglass Tyrant** (General Boss 2)
- *Location:* The Hourglass Vault.
- *Summary:* Fight **Khemet-Ur**, who can rewind time to undo damage and wounds.
- *Key beat:* Mirelle's Tithe is exploited by the General, who feeds on her life force. Kael reaches Ascendant limits to break time.
- *Reward:* **Seal Fragment of Time.**

**MQ18: The Frostspire Ascent**
- *Location:* Frostspire Peaks.
- *Summary:* Climb through whiteout storms. Lyra's magic nearly kills the party in an avalanche, and she freezes in guilt.
- *Key beat:* Bram teaches her that mistakes are survivable when you have people beside you.
- *Reward:* Cold-resist gear, *Mana Anchor*.

**MQ19: The Winter Choir** (General Boss 3)
- *Location:* Silent Choir Cathedral.
- *Summary:* Fight **Isrevna**, whose song silences spells and freezes time between notes.
- *Key beat:* Lyra must cast without chanting for the first time.
- *Reward:* **Seal Fragment of Spirit**, Ascendant Awakening III.

**MQ20: Betrayal at Aurelheim**
- *Location:* Aurelheim cathedral.
- *Summary:* The party returns to find Archbishop Caldrin has been feeding the Vital Tithe to the Demon Lord for centuries. He captures Mirelle to complete a ritual. Darius sides with the party for the first time.
- *Key beat:* Rescue Mirelle. Kael breaks the Tithe bond with Ascendant, which frees her but drains him. Caldrin is defeated (**mini-boss**) and arrested.
- *Reward:* Mirelle's Tithe is broken. **Darius joins as a temporary guest ally** in later battles.

**MQ21: Descent into Cinderhold**
- *Location:* Emberforge, Ashen Mines.
- *Summary:* The blacksmith guilds forge the party's final weapons. Navigate the mines while a volcanic eruption threatens the city.
- *Key beat:* Legendary gear crafting questline.
- *Reward:* Legendary weapon upgrade, blacksmith mastery.

**MQ22: The Molten Warden** (General Boss 4)
- *Location:* The Molten Keep.
- *Summary:* Fight **Pyrrhax**, a flame-armored giant who punishes anyone who does not guard.
- *Key beat:* Bram's greatest moment: he holds Pyrrhax's final blow for the whole party.
- *Reward:* **Seal Fragment of Flame**, Ascendant Awakening IV.

### ACT 5: THE DEMON LORD (MQ23 to MQ25)

**MQ23: The Truth of the Crown**
- *Location:* Aurelheim Archive, Convergence Shrine.
- *Summary:* The four Seal Fragments reassemble into the **Sealing Crown**. In its memory, the party sees the truth: Aurel was the first S-rank hero, a lone savior whose companions died or abandoned him, who sealed the Demon King by absorbing its despair, and became the Hollow King.
- *Key beat:* **Major twist.** Kael realizes he is the next vessel, and that Vorathis wants to hand him the burden and end the world, or end the cycle by dying himself. Kael decides to end it by *not* being alone.
- *Reward:* Sealing Crown (key item), **Ascendant Awakening V** (Bond Resonance).

**MQ24: Siege of the Hollow Gate**
- *Location:* The Hollow Reach.
- *Summary:* The Guild, the Church, Darius and everyone Kael helped across the story (side quest payoffs) rally into a grand army assault. The party breaks through the gate while the army holds the line.
- *Key beat:* Large set piece with NPC allies fighting alongside the player. Skarn returns for his final fight (**boss: Skarn, Final**) and is defeated.
- *Reward:* Access to Voidspire. Last chance to prepare and rest.

**MQ25: The Hollow King** (Final Boss)
- *Location:* Voidspire throne.
- *Summary:* See section 7.
- *Key beat:* The party defeats Vorathis and frees Aurel's spirit.
- *Reward:* Ending cutscene, credits, **New Game+**.

---

## 5. Companion Quest Chains

Each companion has **4 quests** (`CQ-<name>1` to `CQ-<name>4`) with bond thresholds. Higher bond unlocks dialogue, passive bonuses and a combo skill with Kael.

### Bram: "Vows of the Shield"
- **CQ-Bram1: Names on the Wall:** Visit Greywall's memorial and recover his squad's names.
- **CQ-Bram2: The Last Letter:** Deliver a squad member's last letter to their family. Their daughter forgives him.
- **CQ-Bram3: The Captain's Return:** Reclaim the Royal Shield Guard banner and face the General who blamed him.
- **CQ-Bram4: Hold the Line:** Defend Emberforge's refugees in a wave defense. *Reward:* Skill *Unbroken Vow*.

### Lyra: "The Girl Who Held the Storm"
- **CQ-Lyra1: Ink and Ashes:** Recover her burnt research notes.
- **CQ-Lyra2: The Academy's Apology:** Confront the Academy's Dean about her exile.
- **CQ-Lyra3: Control Is Not Fear:** A meditation and duel challenge in a mana storm.
- **CQ-Lyra4: The Spell Without a Name:** She creates her own signature spell with Kael. *Reward:* Skill *Starfall*.

### Mirelle: "A Light Worth Keeping"
- **CQ-Mirelle1: Cost of the Light:** Investigate the Tithe records.
- **CQ-Mirelle2: The Village She Healed:** Revisit a village that was healed by her in the past.
- **CQ-Mirelle3: A Day Off:** Take her on a festival day so she can live as herself.
- **CQ-Mirelle4: Dawn Without Debt:** After the Tithe is broken, she learns pure holy magic. *Reward:* Skill *Radiant Renewal*.

### Darius (guest ally): "The Second Place Prince"
- **CQ-Darius1 to 3:** Rivalry duels, a family secret, and a redemption mission. *Reward:* Unlocks as a temporary summon in the final siege.

---

## 6. The Demon Generals: Mechanics Sheet

| General | Region | Core Mechanic | What the party must do | Fragment |
|---------|--------|---------------|------------------------|----------|
| **Vessara, the Thornqueen** | Rootheart Hollow | Poison roots split the arena. Spawns thorn minions and heals from living vines. | Tank holds aggro; mage burns vines to stop healing; healer cleanses poison. | Life |
| **Khemet-Ur, the Hourglass Tyrant** | The Hourglass Vault | Rewinds the last 10 seconds of damage at set intervals. Time-sand zones slow players. | Burst damage right after a rewind; avoid sand zones; healer must not overheal on rewind. | Time |
| **Isrevna, the Winter Choir** | Silent Choir Cathedral | Song silences spells (mage cannot cast) and freezes party members. | Mage casts without a cast bar (instant skills); hero and tank carry fight; healer thaws allies. | Spirit |
| **Pyrrhax, the Molten Warden** | The Molten Keep | Fire armor reflects damage unless the tank breaks stance. Giant overhead slams. | Tank must block-break; hero attacks the exposed back; mage uses ice to cool the armor. | Flame |

Mini-bosses worth adding: Goblin Chieftain Grukk (MQ07), Archbishop Caldrin (MQ20), Skarn (MQ02, MQ08, MQ24).

---

## 7. Final Boss: Vorathis, the Hollow King (MQ25)

**Arena:** The Voidspire throne room, floating above a starless void. Shattered pillars drift as the fight progresses.

### Phase 1: The Weary King
- *Form:* Armored human-sized warlord with a void face.
- *Mechanics:* Heavy cleaves, summoned shadow blades, **Hollow Curse** (a debuff that blocks healing; the healer must cleanse it).
- *Role focus:* **Tank** keeps aggro; **healer** cleanses.
- *Dialogue:* Vorathis speaks quietly: "You think you are different. They always did."

### Phase 2: Shield of Despair
- *Mechanics:* Vorathis raises a barrier that reflects magic. **Void Rifts** open in the floor, and shadow clones of party members attack.
- *Role focus:* **Mage** must burst through the barrier using combined elements while the tank holds clones.
- *Dialogue:* He shows Kael the memory of Aurel's lost companions.

### Phase 3: The Solo Ascent
- *Mechanics:* Vorathis becomes a Void Colossus and knocks the party down with a gravity collapse. **The player, as Kael, must survive alone** for 60 seconds, which is the "solo ascent" the title mocks. Kael cannot win by damage; he must resist and endure.
- *Mechanic detail:* Each party member's portrait pulses. Pressing the **Call Out** button restores one ally at a time (Bram, Lyra, Mirelle), who rejoin with a powerful return skill.
- *Key beat:* Kael finally *asks for help*, which is the story's emotional peak. Each ally answers with a short line, and the music swells.

### Phase 4: Bond Resonance
- *Form:* Vorathis in true form, a giant armored Aurel with a cracked crown of void.
- *Mechanics:* Fast combos, screen-wide void waves, and limited-time windows. The party's **Resonance Gauge** is full.
- *Finisher:* **Quartet Strike.** Each member's ultimate is triggered in sequence (Bram's Unbroken Vow, Lyra's Starfall, Mirelle's Radiant Renewal, Kael's Ascendant Dawn) and chain into a final blow with a full anime impact-frame sequence.
- *Defeat:* Vorathis kneels. The void fades and reveals Aurel's human form.

### Aftermath
Aurel thanks them: "You did not carry it alone. That was the part I never learned." The Sealing Crown shatters into light, ending the cycle of despair. Aurel's spirit passes on.

---

## 8. Endings

- **Standard Ending:** Vorathis falls, the Demon Lord is vanquished, and the party returns to Aurelheim as heroes.
- **True Ending (requires all four companion quest chains plus Darius's chain):** Aurel's spirit is fully freed and blesses the party. Mirelle regains her full lifespan, Lyra opens a new Academy wing, Bram rebuilds the Shield Guard, Darius becomes Guildmaster's apprentice, and Ashbrook is rebuilt with Orwyn's spirit watching from the shrine. Post-credits: a hint of a new Awakening Ceremony where a child's crystal glows green.
- **New Game+:** Keep levels, skills, and gear. Enemy scaling increases, new dialogue references past events, and a hidden super-boss (**Aurel, Unbound**) unlocks at the Convergence Shrine.

---

## 9. Hero Progression: The Ascendant Awakenings

| Awakening | Trigger | Gameplay unlock | Visual |
|-----------|---------|-----------------|--------|
| **I: Stir** | MQ09 | Skill tree opens; golden aura on charged attacks | Faint gold glow |
| **II: Surge** | MQ15 | Dash-slash (*Gale Rush*), 2nd skill slot | Wind-like gold trails |
| **III: Radiance** | MQ19 | AoE ultimate *Dawn Arc* | Large halo and sunburst |
| **IV: Zenith** | MQ22 | Weapon evolves to **Dawnbrand: Zenith** | Full-body aura |
| **V: Resonance** | MQ23 | **Bond Resonance** party ultimate | Party-wide golden link lines |

---

## 10. Key NPCs (seed list)

Claude Code must create **at least 60 NPCs**. The 24 below are required; the remaining 36+ should fit the archetypes in 10.2. Every NPC needs a name, a personality line, a daily schedule (morning, afternoon, night, home/work/tavern), and at least one unique dialogue tree.

### 10.1 Named NPCs
| Name | Location | Role | Notes |
|------|----------|------|-------|
| Elder Orwyn | Ashbrook | Mentor | Dies in MQ03; appears as spirit later |
| Tilly Brooke | Ashbrook | Childhood friend | Survives; later runs a refugee camp |
| Brother Haldor | Ashbrook | Shrine priest | Runs the Awakening Ceremony |
| Hanna "Iron Hanna" Ostrovsk | Aurelheim | Guildmaster | Quest giver; stern, warm underneath |
| Pip Marlowe | Aurelheim | Guild receptionist | Comic relief, bounty board |
| Garrick Thorne | Aurelheim | Weaponsmith | Upgrades and crafting |
| Madame Selwyn | Aurelheim | Potion shop owner | Alchemy, gossip |
| Darius Valecrest | Aurelheim | Rival | See section 3.3 |
| Archbishop Caldrin | Aurelheim | Church leader | Secret villain |
| Dean Alaric Voss | Spire Academy | Academy dean | Lyra's antagonist and later ally |
| Captain Rhea Dunmore | Aurelheim | City guard captain | Siege commander |
| Old Fenn | Whisperwood | Hermit herbalist | Teaches herbalism |
| Kharim al-Sayyid | Qasr Amun | Desert tribe leader | Mount and desert quests |
| Zaira | Qasr Amun | Caravan merchant | Rare item trader |
| Anwen the Archivist | Khemet Ruins | Scholar | Time-lore quests |
| Skald Torvik | Hrimvald | Mountain guide | Climbing and ice quests |
| Sister Elowen | Hrimvald | Cathedral caretaker | Isrevna lore |
| Forgemaster Brandt Cinder | Emberforge | Master smith | Legendary gear |
| Ysolde Ashfall | Emberforge | Miner's guild leader | Mining quests |
| Nox | Voidspire approach | Information broker | Mysterious ex-cultist |
| Mayor Edda | Qasr Amun | Town leader | Politics side quests |
| Captain Marek | Hrimvald | Mountain patrol | Defense quests |
| Little Mira | Ashbrook / Aurelheim | Orphan child | Emotional side quest |
| The Wandering Bard, Lio | Roaming | Storyteller | Gives lore, plays music at camps |

### 10.2 Archetype templates for the other NPCs (36+)
Shopkeepers, innkeepers, trainers (sword, magic, holy, shield), blacksmith apprentices, herbalists, fishermen, farmers, guards, scholars, bards, merchants, orphans, retired adventurers, cult defectors, nobles, rival adventurer teams, travelling healers, stable masters, and cooks. Each should reference the region's culture and react to the story's progress (before and after major events such as MQ03, MQ09, MQ20 and MQ24).

---

## 11. Side Quests and Systems Hooks

Target **at least 50 side quests**, around 10 per region, plus repeatable guild bounties. Each side quest should give skills, items, bond points or lore.

### Quest categories
- **Guild Bounties (repeatable):** Hunt, gather, escort. Rank-gated.
- **Skill Trials:** Hidden trainers who teach skills when a challenge is completed (e.g., *The Monk of Whisperwood teaches Focus Strike*).
- **Lost Tomes:** Collect pages to unlock mage and healer skills.
- **Companion-linked:** Extra bond scenes at camps.
- **Craft and Legendary:** Collect rare materials for blacksmith legendary weapons.
- **Mystery and Lore:** Gradually reveal Aurel's past so the MQ23 twist feels earned. Examples: murals in Khemet that show a lone hero; Isrevna's songs that mention "the first light"; a Cinderhold forge-song about a tired knight.
- **Seasonal / Festival:** Founders' Festival mini-games, fishing contest, arena tournament.

### Example side quests per region
- **Ashbrook/Aurelheim:** *The Missing Lamb*, *Pip's Lost Ledger*, *The Rival Adventurers*, *Garrick's Rare Ore*, *Little Mira's Wish*.
- **Whisperwood:** *The Hermit's Remedy*, *Whispers in the Bark*, *Moonpetal Harvest*, *The Wolf Who Wasn't*.
- **Sunscar Sands:** *The Caravan Thieves*, *Sand Serpent Hunt*, *The Lost Mural*, *Zaira's Rare Spice*.
- **Frostspire Peaks:** *The Stranded Climbers*, *Songs in the Snow*, *The Frozen Lighthouse*, *Skald's Last Climb*.
- **Cinderhold:** *Ashen Miners Strike*, *The Singing Anvil*, *Forgemaster's Apprentice*, *Lava-Fish Special*.
- **Hollow Reach:** *Cult Defector Nox*, *Last Letters from the Front*, *The Soldier's Promise*.

**Payoff rule:** NPCs and quests the player completes should reappear in **MQ24 (Siege)** as allies, supply providers, or crowd cheers. Skipping them should still work but with a smaller army and fewer assists.

---

## 12. Writing Guide

- **Voice:** Anime-style, expressive, a mix of humor and sincerity. Short punchy lines in combat, longer ones in cutscenes.
- **Banter:** Camp scenes and exploration trigger party banter that depends on bond level and location.
- **Recurring motifs:** Golden light (hope), the hourglass (time and loss), the shield (protection), "You are not alone" (never said directly until MQ25).
- **Catchphrases:** Bram: "Behind me, kid." Lyra: "Don't hold back, or I'll be insulted." Mirelle: "Light will find a way." Kael: "I'll carry it!" (early) changing to "We'll carry it." (late).
- **Content rating:** Teen. No gore. Fantasy violence only.
- **Tutorial pacing:** Introduce mechanics through story moments, not text walls.
- **Localization:** Keep every line in string tables with IDs matching quest/NPC IDs.

---

## 13. Cutscene Master List

Cutscenes should use the in-engine camera with letterboxing, depth of field and a skip option.

1. Opening: Ashbrook at dawn, the Awakening Ceremony.
2. Village burning and Orwyn's sacrifice.
3. Kael vs Darius duel intro.
4. Festival disaster and Ascendant Awakening I.
5. Bram raises his shield.
6. Lyra's storm and Kael's shield.
7. Mirelle's fading light.
8. Convergence Shrine, Bond Resonance born.
9. Vessara's farewell.
10. Archbishop's betrayal and Mirelle's rescue.
11. Bram's last stand at the Molten Keep.
12. The truth of the Crown (flashback of Aurel).
13. The army gathers at the Hollow Gate.
14. "Kael asks for help" (Phase 3 turning point).
15. Quartet Strike impact-frame sequence.
16. Aurel's farewell and ending montage.
17. Post-credits teaser.

---

## 14. Implementation Notes for Claude Code

- Store story flags in a single `story_state` struct (act, current MQ, completed quests, bond levels, awakening level, endings flags). Save and sync it via Supabase.
- All dialogue must support **conditions** (`if bond.bram >= 3`, `if quest.CQ-Mirelle4 == complete`) and **variables** (`{player_name}`).
- Quest JSON should include: `id`, `act`, `giver`, `region`, `prerequisites`, `objectives[]`, `rewards{xp, items, skills, bond}`, `on_complete_flags[]`, and `dialogue_refs[]`.
- Boss JSON should include phase thresholds, mechanic scripts, and enrage timers matching section 6 and section 7.
- Provide a `/tools/validate_story.c` (or script) that checks all quest chains reach MQ25, no quest is unreachable, and every referenced NPC and item exists.
