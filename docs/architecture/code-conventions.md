# Code conventions

How a file is put together, and the two rules about who may touch what. Read
this before wiring one part of the game to another.

## The simulation never draws; drawing never mutates

### Rule

Nothing in `sim/` draws, plays a sound, or reads the keyboard. Nothing in the
drawing path changes the state of the game.

A frame is two halves and they do not overlap: step the rules, then draw what
came out.

### Why

A rule that only runs when somebody is looking is not a rule. The server draws
nothing and has to reach the same answers as the client, which is only possible
if drawing is a pure read.

The reverse matters as much. A draw call that quietly advances an animation
means the game plays differently at a different frame rate, and it means the
same frame drawn twice is not the same frame.

### How to apply

```cpp
// The step: everything that changes is in here.
sim::stepPlayer(world, build, player, input, dt);
const sim::NpcEvents animals = npcs.update(world, build, projectiles, dt, player);

// The draw: reads, and nothing else.
sprites.draw(*node, sx, sy, scale, snowy, broadleaf, alpha);
```

Drawing routines take what they need by `const&` and return nothing. If a
drawing routine needs to remember something between frames, that something is
about drawing, so it belongs to the drawing object and not to the game: the
sprite cache and the text cache are both allowed, because neither changes what
happens.

### Exceptions

The client keeps a small amount of state that is only about presentation: which
fist the next punch comes off, the camera's eased position, how far the screen
is shaking. None of it is read by `sim/`.

## Systems own their state and are stepped in order

### Rule

Each system in `sim/` owns its own data and exposes it through methods. Nothing
holds a pointer back to whoever owns it. The client's loop constructs them,
holds the shared state, and runs them in a fixed order.

### Why

A system that reaches back into its owner can be called from anywhere, which
means it can be called at the wrong time, which means the order a frame runs in
stops being visible in one place.

### How to apply

A system takes what it needs as arguments for that call:

```cpp
NpcEvents NpcSystem::update(World& world, const BuildSystem& build,
							Projectiles& projectiles, double dt, const Player& player);
```

What a system did, the caller learns from what it returns, not from a callback:
`NpcEvents` says what hit the player this tick and what died, and the client
turns that into a sound and a number floating off a body.

### Exceptions

Save and load touch several systems at once by nature. They live in
`sim/src/save.cpp` and take each one explicitly rather than holding anything.

## Headers include what they use

### Rule

A header includes what its own declarations need and nothing more. A source
file includes its own header first, then what it uses.

### Why

A header that leans on its includer's includes compiles until the day somebody
includes it somewhere new.

### How to apply

Forward declare a type you only hold a reference or pointer to. `sim/npcs.hpp`
declares `class Projectiles;` rather than including it: the rounds in the air
know about the animals, and including it both ways is a cycle.

## A blow is a result, not a side effect

### Rule

Something that happens in the rules and needs to be seen or heard returns a
result describing it. The client decides what that looks like.

### Why

It keeps `sim/` free of presentation, and it keeps the client's reaction in one
readable place instead of scattered through the rules as callbacks.

### How to apply

`swing()` returns a `SwingResult`: whether it landed, on what, how big the thing
was, what came off it. The client reads that and produces the number that floats
off the top of the tree, the specks, and the sound the material makes. `sim/`
never knew any of that happened.
