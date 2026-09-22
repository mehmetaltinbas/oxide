# Project structure (feature-first)

How the source tree is organised at the top level: **one self-contained folder per feature** under
`src/features/`, plus `src/shared/` for what belongs to no single feature, and `src/app/` for the
thing that wires them together. For how a feature's own files are split by kind, see
[file-conventions.md](file-conventions.md).

## Feature-first, not type-first

### Rule

`src/features/` is a **flat list of features**, one folder per feature, each owning everything that
feature needs: its system class, its constants, its types, its enums, its utils. There is no global
`constants/` or `types/` dump spanning features.

```
src/
  main.ts                     the entry point: canvas, resize, and the frame loop
  app/
    game.ts                   the orchestrator, owns the systems and wires their hooks
  features/
      building/
      combat/
      crafting/
      items/
      net/
      npcs/
      render/
      session/
      survival/
      time/
      ui/
      world/
  shared/
    core/                     input, camera, audio, particles, owned by nobody, used by everyone
    design/constants/         the design tokens
    utils/                    one maths helper per file
    types/                    shapes two or more features both need
```

### Why

- **Everything for a feature is in one place.** A change to how crafting works touches
  `src/features/crafting/`, not three global directories.
- **Deleting a feature is deleting a folder.**
- It makes Separation of Concerns physical rather than aspirational.
- A contributor: or an agent, finds a feature by name in one hop.

### How to apply

1. New feature → create `src/features/<feature>/` and add its artifacts under the per-kind
   sub-folders described in [file-conventions.md](file-conventions.md).
2. Wire it into `src/app/game.ts`, which constructs it and supplies its hooks.
3. Anything a _second_ feature needs moves down into `src/shared/`.

## `src/shared/`, cross-feature only

### Rule

`src/shared/` holds only what is used by **two or more features and owned by none**: the input,
camera, audio and particle systems, the design tokens, the maths helpers, and the handful of types
that genuinely span features. Feature-specific code never lives here.

### Why

One home for reusable pieces keeps every feature consistent for free, and stops the same helper
being written twice with two slightly different rounding behaviours.

### Exceptions

`src/main.ts` sits at the root because it is the module the HTML loads; it does nothing but build
the canvas, construct the systems, and run the frame loop.

## `src/app/`, the orchestrator

### Rule

`src/app/game.ts` owns the systems, holds the state that is nobody's alone (the clock, the phase,
the player), and supplies every system's hooks. It is the only place that knows about all the
features at once.

### Why

Systems that call each other directly become a graph nothing can reason about. Routing every
cross-system call through hooks the orchestrator satisfies keeps the dependency arrows pointing one
way: a feature depends on its own hooks interface, and the orchestrator depends on the features.

### Exceptions

A feature may import another feature's **data**, its constants, its types, directly. That is not a
dependency on behaviour, and routing a constant through a hook would be ceremony for nothing.

## Systems, not one big orchestrator

### Rule

A feature that has behaviour owns a **system class** in its folder. The orchestrator constructs the
systems, holds the state that belongs to nobody in particular (the clock, the phase, the player),
runs them in order, and supplies their hooks. It does not implement their behaviour.

If `src/app/game.ts` is growing past a few hundred lines, that is the signal: some cluster of
methods in it belongs to a feature.

### Why

An orchestrator that also implements crafting, survival and building is a file nobody
can hold in their head, nothing can be tested in isolation, and every unrelated change touches. Both
games reached two and a half thousand lines this way before the split.

Once a cluster is a system, its dependencies are written down in one interface at the top of the
file, which is the readable summary of what it can do to the rest of the game.

### How to apply

1. Find the cluster: a set of methods that talk to each other far more than to anything else, and
   the state only they touch.
2. Move that state onto the new system. Counters and collections a feature owns go with it.
3. Rewrite the outward calls as `this.hooks.<call>()`, and collect them into `<System>Hooks`.
4. Construct it in the orchestrator, wiring the hooks with arrow functions that close over `this`.
5. Anything outside reaches it through the system, not through the orchestrator.

### Exceptions

**Phase transitions stay in the orchestrator.** Dying, respawning, starting a night, moving to the
next contract: these change what the whole game is doing, and that is exactly what the orchestrator
is for. A system reports that the player ran out of health; the orchestrator decides that means the
game is now in its death phase.

## Order of construction

### Rule

A system is constructed once, in the orchestrator, and any state it owns is reset by constructing a
fresh one. Never reset a system's state before it exists.

### Why

Both games had the same bug immediately after the split: `newWorld()` cleared a queue at the top and
built the system that owns it further down, so the clear ran against the previous instance and threw
on the very first world. The fix is to let construction be the reset.
