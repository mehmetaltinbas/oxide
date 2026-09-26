# Things lying on the ground

## Everything on the ground rots at the same rate

### Rule

Anything dropped on the island disappears thirty minutes later, whatever it is
and however it got there: thrown out of a pack, spilled from a barrel, left by
an animal, or dropped where somebody died. One constant, `sim::kDropLifetime`,
and no kind of drop is exempt.

### Why

Half an in-game day is long enough to come back for a kill you could not carry,
and short enough that a raid does not leave a field of loot lying about for
ever. A second rate for a second kind of drop would mean two rules to remember
and a list of exceptions that nobody can see from the code.

### How to apply

Drops age in `World::update`, which is the only place that touches
`Dropped::age`. Adding a new way to put something on the ground means calling
`World::dropStack`; it does not mean adding a lifetime.

The last few seconds are not signalled. If that changes, it changes for every
drop at once.
