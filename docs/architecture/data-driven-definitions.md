# Data-driven definitions

A new variant is a new row, not a new `switch` case. Read this before adding an
item, an animal, a node, a recipe or a building piece.

## One table per family

### Rule

Every family of things is a struct of fields and a table with one row per
variant, reached through an accessor. Code asks the definition what to do
rather than asking which variant it is.

```cpp
struct NpcDef {
	NpcKind kind;
	const char* name;
	int hp;
	double speed;
	/* ... */
	Biome homes[3];
	int homeCount;
	int rank;
	NpcKind eats[3];
	int eatsCount;
	bool skittish;
};

const NpcDef& npcDef(NpcKind kind);
```

### Why

A `switch` on a kind is the same list written again in every place that cares,
and every one of them has to be found and edited to add a variant. Miss one and
the new thing silently behaves like whatever the `default` was.

The tables are also the game's balance, readable in one place. The whole food
chain is three fields on one struct; you can see it by reading one file instead
of five state machines.

### How to apply

Adding an animal is one row in `sim/features/wildlife/constants/npc-defs.constant.cpp`, one value in `NpcKind`, and a
count in the populations table. Nothing else changes, because nothing else
switches on the kind: `populate` reads `homes`, the chase reads `eats` and
`rank`, the loot drop reads `loot`.

When behaviour cannot be expressed as a number, add a field for it rather than a
case at the call site:

```cpp
// No: the call site now knows about shotguns.
if (held == ItemId::PumpShotgun || held == ItemId::Waterpipe) { /* one shell */ }

// Yes: the gun says how it is fed.
if (gun.singly) { /* one shell */ }
```

### Exceptions

Drawing is the one place a `switch` on a kind is right. A wolf and a bear do not
differ by a number, they differ by being different drawings, and expressing that
as data would be inventing a worse programming language. `client/features/creatures/draw/animal.cpp`
switches on the kind to pick the body proportions and then draws from those.

A table field should still be preferred where it works: an animal's whole build
is a `Build` struct picked by that switch, so the drawing code below it is the
same for every animal.
