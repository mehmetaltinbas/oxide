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

Add the biomes to the row in `sim/src/npc.cpp` and nothing else. `populate`
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
next to a deer and asserts that the wolf both goes for it and catches it.

## The roster

| Animal  | Lives in      | HP  | Speed | Rank | Temper   | Hunts             | Runs from              |
| ------- | ------------- | --- | ----- | ---- | -------- | ----------------- | ---------------------- |
| Chicken | grass, forest | 15  | 104   | 0    | harmless | nothing           | everything, and you    |
| Deer    | forest, snow  | 120 | 172   | 1    | harmless | nothing           | hyena, wolf, bear, you |
| Hyena   | desert, grass | 95  | 176   | 3    | hostile  | chicken, deer     | wolf, bear             |
| Wolf    | grass, snow   | 80  | 184   | 4    | hostile  | chicken, deer     | bear                   |
| Bear    | forest, snow  | 340 | 92    | 9    | hostile  | deer, wolf, hyena | nothing                |

A player walks at 132 and sprints at 185, which is the number the rest are set
against: a deer at 172 can be run down on foot and you will arrive with nothing
left, a wolf at 184 is just slower than a sprint, and a bear at 92 never
catches anybody who runs.

Anything skittish bolts when you come within 260 units of it, not only once you
have hit it: you do not walk up to a deer.

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

Populations, for a map of the reference size: 120 chickens, 95 deer, 80 wolves,
26 bears. The wolf covers the desert as well as the grass and the tundra, which
is what keeps the sand from being a free walk. Everything killed comes back where it lived a day later,
the same rule every other resource on the island follows.
