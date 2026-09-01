# Project rules for Claude Code

Oxide: a top-down survival island, on a plain HTML5 canvas with TypeScript and no engine.

## Source of truth

The conventions in `docs/` are the source of truth for how this codebase is built. **The rules below are a condensed checklist. The full rationale, examples and exceptions live in
those docs.** Before
changing structure, or whenever a rule here is unclear, open the relevant doc and match it rather
than inventing a new pattern.

Entry points:

- [docs/README.md](docs/README.md): the index, the placeholder legend, and the document skeleton.
- [docs/architecture/](docs/architecture/): principles, source tree, file and code conventions.
- [docs/design-system/](docs/design-system/): design tokens and theming.
- [docs/testing.md](docs/testing.md): how anything gets verified.

## Plan first, then align with the docs

For anything non-trivial, work in plan mode. After producing a plan, **re-read the relevant docs and
revise the plan to match them** before writing code. Say which doc each step satisfies. Don't start
editing until the plan matches the documented conventions.

## Never make consequential decisions alone

Implementation details, names, where a helper lives, how to wire a loop, are yours to decide. A
**consequential decision**, anything shaping the architecture, the data model, scope, or how the
game plays, is mine to make.

When you hit one: stop, list the options with their trade-offs, and ask. Don't pick a default and
continue. If you're unsure whether a decision is consequential, treat it as if it is and ask.

## Measure, don't guess

Balance numbers and performance claims come from measurements taken in the running game, and the
measurement goes in a comment beside the value it justifies. Before tuning a number, measure the
thing it controls. Before claiming a change is fast, measure it against the frame budget.

When something behaves impossibly, check the test harness before blaming the game, see
[docs/testing.md](docs/testing.md).

## Conventions to honour

- **Feature-first**: one folder per feature under `src/features/`; `src/shared/` for what belongs to
  nobody; `src/app/game.ts` for the orchestrator.
- **One artifact per file**: one constant, one type, one interface, one util, one class. No barrels.
- **Imports are absolute**, rooted at `src/`. Never `./` or `../`.
- **No hardcoded tuning values.** Every balance number lives in a `*.constant.ts` in the feature that
  owns it.
- **No raw visual values.** Colour, spacing, radius, draw order and motion come from the design
  tokens in `src/shared/design/constants/`.
- **A new variant is a new row**, not a new `switch` case. Add a field to the definition interface
  rather than special-casing at the call site.
- **Simulation never draws; drawing never mutates.** `update(dt)` owns state, `draw()` reads it.
  The renderer takes `GameView` so the compiler enforces it.
- **Cross-system calls go through hooks.** No simulation system holds a reference to `Game`. The
  only three that may are the renderer (read-only `GameView`), the HUD (immediate-mode) and
  save/load, and the reasons are in `docs/architecture/code-conventions.md`.
- **Behaviour lives in feature systems**, not in the orchestrator. `src/app/game.ts` constructs
  systems, owns shared state and phase, and runs them in order. Nothing else.
- **Four-space indentation**, enforced by Prettier. `npm run build` fails on a formatting drift, so
  run `npm run format` before you finish.

## Finish with a smoke-test checklist

At the end of an implementation, give a short, high-level smoke-test checklist, the handful of
things to click or run to confirm it works end to end. Critical path first, then the one or two edge
cases most likely to break.

## Writing style

Report results in plain, brief English: what works now, what it means in practice, and anything left
to do. No em-dashes in anything written for me.
