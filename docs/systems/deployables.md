# Deployables

A **deployable** is a thing you put down rather than build out of tiers: a
campfire, a furnace, a storage box, a sleeping bag, a tool cupboard, a
workbench. That is the proper name for them, and it is the name the code uses
throughout: `DeployKind`, `Deployable`, `DeployDef`.

They are not **structures**. A structure is a foundation, a wall, a doorway, a
door or a ceiling: built on the grid out of tiers, upgradeable, raidable. A
deployable sits on the ground inside that, takes a footprint of fine squares,
and is placed rather than constructed.

## One row per deployable

Everything one deployable is lives on one row of one table:
`kDefs` in `deploy-defs.constant.cpp`, read through `deployDef(kind)`.

| Field | What it is |
| --- | --- |
| `kind` | which one this is |
| `item` | the item in your hand that puts it down |
| `wide`, `deep` | its floor, in fine squares, unturned |
| `slots` | how much it holds, nought for the ones that hold nothing |
| `bench` | which tier of workbench it is, nought for everything else |

Nothing asks about a kind any other way. `deployFootprint`, `containerSlots`,
`benchTier`, `deployableOf` and `itemOf` are all one line each, reading the
table.

## Why it is one table

Because it was five. A deployable was described by a footprint in one file, a
slot count in a second, a bench tier in a third and the item it comes from in
two more, each a `switch` with a `default`. Adding one meant finding all five,
and **a miss fell through to the default rather than failing**: a new box with
no footprint was silently two squares, and a new bench with no tier was
silently not a bench at all.

Two asserts hold the table to the enum:

```cpp
static_assert(rows == kDeployKindCount, "every deployable needs a row, and only one");
static_assert(inOrder(),                "the table must be in the order of DeployKind");
```

So a missing row is a build error, and a row in the wrong place is a build
error. There is no way to half-add a deployable.

## Adding one

1. A value on `DeployKind`, and bump `kDeployKindCount`.
2. An item on `ItemId`, with a row in the item table and a recipe.
3. A row in `kDefs`, in the same position as the enum value.
4. Art in `client/features/building/draw/deployable.cpp`, drawn inside `hw`
   and `hd`, the half-extents of its own box, so it fills the squares it was
   put down on.

That is all. The footprint, the placement, the collision, the ghost, the
container, the bench tier and the item it drops as all follow from the row.

## The grid they sit on

See [deploy-grid.md](deploy-grid.md) for the fine squares, turning and
snapping.
