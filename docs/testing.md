# Testing

What gets verified, and how. This codebase has no unit-test suite and does not want one: it is a
simulation, and the things that break are emergent behaviours over hundreds of ticks, not the return
value of a pure function. Testing here means **driving the real game and asserting on real state**.

## Drive the real game, assert on real state

### Rule

Verify a change by running the actual game in a browser, stepping the real simulation, and reading
the real state afterwards. Do not assert on mocks, and do not assert on what the code looks like.

### Why

Every serious bug found in this project so far was emergent: a pathfinder that only failed when
another flood had already visited the ground outside, a roof that only stayed on when the cells were
scanned in a particular order, crew that starved because their larder never filled. None of them are
reachable from a unit test of the function that contained them.

### How to apply

The game exposes itself on `window` for exactly this purpose. A check is:

1. Set up the world state you need directly.
2. Step the simulation a known number of fixed steps, `for (let i = 0; i < n; i++) game.update(1 / 60);`
   , rather than waiting on wall-clock time.
3. Read the state and assert on it.

```ts
// build the situation, run it, look at what happened
const before = <state>;
for (let i = 0; i < 600; i++) game.update(1 / 60);   // ten seconds
const after = <state>;
```

### Exceptions

Anything genuinely pure and fiddly, a maths helper, a stack-merging rule, can be checked by calling
it directly. That is still done through the same console harness, not a test runner.

## Step the simulation yourself

### Rule

Never rely on the frame loop advancing while you measure. Call `update` in a loop.

### Why

A backgrounded tab throttles `requestAnimationFrame` to nothing, so a check that waits on real time
observes a world that never moved and reports a pass. Several apparent bugs during development were
this and nothing else.

## Check the harness before you blame the game

### Rule

When a check reports something impossible, verify the harness first.

### Why

Repeatedly, the fault was the test: the player parked outside the radius where NPCs simulate at all,
a movement key left held so every channel cancelled, a measurement taken across all bases when it
should have been per base. Correct the check, then re-run, then believe it.

## Watch the frame budget

### Rule

Any change to a system that runs every tick gets its cost measured before and after, against
docs/systems/performance.md.

### Why

The cost of a change is invisible until it is a stutter. Two regressions in this codebase were caught
this way and would have shipped otherwise: a pathfinder that went from 0.38ms to 7.2ms per update,
and an enclosure flood that cost 22ms every time anything was built.

### How to apply

```ts
const t = [];
for (let i = 0; i < 1800; i++) {
    const t0 = performance.now();
    game.update(1 / 60);
    t.push(performance.now() - t0);
}
t.sort((a, b) => a - b);
// report p50, p95, p99 and the worst, an average hides the spike that is the problem
```

## Finish with a smoke-test checklist

### Rule

At the end of a change, write down the handful of things to click or run to confirm it works end to
end: the critical path first, then the one or two edge cases most likely to break.

### Why

It is the shortest thing that catches an integration mistake, and it tells whoever reads the change
what "working" was taken to mean.

## Jumping to a state worth testing

Checking a change used to mean playing from the beach until the game reached the
situation the change was about, which for anything past the first few minutes is
a lot of gathering before you learn whether it even works.

Add `?scenario=<name>` to the dev URL:

```
http://localhost:5173/?scenario=base
```

Or switch without reloading, from the browser console:

```js
oxide.scenarios(); // list them
oxide.scenario('night');
```

They live in `src/features/dev/scenarios.ts`, one row each: a name, a line
saying what it sets up, and a function that puts the game in that state. Adding
one is a new row, not a new branch somewhere else.

**Verify a scenario when you add it.** A harness that quietly fails to reach the
state it advertises is worse than no harness: it turns a broken feature into a
passing check. The first `night` here was exactly that. It called `skipTime`,
which is creative-mode only and returns without doing anything outside sandbox,
so the scenario advertised midnight and handed back broad daylight. It sets the
clock directly now.

None of it ships. The whole block in `main.ts`, the console helpers included,
sits behind `import.meta.env.DEV`, which folds to false in a build and takes the
module with it. Exposing the helpers unconditionally would keep the entire table
in the bundle, so if you add to this, check `dist/` afterwards.
