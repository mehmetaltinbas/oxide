# Oxide

A top-down take on Rust: land on an island with a rock, work your way up to
sheet metal and explosives, and hold what you built.

**C++ with SDL3, no engine and no art assets.** The island, every item, every
animal and every sound are made in code. One rules library shared by the game
and the server, so the two cannot disagree about what happened.

## Build and run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

```bash
./build/bin/oxide
```

A launch is a new island. The title screen offers **Sandbox**, which is the
game with the whole recipe book open and free, and **Multiplayer**, which is
listed and not built yet. `C` carries on from your last island if there is one.
A number on the command line pins the island: `./build/bin/oxide 12345`.

### Controls

WASD to move, shift to run. Left click uses what is in your hand, right click
draws a bow. 1-6 pick a belt slot, R reloads, E uses whatever is in front of
you, G drops it, TAB opens the pack, C the bench, M the map, B changes what the
building plan puts down, H lists all of this. Wheel zooms. Minus and equals set
the volume.

## What is in the repository

```
CMakeLists.txt     the three binaries and the fetched libraries
sim/               the rules of the game. No SDL, no sockets, no platform
client/            the game you play
server/            the headless authority
tools/             the checks
assets/            fonts
docs/              architecture, design and system conventions
```

`sim/` is the point of the whole thing. The client and the server link the same
rules, so a wall is the same thickness in both because it is the same number,
not because somebody kept two copies in step.

## Checks

```bash
./build/bin/play_check     # plays a stretch of the game with nobody watching
./build/bin/island_check   # generation: biomes, node counts, monuments
./build/bin/sound_check    # renders every sound with no sound card
./build/bin/net_check      # two players against a running oxide_server
```

Every one of them exists because something broke and nobody noticed. See
[docs/testing.md](docs/testing.md) for the flags that draw one frame of any
state you want to look at.

## The loop

1. **Gather** with a rock, then craft a hatchet and pickaxe. Hatchets favour
   wood, pickaxes favour ore, and the rock is bad at both.
2. **Survive**: food and water drain constantly, night is nearly black and a
   torch is your first craft, snow will freeze you, and radiation at monuments
   will kill you without a hazmat suit.
3. **Smelt** ore in a furnace, **cook** meat on a campfire. Both need wood and
   `E` to light.
4. **Build** a base, place a tool cupboard to claim the ground, upgrade from
   twig through wood and stone to sheet metal with the hammer.
5. **Loot** monuments for scrap and high quality metal, which is what the best
   armour is made of.
6. **Hold it.**

## The island

Temperate in the middle, snow at the north end and desert at the south, with
the bands broken up by a moisture field rather than drawn as stripes. Wherever
grass or forest runs into the sea it turns to sand first; snow goes straight
into the water and desert is sand already, so neither grows a beach.

Those beaches are the spawn. A fresh character, and every respawn without a
sleeping bag, washes up on a random stretch of sand, never inside somebody
else's claim.

## Wildlife

Four animals and a neutral, each answering a question no other one answers.
Full table in [docs/systems/wildlife.md](docs/systems/wildlife.md).

| Animal | Lives in | Temper |
| --- | --- | --- |
| Rabbit | grass, forest | runs from everything |
| Elk | forest, snow | runs from you, and is a week of food |
| Kangaroo | grass, desert | neutral until you hit it, then it kicks |
| Wolf | grass, snow, desert | comes for you, and a sprint only just escapes |
| Bear | forest, snow | comes for you, and 340 health |

They hunt each other too: a wolf runs down an elk, a bear runs down the wolf.

## Survival

| System | Detail |
| --- | --- |
| Food and water | Drain constantly. Hunt, cook, and drink at any shoreline with `E` |
| Temperature | Biome, time of day, clothing and nearby fire. Freezing does steady damage |
| Radiation | Builds at monuments, decays outside them. A hazmat suit blocks most of it |
| Bleeding | Some hits open you up. Bandages stop it |
| Death | You drop everything where you fell, the bench included. A sleeping bag decides where you wake |

A full day is an hour: forty five minutes of light and fifteen of dark.

## Building

Rust's building system, adapted for a single floor:

- **Foundations** sit on grid cells. **Walls, doorways and doors** sit on the
  edges between cells and need one of the two neighbouring cells to already be
  your foundation.
- Everything starts as **twig** and is upgraded one tier at a time with the
  hammer: wood, stone, sheet metal.
- Every wall has a **soft side**, marked with a pale dot. Melee does a tenth as
  much to the hard side, so build with the soft side facing inward.
- A **tool cupboard** claims a radius around it. Nobody builds inside someone
  else's claim.
- Nothing goes up through a tree, a rock or a nettle. Clear the ground first,
  and it grows back on its own timer once the building is gone.

A room is a patch of ground the open air cannot reach, and a room you are not
in is drawn as a roof. A room needs a floor, so ringing a stretch of forest
with buildings does not roof the woodland. A hole anywhere opens all of it: one
wall off a six-cell hall takes the roof off the hall.

A charge breaks what it is stuck to and nothing else, so raiding is placing
charges one at a time rather than lobbing them at a compound. The blast still
catches anything alive nearby, yours included.

## Monuments

Six of them, from the unguarded to the lethal: **Abandoned Cabins**,
**Lighthouse**, **Ashvale**, **Airfield**, **Power Plant** and **Military
Base**. The last three are irradiated and held by scientists and soldiers, and
they are where scrap, high quality metal, rifles and hazmat suits come from.

## Docs

[docs/README.md](docs/README.md) is the index: the architecture, the design
tokens, and a page per system with the rule it holds and the mistake it exists
to prevent. [CLAUDE.md](CLAUDE.md) is the condensed checklist.
