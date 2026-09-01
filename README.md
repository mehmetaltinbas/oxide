# Oxide

A top-down 2D take on Rust: land on an island with a rock, work your way up to
sheet metal and explosives, and hold what you built against three AI clans who
will eventually come for it.

TypeScript + HTML5 canvas, no engine, no art or audio assets, terrain, items and
sound are all generated. Vite for dev/build.

## Run it

```bash
npm install
```

```bash
npm run dev
```

Default port is 5173; this project is usually run on 5174 alongside Lumberwave:

```bash
npm run dev -- --port 5174
```

## Controls

| Key        | Action                                                                |
| ---------- | --------------------------------------------------------------------- |
| WASD       | Move · Shift to sprint                                                |
| Left mouse | Use the held item: gather, attack, shoot, place, throw, eat           |
| 1-6        | Belt slots                                                            |
| TAB        | Opens the pack. Closes whatever screen is open, whichever one it is   |
| C          | Crafting (a tab of the same screen: click across without closing)     |
| E          | Interact: doors, boxes, furnaces, crates, and drinking at a shoreline |
| R          | Reload the held weapon                                                |
| G          | Drop the held item                                                    |
| Q / wheel  | With the Building Plan out: pick foundation / wall / doorway / door   |
| Wheel      | Zoom · `H` help · `Esc` pause                                         |
| F, [ , ]   | Sandbox only: item shelf, and the clock an hour back or forward       |

**Sandbox** is the third button on the title card. Creative mode: free building, free and instant
crafting at any tier, infinite ammo, nothing can hurt you, and a shelf with every item in the game
on it. It is for testing things, not for playing.

## The loop

1. **Gather** with a rock, then craft a hatchet and pickaxe. Hatchets favour
   wood, pickaxes favour ore, and the rock is bad at both.
2. **Survive**: food and water drain constantly, nights and snow will freeze you,
   and radiation at monuments will kill you without a hazmat suit.
3. **Smelt** ore in a furnace, **cook** meat on a campfire (both need wood as fuel
   and `E` to light).
4. **Build** a base, place a tool cupboard to claim the ground, upgrade from twig
   through wood and stone to sheet metal with the Hammer.
5. **Loot** monuments for scrap, which unlocks the higher workbench tiers.
6. **Hold it.** From day 2 onward, clans raid you with satchels and C4.

Everything saves automatically, and your base decays while you are away.

## Building

Rust's building system, adapted for a single floor:

- **Foundations** sit on grid cells. **Walls, doorways and doors** sit on the
  _edges_ between cells, and need one of the two neighbouring cells to already
  be your foundation.
- Everything starts as **twig** (10 hp) and is upgraded one tier at a time with
  the Hammer: **wood** 250 → **stone** 500 → **sheet metal** 1000.
- Every wall has a **soft side**, marked with a pale dot. Melee does 10% damage
  to the hard side, so build with the soft side facing inward.
- A **tool cupboard** claims a radius around it. Nobody can build inside someone
  else's claim, which is what stops a raider walling you in.
- Doors can be opened with `E`. Clan doors are locked, so you break them instead.

Raid costs work out roughly as they should: a satchel does 475 and C4 does 550,
so stone walls take two satchels and sheet metal takes four.

## The shore

The island is temperate in the middle, snow at the north end and desert at the south, with the
bands broken up by the moisture field rather than drawn as clean stripes. Wherever grass or forest
runs into the sea it turns to sand first. Snow goes straight into the water and desert is sand
already, so neither grows a beach.

Those beaches are the spawn. A fresh character, and every respawn without a sleeping bag, washes up
on a random stretch of sand anywhere on the coast, never inside a tool cupboard's claim, so you do
not wake up in somebody's base. The clans are placed before you are, so the check sees their
cupboards.

## Survival

| System       | Detail                                                                         |
| ------------ | ------------------------------------------------------------------------------ |
| Food & water | Drain constantly. Hunt boar for meat, cook it, drink at any shoreline with `E` |
| Temperature  | Biome + time of day + clothing + nearby fire. Freezing does steady damage      |
| Radiation    | Builds at monuments, decays outside them. A hazmat suit blocks 90%             |
| Bleeding     | Some hits open you up. Bandages stop it                                        |
| Death        | You drop **everything** where you fell. A sleeping bag decides where you wake  |

## Roofs and rooms

A room is a patch of ground that the open air cannot walk to: sealed by walls, with doorways
counting as gaps and a shut door counting as a wall. A room you are not standing in, and hold no
cupboard authority over, is drawn as a roof.

Two rules keep that honest:

**A room needs a floor.** Sealing off open ground, a courtyard inside a compound, or a stretch of
forest that a few buildings happen to ring in between them, still counts as enclosed for reach and
privacy, but nothing goes over it. Without this the map grew dark slabs of "ceiling" across open
woodland with the trees still drawn on top of them.

