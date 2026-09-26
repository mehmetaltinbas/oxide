# Project rules for Claude Code

Oxide: a top-down survival island in C++ with SDL3 and no engine. One rules
library, three binaries, no other language in the repository.

## Source of truth

The conventions in `docs/` are the source of truth for how this codebase is
built. **The rules below are a condensed checklist. The full rationale, examples
and exceptions live in those docs.** Before changing structure, or whenever a
rule here is unclear, open the relevant doc and match it rather than inventing a
new pattern.

Entry points:

- [docs/README.md](docs/README.md): the index, the single-source-of-truth table, the placeholder legend, and the document skeleton.
- [docs/architecture/](docs/architecture/): principles, the source tree, file and code conventions.
- [docs/design-system/](docs/design-system/): design tokens and theming.
- [docs/systems/](docs/systems/): how each part of the game works and the rules it holds.
- [docs/testing.md](docs/testing.md): how anything gets verified.

## Plan first, then align with the docs

For anything non-trivial, work in plan mode. After producing a plan, **re-read
the relevant docs and revise the plan to match them** before writing code. Say
which doc each step satisfies. Don't start editing until the plan matches the
documented conventions.

## Never make consequential decisions alone

Implementation details, names, where a helper lives, how to wire a loop, are
yours to decide. A **consequential decision**, anything shaping the
architecture, the data model, scope, or how the game plays, is mine to make.

When you hit one: stop, list the options with their trade-offs, and ask. Don't
pick a default and continue. If you're unsure whether a decision is
consequential, treat it as if it is and ask.

## Measure, don't guess

Balance numbers and performance claims come from measurements taken in the
running game, and the measurement goes in a comment beside the value it
justifies. Before tuning a number, measure the thing it controls. Before
claiming a change is fast, measure it against the frame budget.

Where a number is pinned by a relationship rather than by taste, add a check
that asserts the relationship, and assert **both** halves of it.

## Look at it before saying it works

Anything visual is screenshotted and looked at before it is reported as done.
`./build/bin/oxide 12345 --at 10368 10368 --shot /tmp/a.bmp` costs one command,
and there are flags for every state worth looking at: see
[docs/testing.md](docs/testing.md). An edit that matched nothing is silent.

## Conventions to honour

- **One rules library.** `sim/` knows nothing about SDL, sockets or drawing.
  `client/`, `server/` and `tools/` include it and never each other. If two of
  them need the same thing, it moves into `sim/`.
- **One artifact per file**, named after that artifact in `snake_case`. No
  headers that hold two unrelated things.
- **Simulation never draws; drawing never mutates.** Step the rules, then draw
  what came out. A drawing routine takes `const&` and returns nothing.
- **A new variant is a new row**, not a new `switch` case. Add a field to the
  definition struct rather than special-casing at the call site.
- **No hardcoded tuning values.** Every balance number is a named constant or a
  field on a definition, and carries the reason it is that number.
- **No raw visual values.** Colour, ink weight, spacing and radius come from
  `client/src/palette.hpp` and `client/src/ui.hpp`.
- **One routine per rule.** When two copies of a rule exist, one of them is
  wrong and nobody finds out until they watch it happen. Collision is the
  standing example: see [docs/systems/collision.md](docs/systems/collision.md).
- **Widths are in world units.** Ink, bars and name tags scale with the view;
  the interface does not. See [docs/systems/world-scale.md](docs/systems/world-scale.md).
- **No prose during play.** No notices, toasts, banners or sentences on screen
  for refusals, warnings or confirmations. Say it with the thing itself or with
  a sound. See [docs/systems/screen-text.md](docs/systems/screen-text.md).
- **Tabs for indentation, four columns wide.** A block always indents its
  contents; a body flush against its brace is not acceptable. A namespace body
  is not indented. `.editorconfig` and `.clang-format` set it. See
  [docs/architecture/indentation.md](docs/architecture/indentation.md).

## Finish with a smoke-test checklist

At the end of an implementation, give a short, high-level smoke-test checklist,
the handful of things to click or run to confirm it works end to end. Critical
path first, then the one or two edge cases most likely to break.

## Writing style

Report results in plain, brief English: what works now, what it means in
practice, and anything left to do. No em-dashes in anything written for me.
