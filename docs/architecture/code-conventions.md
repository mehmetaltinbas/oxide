# Code conventions

Cross-cutting rules for how code is shaped inside `src/`. These apply to every feature.

## Imports are absolute, rooted at `src/`

### Rule

Always import via a path rooted at `src/`. Never use `./` or `../`.

### Why

- Absolute paths survive file moves: a system can be relocated without rewriting the imports of
  every neighbour.
- The path documents which feature a symbol comes from at a glance: `src/features/building/…` is
  self-evident in a way `../../constants` is not.
- Search and refactor tools resolve one canonical path per module instead of a family of
  relative-depth variants.

### How to apply

```ts
// good
import { CELL } from 'src/features/building/constants/cell.constant';
import { dist } from 'src/shared/utils/dist.util';

// bad
import { CELL } from './constants/cell.constant';
import { dist } from '../../shared/utils/dist.util';
```

`tsconfig.json` maps `src/*` through `paths`, and `vite.config.ts` carries the matching alias so the
bundler resolves it the same way. Both must be updated together.

## No barrel files

### Rule

Nothing re-exports anything. Import from the file that declares the symbol.

### Why

A barrel makes every consumer depend on every symbol in it, so touching one constant invalidates
modules that never used it. It also hides where a symbol actually lives, which is the one thing an
import path is for.

## A system owns its state and exposes it as data

### Rule

Each feature's system class owns its own collections and the invariants over them. Other code reads
what it exposes and calls its methods to change it; nothing reaches in and mutates another system's
arrays.

### Why

When two places can mutate the same array, the invariant that array is supposed to hold has no home,
and the bug shows up somewhere neither of them.

## Cross-system calls go through hooks

### Rule

A system that needs to cause an effect outside itself declares a `<System>Hooks` interface listing
exactly the calls it makes, and takes it in its constructor. It never imports the orchestrator, and
never holds a reference back to `Game`.

### Why

- The interface is a readable, reviewable list of everything that system can do to the rest of the
  game.
- It keeps the dependency arrows one-way, so a feature can be read: and tested, without the whole
  application.

### How to apply

```ts
export interface <System>Hooks {
  /** One line saying when this fires and what it is expected to do. */
  <onSomething>(...): void;
}

export class <System> {
  constructor(private world: World, private hooks: <System>Hooks) {}
}
```

The orchestrator supplies the implementations at construction time.

## Simulation and rendering stay separate

### Rule

`update(dt)` mutates; `draw()` does not. The world renderer takes a `GameView`, which is a read-only
view of the game, so the compiler refuses any assignment to a top-level field during a draw call.

### Why

The simulation runs at a fixed 60Hz while rendering runs at whatever the display offers, so a frame
may run zero simulation steps or several. Anything that mutates during drawing would advance at the
frame rate, which means the same fight resolves differently on different hardware.

Passing `Game` itself made this a convention nobody could check. `GameView` makes it a compile
error. It is a shallow readonly, so it does not stop someone reaching into a nested object, but it
does stop the mistake that actually happens.

### Exceptions

- **Purely visual state** that no rule depends on, a cached canvas or an animation phase, may be
  updated during drawing. The renderer keeps these as its own fields, not on the game.
- **The HUD is immediate-mode**, so it takes the mutable `Game`. Hit-testing a button and drawing it
  are the same pass: the rectangle is computed, drawn, and tested against the cursor in one place,
  which is precisely what makes the code readable. It therefore handles clicks while it draws, and
  writes things like `panel`, `dragging` and `uiHover`. That is the pattern working as intended, not
  a leak, but it is the reason the HUD cannot take a read-only view.

## Who may hold the orchestrator

### Rule

Simulation systems never reference `Game`. They declare a `<System>Hooks` interface and receive it.
Three things are allowed to know the whole game, and only these three:

| Holder       | What it takes | Why                                                                         |
| ------------ | ------------- | --------------------------------------------------------------------------- |
| the renderer | `GameView`    | it reads everything and writes nothing                                      |
| the HUD      | `Game`        | immediate-mode drawing handles input in the same pass                       |
| save/load    | `Game`        | serialising the world means reading all of it, and loading means writing it |

### Why

The rule exists to stop simulation systems reaching sideways into each other through the
orchestrator, which is what turns a dependency graph into a knot. It was never meant to stop a
serialiser from serialising. Stating the three exceptions precisely is more useful than a rule
everyone can see is broken.

## Input is polled once per frame

### Rule

Read edge-triggered input (`justPressed`, `mouseClicked`) once per rendered frame, before the
simulation steps run, not inside `update()`.

### Why

Plenty of frames run zero simulation steps. A key pressed on one of those was cleared at the end of
the frame having never been read, which is why single-press actions used to work only sometimes.

## Indentation and formatting

### Rule

Four spaces, never tabs. Formatting is not a matter of taste here: Prettier owns it, the settings
live in `.prettierrc.json`, and `.editorconfig` tells editors the same thing so most of them get it
right with no plugin.

```bash
npm run format        # fix everything
npm run check         # formatting + types, no build
```

### Why

- A repo that mixes indentation produces diffs full of whitespace, which hides the change that
  actually matters.
- Making it a tool's job rather than a reviewer's job means it never comes up in review at all.

### How to apply

`npm run build` runs `prettier --check` before it type-checks, so badly formatted code cannot be
built, let alone shipped. If the build complains about formatting, run `npm run format`.

Editors pick the width up from `.editorconfig` automatically. Anyone who opens the project gets four
spaces whether or not they have Prettier installed, and the build catches them if their editor
ignored it.
