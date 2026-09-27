# Dropped items

A stack on the ground is the game's one way of handing you something it cannot
put straight in your pack: loot off an animal, what a broken box spills, what
you threw away, what a cancelled craft gives back. There are a dozen places
that drop something and there will be more.

## The rule

**Nothing lands inside something solid.**

A stack inside a wall or under a boulder is a stack nobody can pick up. It is
still there, it still counts against the world, and the only sign of it is a
player walking in circles trying to reach it.

## How the rule is enforced

**By `World::dropStack`, once, for every drop there will ever be.**

Not by the callers. The callers that get it right are not the ones that
matter: the one that forgets is the one you find out about. Every drop goes
through `dropStack`, so `dropStack` is where the nudge belongs.

The world knows its own trees and rocks but not what has been built on it, so
it is told:

```cpp
sim::settleDropsAgainst(world, build);   // once, at startup
```

After that the world settles every drop against the one collision routine,
`keepOutOfSolids`, by `kDropClearance`. A new kind of solid is picked up for
free, because `blocksMovement` is the only place that says what solid means.
A new way of dropping something is picked up for free, because it goes through
`dropStack` like everything else.

## Adding a way to drop something

Call `world.dropStack(stack, x, y)`. That is all. Do not nudge the position
yourself, and do not push items into `drops_` by another route.

## Adding a new binary or test harness

Call `sim::settleDropsAgainst(world, build)` next to where the world and the
build system are made. The client does it at startup, the server does it in
`Room`'s constructor. Without it, drops land wherever they were asked to, which
is the old behaviour rather than a crash: the `settle` check in
`tools/play_check.cpp` is what catches a missing wire.

## How long they last

`kDropLifetime`, thirty minutes: see [ground-items.md](ground-items.md).

## See also

- [collision.md](collision.md): the one routine that says what is solid.
