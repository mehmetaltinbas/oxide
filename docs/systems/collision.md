# Standing against things

One routine decides what is solid, and everything that walks calls it. Read
this before adding anything to the island that a player should not be able to
walk through.

## One routine, for everybody

### Rule

`sim::keepOutOfSolids(world, build, x, y, radius)` is the only place that knows
what solid means. A player calls it, an animal calls it, and anything that
moves in future calls it. No caller writes its own check.

`sim::blocksMovement(node)` is the only place that decides whether a thing
standing on the island stops something. A new kind of solid is a new case
there, never a new test at a call site.

### Why

A bear walked straight through a barrel for a month. Not because anybody
thought a bear should: because the player had its own private copy of the
collision code and the wildlife had none, and nothing in the codebase connected
the two. The moment there are two copies of a rule, one of them is wrong and
nobody finds out until they watch it happen.

### How to apply

Move, then call it, then write the position back:

```cpp
double nx = npc.x + npc.vx * dt;
double ny = npc.y + npc.vy * dt;
keepOutOfSolids(world, build, nx, ny, def.radius);
npc.x = nx;
npc.y = ny;
```

In the wildlife this lives in one `step` lambda, so no branch of the state
machine can give an animal a heading and skip the walls on the way.

What it covers today: built pieces, resource nodes that block, and unlooted
crates. A node blocks unless it has been taken or it is a nettle, which is a
plant you walk through.

`play_check` walks a bear at a barrel and prints how far short of its middle it
stopped. If that number ever reaches zero, something has grown its own copy of
this again.

### Exceptions

Water is not a solid and is not handled here. It is a biome test, and animals
run the shoreline rather than being pushed out of it, which is a different
behaviour and lives with the movement code.
