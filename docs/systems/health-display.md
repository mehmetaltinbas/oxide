# Health display

Anything on the island that can be hurt shows a bar for it, and every one of
them obeys the same rule about when that bar is on screen. The rule lives in
one place: `sim/shared/utils/health.util.hpp`. Nothing tests it by hand.

## The rule

A bar is drawn when the thing is alive, below full health, **and** was hurt
within `kHealthShownFor`.

After that the bar goes away. **The health does not change.** A tree chopped
to 65 stays at 65, it simply stops advertising it. Hit it again and the bar
comes back where it left off.

The clock runs wherever the player is. Standing next to something does not
keep its bar up, and walking away and back does not reset it: what the bar
reports on is the wound, and the wound is as old whether anybody is watching
it or not. A bar that reappears because you came close is worse than no bar,
because it looks like the thing healed and got hurt again.

Why it exists: without it, a bar sits over everything you have ever hit. An
afternoon's chopping leaves a forest of half-felled trees all shouting about
it, and a screen full of bars that describe your own past tells you nothing
about what is happening now.

## What a thing has to do to join in

Three things, and the compiler will not let you half-do them:

1. **Carry `sinceHurt` beside its `hp`.** A `double`, in the struct itself, not
   in the system that owns it.
2. **Call `tookDamage(thing)` wherever its health drops.** Every place, not the
   common one: a wall loses health to a swing, to a blast and to decay, and all
   three are wounds.
3. **Call `ageWound(thing, dt)` once per tick, for every one of them.** Not
   only the ones near the player, and not only the ones being simulated
   closely. Something that went to sleep wounded stops showing its health on
   the same clock as something in front of you.

The drawing then asks `showsHealth(thing)` and writes no condition of its own.
Something that keeps its full health on a definition rather than on itself, as
an animal does, asks the three-number form: `showsHealth(hp, maxHp, sinceHurt)`.

## Who does it now

| Thing | Struct | Damaged in | Aged in |
| --- | --- | --- | --- |
| Animals | `Npc` | `NpcSystem::hurt` | `NpcSystem::update` |
| Resource nodes | `ResourceNode` | `World::hurtNode` | `World::update` |
| Deployables | `Deployable` | `Explosives`, `BuildSystem` | `BuildSystem::updateDeployables` |
| Building pieces | `Structure` | `BuildSystem::hurt`, decay | `BuildSystem::update` |

Anything added to that list adds a row here.

## Why one routine and not four conditions

Because there were four conditions. The rule was added to animals and the
other three went on showing a bar for ever, and nobody finds that out by
reading the code: you find it out by chopping a tree and walking back past it
an hour later. It is the same failure as the collision one, for the same
reason: see [collision.md](collision.md).

## The number

`kHealthShownFor` lives in `sim/shared/constants/health-display.constant.hpp`.
It is **one minute** at the moment, deliberately short, so the behaviour can be
watched without waiting. It is meant to sit at **fifteen minutes**.
