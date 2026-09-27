# Testing

Critical operations only, driven headlessly against the real rules. Read this
before adding a check, and before claiming something works.

## A check drives the real game

### Rule

A check links `sim/` and calls the same functions the game calls. It builds a
real island, steps real systems, and asserts on real state. Nothing is mocked
and nothing is stubbed.

### Why

A check against a test double proves the double works. The rules here are
mostly relationships between numbers, and the only way to know a relationship
still holds is to run the thing that depends on it.

Every check in `tools/` exists because something broke and nobody noticed. They
are listed below with what each one caught.

### How to apply

```bash
cmake --build build
./build/bin/play_check     # the rules
./build/bin/island_check   # generation
./build/bin/sound_check    # every sound makes a noise
./build/bin/net_check      # two clients on one island (needs oxide_server running)
```

`play_check` prints a line per thing it drove, in plain English, and returns
non-zero if the island cannot be worked at all:

```
wildlife: Rabbit 130 Elk 85 Kangaroo 70 Wolf 80 Bear 26, 0 in the wrong country
chain: wolf went for the elk, caught it, gap 0 from 160
reload: shotgun 2 after a second and a half then 6, rifle 0 then 16
temper: wolf closed on you by 79, elk ran from you by 60
outrun: sprinting away from a wolf for ten seconds, gap 178 from 90
collide: a bear walking at a barrel stopped 32 short of its middle
```

### Exceptions

Drawing is checked by eye, through the flags below, because there is no useful
assertion about whether a rock looks like a rock.

## Assert both halves of a relationship

### Rule

When a number is pinned from two sides, the check asserts both. One of them
alone can be satisfied by a value that breaks the other.

### Why

The wolf's speed has to be under a sprint and over an elk. A check that only
proved you can escape a wolf would pass at any speed at all, including one at
which a wolf starves.

### How to apply

`play_check`'s `outrun` sprints a player away from a wolf and reports the gap;
`chain` puts a wolf on an elk and requires it to catch one. Both run every time.

## Look at it before saying it works

### Rule

Anything visual is screenshotted and looked at before it is reported as done.
The game takes a picture of itself and exits.

### Why

Twice now something was reported as finished when the edit had matched nothing
and changed nothing. A screenshot costs one command.

### How to apply

```bash
./build/bin/oxide 12345 --at 10368 10368 --shot /tmp/a.bmp
```

The seed pins the island, so the same command gives the same picture every
time. The flags that exist for looking at things:

| Flag | What it shows |
| --- | --- |
| `--shot <file>` | draw one frame to a BMP and exit |
| `--at <x> <y>` | start somewhere in particular |
| `--zoom <n>` | at a given view scale |
| `--window <w> <h>` | in a window of a given shape |
| `--zoo` | one of every animal in a row, held mid-blow |
| `--rocks` | one of every node kind in a row |
| `--icons` | every item picture on one sheet |
| `--arrow` | arrows and a rifle round frozen in the air |
| `--reload <t>` | a gun frozen part way through a magazine change |
| `--bleeding <s>` | that many seconds of a wound, for the cost beside the gauges |
| `--dragqueue <from> <over>` | a queued job carried to another row, for the drop preview |
| `--category <n>` | which crafting category the craft screen opens on |
| `--wear rad\|heavy\|metal\|hide` | something worn from the off |
| `--wheel` | the building plan's ring, held open |
| `--swing <t>`, `--draw <t>` | a blow or a bow frozen part way through |
| `--monument <n>` | dropped at the nth looting place |
| `--night`, `--panel`, `--craft`, `--title`, `--map` | the state you want to see |
| `--bench <frames>` | how long a frame takes to build, printed |

None of these touch the save.
