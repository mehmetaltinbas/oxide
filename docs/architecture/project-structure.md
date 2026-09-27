# Project structure

**The shape of the tree is the architecture.** Where a file sits says what it
belongs to, and a feature you can delete by deleting one folder is a feature
that never grew roots into the rest of the game.

## The rule

`features/` is a **flat list of features**. One folder per feature, and that
folder owns everything the feature needs: its types, its constants, its
systems, its helpers, its drawing. `shared/` holds the handful of things no
single feature owns.

There is **no global `types/`, `utils/` or `systems/` dump** spanning features.
A folder called `types/` only ever exists *inside* a feature, or inside
`shared/`.

```
oxide/
	sim/                      the rules, with no SDL, no sockets, no drawing
		shared/               owned by no feature, needed by several
			constants/
			utils/
		features/
			world/            the island, its nodes, what lies on the ground
			monuments/        the landmarks and their loot
			items/            what a thing is, and the bags it sits in
			crafting/         recipes and the queue
			survival/         the body, its meters, how it moves
			combat/           swinging, shooting, blowing up
			wildlife/         the animals and the food chain
			building/         walls, doors, deployables
			net/              what goes over the wire
			session/          saving a world and loading it
	client/                   the game as you play it
		design/               the tokens everything draws with
			tokens/
			types/
		features/
			render/           the pen, the lettering, the ground, the night
			creatures/        drawing anything that walks
			items/            the thing in the hand, and the picture of it
			building/         drawing what is built, put down, or always there
			ui/               the interface over the world
			audio/
			net/
		app/                  the window, the input, the frame
	server/
		app/
	tools/                    one check, one program, one file
	assets/
	docs/
```

## Inside a feature

Per-kind sub-folders, and each holds one kind of thing:

| Folder       | What goes in it                                        |
|--------------|--------------------------------------------------------|
| `types/`     | one struct or enum per file, and nothing else           |
| `constants/` | a definition table, or one named number and its reason  |
| `systems/`   | a class that holds state and is stepped or drawn        |
| `utils/`     | free functions with no state behind them                |
| `draw/`      | client only: a routine that draws and mutates nothing   |

A feature uses only the sub-folders it needs. `sim/features/crafting/` is one
system and nothing else, and that is not a gap.

## Who may include whom

Dependencies point **one way**, and there are no cycles:

```
app  ->  features  ->  shared
                   ->  sim
```

- `sim/` includes nothing from `client/`, `server/` or `tools/`, ever.
- A feature may include another feature's headers, but never in both
  directions. `wildlife` knows about `world` because an animal lives in a
  biome; `world` knows nothing about animals.
- Anything two features need and **neither owns** moves down into `shared/`.
  Anything two features need that **one of them owns** stays where it is: the
  owner keeps it. `Biome` belongs to `world` and is read by `wildlife`; it does
  not move.

## Includes are absolute from the repository root

```cpp
#include "sim/features/wildlife/types/npc-def.struct.hpp"
#include "client/features/render/systems/paint.hpp"
```

Never `"../../types/npc-def.struct.hpp"`, never a bare `"paint.hpp"`. An
include line says where the file lives, so a reader can open it without
searching and a move shows up as a compile error rather than as the wrong file
being found. There is one include directory for the whole build: the root.

## No barrels

There is no `everything.hpp` that pulls a feature together, and no header whose
job is to include other headers. Each file includes exactly what it uses. A
barrel makes every file depend on every other file in the feature, and then
nobody can tell what would actually break.

## Adding a feature

Make the folder, put the sub-folders it needs inside it, add its `.cpp` files
to the `add_library`/`add_executable` list under the feature's comment. The
list in `CMakeLists.txt` is grouped by feature on purpose: it is the same map
as this page, and a file that is not in a feature group has nowhere to go.
