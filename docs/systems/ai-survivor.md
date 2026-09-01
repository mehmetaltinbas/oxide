# Survivor AI

**This document is the contract for how clan NPCs behave. Any change to their
behaviour should be reflected here first, and any new behaviour should be added
as a goal in the table below rather than bolted onto an existing one.**

## The principle

A clan NPC is not a spawner output or a patrol route. It is modelled as _another
survivor playing the same game as you_, with the same needs and the same tools.
It gets hungry, it gets tired, it chops trees, it hunts, it cooks what it kills,
it feeds ore into a furnace and waits for fragments, it spends what the clan has
banked on upgrading walls, and it sleeps when it is dark. It uses the real
containers, the real deployables and the real building system, nothing is
special-cased for AI.

The practical test: **if you watch a clan member for two minutes without knowing
it is an NPC, its behaviour should be indistinguishable from a player going about
their business.**

## Needs

Every survivor carries three continuous values, all 0-100:

| Need      | Rises                       | Falls                   | Consequence                         |
| --------- | --------------------------- | ----------------------- | ----------------------------------- |
| `hunger`  | constantly                  | eating cooked meat      | above 85 they take slow damage      |
| `fatigue` | constantly, faster at night | resting inside the base | above 70 they move and work slower  |
| `danger`  | derived, not stored         | ,                       | overrides everything below `defend` |

Needs feed the utility scores below. They do not directly drive behaviour, which
is what stops the AI looking like a state machine with a hunger timer.

## Goals

Each survivor re-scores every goal on a fixed replan interval (and immediately
when its current goal completes or becomes impossible). The highest score wins,
and is then **committed to** for a minimum duration so they do not dither.

| Goal     | Scored on                                             | What they actually do                                                          |
| -------- | ----------------------------------------------------- | ------------------------------------------------------------------------------ |
| `defend` | hostile within 320 of the base                        | Break off, engage, return when clear                                           |
| `raid`   | the clan has ordered a raid                           | Travel to the player's base, breach it                                         |
| `eat`    | `hunger`, and cooked food being reachable             | Take cooked meat from carry or the clan box, eat it                            |
| `cook`   | holding raw meat, a campfire exists                   | Walk to the campfire, load meat and wood, light it, wait, take the cooked meat |
| `rest`   | night, and `fatigue`                                  | Go inside the compound, idle, recover                                          |
| `hunt`   | the clan's food stock is low                          | Find an animal, kill it, carry the meat home                                   |
| `smelt`  | ore in the clan box, a furnace exists                 | Load ore and wood into the furnace, light it, collect fragments later          |
| `build`  | the clan can afford an upgrade                        | Walk to the weakest piece and take it up a tier                                |
| `loot`   | veteran/expert clans, monument in range, off cooldown | Travel to a crate, empty it, carry it home                                     |
| `expand` | the clan can afford another piece of compound         | Walk to open ground beside the base and wall it in                             |
| `gather` | always available, weighted by scarcity                | Chop/mine the resource the clan is shortest on, haul it to the box             |

`gather` is the floor: a survivor with nothing pressing to do always finds
something worth carrying home. Scarcity weighting means a clan short on stone
goes and mines stone rather than cutting more wood it does not need.

## The clan stockpile

The clan's wooden box **is** its stockpile. Survivors deposit what they gather
into it and draw from it to build, smelt and eat. This matters for two reasons:

1. You can see it. Break into a clan base and the box contents tell you what
   they have been doing.
2. Raiding it actually hurts them. Emptying a clan's box sets their building and
   smelting back, because there is nothing left to spend.

## Distance handling

Survivors outside `NPC_ACTIVE_RADIUS` are frozen, on an island this size, most
of the population is nowhere near the player, and simulating all of it every
frame is wasted work.

To stop distant clans from being static, they get a **coarse offscreen tick**
instead: their box slowly accrues resources and they occasionally upgrade a
piece, at a rate matched to their skill. A clan you leave alone for ten minutes
should be visibly better off when you come back, whether or not you watched.

