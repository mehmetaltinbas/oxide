# Putting things down

Furniture, fires, benches and boxes are **deployables**: things you place on
the ground rather than build out of tiers. They sit on a grid of their own,
finer than the building grid, and each one takes the floor its own footprint
says.

## The grid

A building cell is `kBuildCell` across. It is divided into **twelve by twelve**
fine squares for placing things: `kDeployGrid`, and `kDeployCell` for one of
them in world units.

Twelve, because it divides by two, three, four and six, which is every
footprint on the island. Nothing ever lands on half a square.

## Footprints

`deployFootprint(kind)` is the table, in fine squares, measured the way the
thing is drawn: `wide` runs across the screen, `deep` runs down it.

| Thing | Footprint |
| --- | --- |
| Campfire | 2 x 2 |
| Small Storage Box | 2 x 2 |
| Large Storage Box | 4 x 2 |
| Sleeping Bag | 4 x 2 |
| Tool Cupboard | 3 x 2 |
| Furnace | 3 x 3 |
| Workbench I | 2 x 1 |
| Workbench II and III | 3 x 2 |

A new deployable is a new row in that table and nothing else. Nowhere asks
about a kind's size except through it.

## Turning

Right click before you put something down and it goes in turned a quarter.
`deployFootprint(kind, turned)` swaps `wide` and `deep`, and that is the whole
of what rotation is. One flag, not an angle: a thing on a square grid faces
two ways that matter, and a free angle would mean a free footprint too.

## Placing

`snapDeploy` puts the middle of the thing on the nearest legal spot. A thing
an even number of squares across has its middle on a grid line; an odd one has
its middle in the middle of a square. Snapping both the same way put odd
things half a square off their own floor.

`refuseDeploy` then answers whether it may go there. It checks the box against
every other deployable's box, so two small boxes share a building cell happily
and a furnace may not sit on a campfire. It used to ask whether the **cell**
was taken, which made everything the same size.

## Adding a deployable

1. A row in `deployFootprint`.
2. A row in `containerSlots`, `benchTier`, `deployableOf` and `itemOf` as it
   needs them.
3. Art in `client/features/building/draw/deployable.cpp`, drawn inside `r`,
   the short half of its own box, so nothing spills out of the squares it was
   put down on.

The `footprint` check in `tools/play_check.cpp` asserts the size, the turn and
the overlap rule.
