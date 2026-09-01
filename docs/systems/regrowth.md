# Regrowth

**Everything harvestable comes back one full in-game day later.** One rule, no
exceptions, so you never have to remember which resource is on which timer.

`REGROWTH.seconds` is `DAY_SECONDS`, with `REGROWTH.spread` of jitter either way
so a field cleared in one sweep does not all pop back on the same tick.

## What it covers

| Resource                       | Set by                                       |
| ------------------------------ | -------------------------------------------- |
| Trees and ore nodes, harvested | `held-item.ts`, on the node running out      |
| Nodes cleared for a base       | `world.clearNaturalIn`                       |
| Nodes a clan clears            | `clans.ts`                                   |
| Loot crates                    | `world.update`, once looted                  |
| Wildlife                       | `npcs.kill`, via the `scheduleRegrowth` hook |

All of them call `regrowthSeconds()`. **Adding a resource means calling that,
not inventing a timer.**

## Wildlife is a resource; people are not

`NpcDef.wild` marks an animal. Animals regrow on this timer like a tree. Clan
survivors do not: a clan replaces its own people on `CLAN_RESPAWN_DELAY`, which
is a different mechanism for a different reason.

A regrown animal appears **somewhere else on the island**, not where it died.
The island keeps its population, but a stretch you hunted out stays quiet, which
is the point of having the timer at all.

## Building is banned on monument ground

Monuments are the looting locations: every crate on the island sits inside one.
`BuildSystem.monumentBlocked` is a hook set by the game, the same shape as
`naturalBlocked`, and both the foundation and the edge checks consult it.

The ban covers `monument.radius + MONUMENT_NO_BUILD_MARGIN`, so the approach is
as protected as the ground itself. Walling one in would turn a shared landmark
into somebody's private stash, which is the one thing it must never be.

## The one short timer

`REGROWTH.blockedRetrySeconds` (90s) is not a resource timer. It is how long to
wait before checking again when the spot is still built over, so a node under
somebody's base is not lost for good if the base ever comes down.