**A hole anywhere opens all of it.** The room is one region however many foundations it spans, so
blowing one wall of a six-cell hall takes the roof off the whole hall, not off the square the wall
happened to touch.

## Explosives

A charge breaks what it is stuck to and nothing else. A satchel on a wall takes that wall down and
leaves the wall beside it at full health, which is what makes raiding a matter of placing charges
one at a time rather than lobbing them at a compound and watching it fall over. The blast still
catches anything alive nearby, yours included, so standing next to your own charge is a mistake.

A charge that never stuck to anything, thrown into open ground, only hurts whoever is standing
there.

## Building on natural ground

Nothing goes up through a tree, a rock or a hemp bush. Clear the ground first and the spot frees up.
This holds for the clans too: they fell their plot before they raise a compound, which is why their
houses no longer have trees standing in the middle of a room. Anything cleared grows back on its
normal timer, but not while a building stands on it, and not half-overlapping one either.

## Monuments

| Monument    | Radiation | Scientists | Loot                                                |
| ----------- | --------- | ---------- | --------------------------------------------------- |
| Lighthouse  | none      | 1          | Low tier: scrap, cloth, fuel                        |
| Airfield    | moderate  | 4          | Scrap, metal, sulfur, pistol ammo                   |
| Power Plant | heavy     | 7          | The best in the game, including C4 and hazmat suits |

## The clans

The island supports **2 to 5 clans at a time**, re-rolled within that range as
the game runs. Each one has its own name, colour and skill, and the skill decides
everything about them:

| Skill    | Compound | Built from  | Members | Tech/day | Raid pace | Loot            |
| -------- | -------- | ----------- | ------- | -------- | --------- | --------------- |
| Amateur  | 2x2      | Wood        | 2       | 0.5      | 0.6x      | 0.5x            |
| Seasoned | 3x3      | Stone       | 4       | 1        | 1x        | 1x              |
| Veteran  | 4x4      | Stone       | 6       | 1.6      | 1.35x     | 1.8x            |
| Expert   | 4x4      | Sheet metal | 8       | 2.4      | 1.7x      | 3x (carries C4) |

Amateurs are common and experts are rare, so most islands have a couple of soft
targets and one outfit you do not want to annoy. Bigger clans field a higher
share of raiders over gatherers.

**Wiping a clan** means destroying its tool cupboard. Its survivors scatter and
walk off the map, and the plot is free.

**The island refills itself.** Whenever the live count drops below the target,
somebody new moves onto an empty plot and **raises their compound in real time**

- the cupboard goes down first to claim the ground, then floors, then the wall
  ring, one piece every few seconds (faster for better clans). Half the members
  arrive at the start and the rest move in once the walls are up. You can watch it
  happen, and you can interrupt it.

The HUD carries a live roster under the minimap: who is on the island, how good
they are, and who is still building.

You can raid them back. Their box is full of metal, scrap and gunpowder, and an
expert clan's box has C4 in it.

## Code layout

The tree is feature-first: one folder per feature, everything that feature needs inside it.
`docs/architecture/project-structure.md` is the full rule; this is the shape of it.

```
src/
  main.ts                 canvas sizing, the fixed-timestep loop, save wiring
  app/game.ts             orchestrator: owns the systems and wires their hooks
  features/
    building/  clans/  combat/  crafting/  items/  net/  npcs/
    render/  session/  survival/  time/  ui/  world/
  shared/
    core/                 input, camera, audio, particles
    design/constants/     the design tokens
    utils/  types/        maths helpers and cross-feature shapes
```

Inside a feature, artifacts are split one per file under `constants/`, `types/`, `enums/` and
`utils/`, with the system class at the folder root. Balance lives in the `constants/` folders, one
named number per file, each with the reasoning beside it.

Every import is absolute and rooted at `src/`.

`window.oxide` exposes the live game for debugging in the console.

## Conventions

`docs/` is the source of truth for how this codebase is built:

- [docs/README.md](docs/README.md): the index.
- [docs/architecture/](docs/architecture/): principles, source tree, file and code conventions.
- [docs/design-system/](docs/design-system/): design tokens and theming.
- [docs/testing.md](docs/testing.md): how anything gets verified.
- [docs/systems/](docs/systems/): how individual parts of the game work.

## Status

This is a first vertical slice: every system above is implemented and tested end
to end (gathering, crafting, the full building and upgrade chain, smelting,
cooking, monument radiation, clan raids that actually breach stone walls, the
wipe-and-resettle cycle, death and respawn, save/load). What is deliberately not in yet: multiplayer, ceilings
and multi-floor bases, structural-integrity collapse, electricity, and vehicles.
