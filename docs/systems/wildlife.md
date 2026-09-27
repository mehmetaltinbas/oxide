# Wildlife

What lives on the island, where it lives, and who eats whom. Read this before
adding an animal or changing one's numbers.

## An animal belongs to a country

### Rule

Every animal names the biomes it lives in, and is only ever placed in one of
them. A definition with no biomes is not wildlife: it is placed by hand, like
the people who hold the monuments.

### Why

An animal that can turn up anywhere tells you nothing about where you are. When
a deer is a thing you find in the trees and the snow, walking into the trees
becomes a decision, and the desert being nearly empty is information rather
than an oversight.

### How to apply

Add the biomes to the row in `sim/features/wildlife/constants/npc-defs.constant.cpp` and nothing else. `populate`
rejects any spot whose biome is not one of them, and `play_check` counts how
many ended up in the wrong country: the answer has to stay zero.

## Rank decides who runs, a list decides who hunts

### Rule

Each animal carries three numbers that make up the whole food chain:

- `rank`, how formidable it is.
- `eats`, the kinds it hunts unprovoked.
- `skittish`, whether it bolts from anything that outranks it.

Nothing attacks anything that is not on its own list. Anything skittish runs
from a higher-ranked hunter, and from you once you have taken a swing at it.
Running beats hunting: a wolf with a bear on it forgets the deer.

### Why

Two fields and a flag give a believable island without a behaviour tree per
species, and they are readable as data: you can see the whole chain by reading
one table instead of five state machines.

### How to apply

A chase only resolves if the hunter is faster than the prey. Set the speeds
against each other deliberately, and check with `play_check`, which puts a wolf
next to an elk and asserts that the wolf both goes for it and catches it. That
check earns its keep: the chase branch once tested the state it had itself set
the frame before, so a wolf went for an elk for exactly one tick and then
forgot about it, and nothing else would have caught that.

## The roster

| Animal  | Lives in      | HP  | Speed | Rank | Temper   | Hunts             | Runs from              |
| ------- | ------------- | --- | ----- | ---- | -------- | ----------------- | ---------------------- |
| Chicken | grass, forest | 15  | 104   | 0    | harmless | nothing           | everything, and you    |
| Deer    | forest, snow  | 120 | 172   | 1    | harmless | nothing           | hyena, wolf, bear, you |
| Hyena   | desert, grass | 95  | 176   | 3    | hostile  | chicken, deer     | wolf, bear             |
| Wolf    | grass, snow   | 80  | 184   | 4    | hostile  | chicken, deer     | bear                   |
| Bear    | forest, snow  | 340 | 92    | 9    | hostile  | deer, wolf, hyena | nothing                |

A player walks at 132 and sprints at 185, which is the number the rest are set
against: an elk at 165 can be run down on foot and you will arrive with nothing
left.

Every threat has the same shape: **faster than a walk, slower than a sprint**.
A bear at 150 runs down anybody who strolls away from it and never catches
anybody who sprints. It was 92, at which it could not catch a walking player
either, so the thing that is supposed to make the forest somewhere you do not
go yet was something you ambled past.

The wolf's 176 is the number the rest hangs off, and it is pinned from both
sides. It has to sit **under a sprint**, or a wolf is a death sentence rather
than a decision. It has to sit **over an elk's 165**, or it never eats and the
food chain does nothing. That leaves nine units of margin against a sprint,
which is a break you have to commit to rather than one you stroll away with. It
was 184 for a while, a sprint to within a rounding error, and you could not get
away from one at all.

`play_check` asserts both halves: `outrun` sprints a player away from a wolf
for ten seconds and reports the gap, and `chain` puts a wolf on an elk and
requires it to catch one.

Anything that runs from you bolts within 150 units, not only once you have hit
it: you do not walk up to an elk. It was 260, at which range everything you
could see was already running, so the herds were never seen grazing. Roaming
itself is an amble at a sixth of top speed, because a grazing animal crossing
the field at a third of it reads as one already running from something.

### What each is worth

Per kill, as a range. A kill is worth roughly what the animal was worth alive,
so a deer pays for the walk into the forest and a chicken does not pay for the
arrow.

| Animal  | Raw meat | Leather  | Bone     | Animal fat |
| ------- | -------- | -------- | -------- | ---------- |
| Chicken | 1 to 2   | none     | 1 to 2   | 0 to 1     |
| Deer    | 5 to 9   | 14 to 24 | 6 to 11  | 5 to 10    |
| Hyena   | 2 to 4   | 5 to 10  | 4 to 8   | 2 to 4     |
| Wolf    | 2 to 4   | 6 to 12  | 3 to 6   | 2 to 5     |
| Bear    | 8 to 16  | 20 to 36 | 10 to 20 | 14 to 30   |

Populations, for a map of the reference size: 130 rabbits, 85 elk, 70
kangaroos, 80 wolves, 26 bears. The wolf covers the desert as well as the grass
and the tundra, which is what keeps the sand from being a free walk. Everything killed comes back where it lived a day later,
the same rule every other resource on the island follows.