## Skill

A clan's skill (`amateur` → `expert`) shifts the AI as well as the stats:
better clans replan faster, hold more in reserve before spending, hunt and loot
more readily, and are the only ones that travel to monuments.

## Hostility

Clan members are not neutral scenery. They become **alerted** when:

- anything hits them, or hits a clanmate within `alertRadius`
- the player comes within ~340 of their compound

An alerted survivor drops what it was doing, takes the `defend` goal, and
engages. If it loses sight of the target it closes on the last known position
rather than immediately forgetting. Alert decays after `alertSeconds`.

### Breaking off

They are people, not wolves, so they do not trail you across the island. A
clansman **disengages** when any of these is true:

- their health drops below `CLAN_RETREAT_HEALTH`
- they have chased for longer than `CLAN_PURSUIT_SECONDS`
- they are further than `CLAN_PURSUIT_LEASH` from their post

On breaking off they set a `disengage` timer, head home, and, importantly -
that timer **suppresses both the trespass alarm and the `defend` goal**, so
nothing talks them straight back into a fight they were losing. A gunman keeps
firing over his shoulder while backing away unless he is badly hurt. Taking
another hit clears the timer immediately: cornered, they fight.

## Death and respawning

A clan compound includes a **sleeping bag**, and a clan that is short-handed
respawns a member on it every `CLAN_RESPAWN_DELAY` seconds up to its skill's
headcount. Killing clansmen is therefore attrition, not elimination, the bag
is what makes it permanent, which is exactly why it is worth breaking.

## Movement and navigation

Survivors path with **A\* over the build grid** (`navigation.ts`). A step
between two cells is legal only when the edge between them is not a wall or a
shut door, so a route through a base genuinely goes through its doorway.

This replaced a local steering probe, which is the single thing that made them
look stupid: no whisker probe can see that the way to a tree on the far side of
your own wall is round through the gate, so they pressed into the wall forever
and starved doing it.

Three things keep it affordable, it was measured at **7.2ms per update** before
any of them, against a whole-frame budget of about 4ms:

1. **Cell walkability is cached** in a `Uint8Array` and invalidated when
   building or resource nodes change. Testing it properly costs a spatial query
   per cell, which cannot be paid on every A\* expansion.
2. **The frontier has a membership `Set`.** `open.includes()` inside the loop
   made the search quadratic.
3. **A whole-frame search budget** (`SEARCHES_PER_FRAME`) plus staggered
   re-path timers, so a crowd never plans in lockstep. Agents that miss the
   budget keep their old route for a frame.

Short unobstructed hops skip pathfinding entirely and just walk.

On top of that every survivor still runs a **stuck watchdog**: if it is pursuing
a goal and has not moved for `stuckTimeout` seconds, it abandons the goal and
re-plans.

## Why not machine learning

Because the failures were never failures of _intelligence_. Every time these
agents looked stupid it turned out to be a plain bug in the plumbing:

- they walked into walls: no pathfinder
- they never looted: the `loot` goal was scored but had no implementation and
  fell through to `rest`
- they starved: food was kept in pockets, so the clan larder never filled and
  hunger scoring hunted forever
- they had no stockpile: an inverted `canDeploy` check meant clan boxes were
  never placed

A learned policy would have papered over all four while making them impossible
to diagnose. Utility scoring over hand-written behaviours is the right shape for
this: it is inspectable, it is cheap, and when an agent does something daft you
can read the score table and see exactly why.

If they ever need to be _smarter_ rather than _less broken_, the next steps are
still not ML, they are more goals, better scoring inputs, and giving them
memory (where they last saw a player, which monument they already stripped).

## Invariants

- Survivors never teleport, never spawn items from nothing, and never use a
  recipe or building rule the player cannot.
- Everything they gather passes through a real container.
- A survivor that cannot reach its goal abandons it rather than pathing into a
  wall forever.
