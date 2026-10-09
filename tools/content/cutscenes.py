"""Cutscene scripts (dialogue-only for now; the in-engine camera sequences arrive with the story-content milestone)."""
from dsl import convo

def cs(name, steps): return convo(f"cs.{name}", steps)

cs("awakening", [
    ("narrator", "Ashbrook at dawn. Frost on the thatch, smoke from a dozen chimneys, and sixteen-year-old {player_name} standing at the shrine steps trying very hard not to look nervous."),
    ("haldor", "The Awakening Ceremony is older than the village. Every child of sixteen places a hand on the crystal. It answers with a color, and a rank. Step forward."),
    ("kael", "It's just a rock. It's just a rock. It's just a rock."),
    ("narrator", "The crystal wakes. Gold light, brighter than the sunrise, floods the shrine. The glass cracks like ice on a pond and does not stop glowing."),
    ("haldor", "...S-rank. The first in three hundred years. Ascendant."),
    ("narrator", "The villagers cheer. Then, one by one, they fall quiet. Nobody meets his eyes."),
    ("kael", "Why is everyone looking at me like that?"),
    ("tilly", "Like what? Like you're the best thing that ever happened to this village? Don't be stupid. Come on. Let's go."),
    ("narrator", "She laughs. It sounds a little too bright. For the first time in his life, {player_name} feels alone in a crowd."),
])
cs("skarn_arrives", [
    ("narrator", "Evening. The sky over Ashbrook turns the color of a bruise."),
    ("voice", "So that's where the light came from. A farm village. How delightfully small."),
    ("skarn", "Don't mind me, little hero. I'm only the scout. The real visitors come after you've been properly... collected."),
    ("kael", "Get out of our village!"),
    ("skarn", "Oh, I like him. He thinks he has a say."),
])
cs("village_burns", [
    ("narrator", "Fire. The mill, the bakery, the roof of the old shrine. The imps are gone, but the flames are not."),
    ("tilly", "Kael, the bridge! The children are at the bridge!"),
    ("kael", "I had him. I almost had him. I could have--"),
    ("skarn", "Not bad for a boy. Come find me when you've grown up. I'll still be here. Burning things."),
    ("narrator", "{player_name} stands in the ashes with his hands shaking. The glow around him has gone dim, as if ashamed."),
])
cs("orwyn_sacrifice", [
    ("orwyn", "Listen, boy. This sword was mine before it was yours. Plain iron. It has never failed me and it never asked me to be a hero."),
    ("orwyn", "The Seals are failing. A demon king sleeps under the world and a crown keeps him there. The Crown is in pieces. You will find them. And you will not do it alone."),
    ("kael", "Grandfather, come with us. Please. There's room on the cart."),
    ("orwyn", "There's never room for the old on a cart. Go. Take the survivors. I'll keep the road closed behind you."),
    ("kael", "I'll carry it! I'll carry all of it, I swear!"),
    ("orwyn", "Hah. That is exactly the kind of thing a boy says before he learns how heavy it is. ...Go on. I'm proud of you."),
    ("narrator", "The cart rattles down the road. Behind them, a small figure in grey robes stands in front of the burning gate. Light rises from his hands. Then the road goes quiet."),
    ("kael", "I'll carry it. I swear on the sword."),
], )
cs("darius_mocks", [
    ("narrator", "Aurelheim's gates, mid-morning. Merchants, guards, adventurers, and a boy covered in road dust holding a plain iron sword."),
    ("darius", "Look at that, a farm boy. Does the guild let anyone in now? Or did they run out of competent applicants?"),
    ("kael", "I'm just here to register."),
    ("darius", "Of course you are. Try not to trip over your sword. It'd be embarrassing for both of us."),
    ("tilly", "Is he always like this?"),
    ("kael", "I don't know. I only just met him."),
])
cs("duel_intro", [
    ("narrator", "The Rank-Up Arena. Sand, torchlight, a crowd of Aurelheim's finest betting on the new boy."),
    ("hanna", "Rank C trial. Kael of Ashbrook against Darius Valecrest. First yield or first fall. No killing, no cheating, no complaining."),
    ("darius", "I'll be gentle. I want you to remember this fight. For the rest of your short career."),
    ("kael", "Just draw your sword."),
    ("darius", "Fine. En garde, farm boy."),
])
cs("festival_disaster", [
    ("narrator", "The Founders' Festival. Lanterns, music, a river of laughing people, and, beneath the cathedral steps, a circle drawn in ash."),
    ("voice", "In the name of the Hollow King. Let the light break."),
    ("narrator", "Cultists in violet robes throw back their cowls. The crowd screams. Guards shout. A wave of black fire rolls across the plaza."),
    ("kael", "Everyone get behind me! I'll... I'll handle it!"),
    ("narrator", "He pulls the power up. The crystal's gold floods his arms, his chest, his eyes. It does not stop where he wants it to. Golden light tears across the plaza, throwing guards off their feet, cracking stone."),
    ("hanna", "{player_name}! Control it! You're hurting them!"),
    ("kael", "I can't! It won't stop! I can't--"),
    ("narrator", "A hand catches his shoulder. Hanna's. The light gutters. Silence, apart from the crying."),
    ("hanna", "You are an S-rank. You are not a god. And you are not going to carry this alone. Understood?"),
])
cs("bram_shield", [
    ("narrator", "Fort Greywall. The walls are rubble. The ghosts are gone. Only a man sits on the broken steps, a cracked shield across his knees."),
    ("bram", "You killed my old squad. You know that? Forty of them. Not for malice. They just didn't know they'd died."),
    ("kael", "They were asking for you. By name."),
    ("bram", "...Of course they were."),
    ("narrator", "Bram stares at the shield for a very long moment. Then he stands. The shield is heavy. He holds it anyway."),
    ("bram", "Behind me, kid. I don't know if I deserve to say it. But I'm going to."),
])
cs("lyra_storm", [
    ("narrator", "The Forbidden Library. Books spin through the air. Arcane lightning crawls along the shelves. In the middle of it, a girl with silver hair and tears on her face."),
    ("lyra", "Stay back! I can't stop it! It just keeps coming out and I can't stop it!"),
    ("bram", "Kid. She's going to take the roof off."),
    ("kael", "I know."),
    ("narrator", "He walks into the storm. The lightning strikes him, and he does not flinch. Gold flares around him, pushing the chaos back."),
    ("kael", "See? I'm still standing. You can't hurt me."),
    ("lyra", "...Nobody has ever stood in it before."),
    ("kael", "Then it's about time."),
    ("lyra", "Don't hold back or I'll be insulted. Rival."),
])
cs("mirelle_fading", [
    ("narrator", "The Sanctum of Dawn. Wounded pilgrims fill the aisles. In front of the altar, a priestess kneels, hands glowing."),
    ("mirelle", "One more. Just one more. Light will find a way."),
    ("kael", "Sister, your hair. It's... it's white at the roots."),
    ("mirelle", "It's nothing. It's the light. It happens."),
    ("narrator", "She smiles. Then her knees give way. Kael catches her. She weighs almost nothing."),
    ("kael", "That wasn't light. That was you."),
    ("mirelle", "It's the Tithe. It's my duty. Please don't tell anyone."),
    ("kael", "Then let me carry some of it. Come with us. We'll find another way."),
    ("mirelle", "...Light will find a way."),
])
cs("resonance_born", [
    ("narrator", "The Convergence Shrine. Four pedestals, four faint circles of light. A mirror at the center, and in the mirror, four people who are not quite themselves."),
    ("kael", "That's me. Standing alone. Always alone."),
    ("bram", "That's me. Sitting alone, letting them die."),
    ("lyra", "That's me. Alone in the ruins I made."),
    ("mirelle", "That's me. Smiling. Alone."),
    ("narrator", "They reach out. Not to the mirror. To each other. The four lights meet and braid into one."),
    ("kael", "It's warm."),
    ("lyra", "It's loud."),
    ("bram", "It's... steady."),
    ("mirelle", "Light will find a way. Together."),
])
cs("vessara_farewell", [
    ("narrator", "Rootheart Hollow. The thorns wither. The Thornqueen sinks to her knees, vines peeling away from a face that is almost human."),
    ("vessara", "Thank you. I could not stop myself. Every year the green grew darker and every year I forgot why I was so tired."),
    ("kael", "We're sorry."),
    ("vessara", "Do not be. You are carrying so much, little light. Listen. He is waiting at the end of the road. And he is tired."),
    ("mirelle", "Who is?"),
    ("vessara", "The one who wears the Crown's shadow. Be kind. He has been alone so long."),
    ("narrator", "She fades into green light. A single fragment of the Crown remains, pulsing like a heart."),
])
cs("avalanche", [
    ("narrator", "Whiteout Pass. A crack like thunder. The mountain slides."),
    ("lyra", "That was me! That was my fire! I thought it was small and it just kept going and I--"),
    ("bram", "Lyra. Breathe."),
    ("lyra", "People could have died! You could have died!"),
    ("bram", "Kid, listen. I made a mistake that cost forty lives. You made one that cost a bruised shoulder and a lost scarf."),
    ("bram", "Mistakes are survivable when you have people beside you. That's what I got wrong back then. Nobody beside me."),
    ("lyra", "...Okay. Okay. I'm here."),
    ("kael", "We're all here."),
])
cs("betrayal", [
    ("narrator", "Aurelheim Cathedral. Candlelight. A ritual circle in the nave, and in its center, a girl in white."),
    ("caldrin", "Nine hundred years of Tithes, child. Every drop of life you spend, the Church banks. Every drop of life the Church banks goes to the one who never stopped waiting."),
    ("mirelle", "You told me it was a gift."),
    ("caldrin", "It was. For him. Please do be still."),
    ("darius", "Archbishop. Your Grace. Step away from her."),
    ("kael", "Darius?"),
    ("darius", "Don't. I'm angry enough already. I followed the ledgers. Valecrest signatures on every page."),
    ("narrator", "Gold light cracks the ritual circle. Mirelle gasps. The chain of light binding her to the altar shatters."),
    ("mirelle", "...I can breathe."),
])
cs("bram_stand", [
    ("narrator", "The Molten Keep. Pyrrhax raises a fist of living fire over the party. Lyra is down. Mirelle is down. Kael is on one knee."),
    ("bram", "Behind me, kid. All of you. Behind me."),
    ("kael", "Bram, it'll crush you!"),
    ("bram", "I've been crushed before. I keep getting up. It's a talent."),
    ("narrator", "He plants the Bulwark of Oaths. The blow lands. Stone cracks. Armor sparks. Bram does not move."),
    ("bram", "FORTY of them I left. Not one more. Not today. Not ever!"),
    ("narrator", "The shield blazes with the names engraved in it. Pyrrhax staggers back."),
])
cs("crown_truth", [
    ("narrator", "Aurelheim Archive. The four fragments rise, spin, and lock into a crown of white light. Memory pours out of it."),
    ("aurel", "I was the first. They called me Ascendant. They told me the world's pain had chosen me."),
    ("aurel", "I sealed the Demon King. I took his despair into myself so the world could be spared it. My companions died, or left, or lost their nerve."),
    ("aurel", "I carried it alone for three hundred years. Slowly I became what I held."),
    ("kael", "That's Vorathis. The Hollow King. He's you."),
    ("aurel", "He is what I became. And he is waiting for the next Ascendant to take the weight. Or to end it."),
    ("lyra", "So the talent isn't a gift."),
    ("mirelle", "It's the world dumping its despair onto one person."),
    ("bram", "Then we break the pattern. All of us. Together."),
    ("kael", "We'll carry it."),
])
cs("army_gathers", [
    ("narrator", "The Hollow Gate. A wall of black stone, a sky like a bruise, and the largest army Veyrath has ever raised."),
    ("dunmore", "Aurelheim Watch, form up! Guild lines, on me!"),
    ("hanna", "Every adventurer in the capital. Every rank. I swore I'd never sign this many casualty forms."),
    ("darius", "Don't die. I haven't finished beating you."),
    ("kael", "Everyone stood up for us. Even the people we barely knew."),
    ("bram", "That's what a line is, kid. It's the people who stay."),
])
cs("asks_for_help", [
    ("narrator", "Voidspire's throne. Gravity collapses. The party is flung down. Kael stands alone against a colossus of shadow."),
    ("vorathis", "Sixty seconds. Show me the strength of one who needs no one."),
    ("kael", "I can... I can do this. I can handle it. I always handle it."),
    ("narrator", "A hand slams him to the ground. The glow around him gutters."),
    ("kael", "...I can't."),
    ("kael", "Bram. Lyra. Mirelle. I can't do this alone. I need you."),
    ("bram", "Took you long enough, kid. Behind me."),
    ("lyra", "Don't hold back. I'm right behind you."),
    ("mirelle", "Light will find a way. Stand up, Kael. We're here."),
])
cs("quartet_strike", [
    ("narrator", "Four lights, one rhythm. Bram's vow. Lyra's star. Mirelle's dawn. Kael's blade."),
    ("bram", "UNBROKEN VOW!"),
    ("lyra", "STARFALL!"),
    ("mirelle", "RADIANT RENEWAL!"),
    ("kael", "ASCENDANT DAWN!"),
    ("narrator", "The throne room shatters in gold."),
])
cs("aurel_farewell", [
    ("narrator", "The void fades. A man kneels where the Hollow King stood: tall, tired, human. Aurel."),
    ("aurel", "You did not carry it alone. That was the part I never learned."),
    ("kael", "We'll carry it. All of it."),
    ("aurel", "Then it was not wasted. Thank you. All of you."),
    ("narrator", "The Sealing Crown shatters into light. The cycle ends. Aurel's spirit passes on, smiling."),
])
cs("ending_montage", [
    ("narrator", "Aurelheim, a week later. The party returns to cheering streets. Hanna reads out the heroes' names. Pip faints."),
    ("narrator", "Ashbrook is rebuilt, board by board. A small shrine stands where the old one burned. Orwyn's spirit watches the sunrise from its steps."),
    ("kael", "We'll carry it. Together."),
])
cs("post_credits", [
    ("narrator", "A small village. A child steps up to the shrine. The crystal flares. This time it glows green."),
    ("haldor", "...A new color. Interesting."),
])
