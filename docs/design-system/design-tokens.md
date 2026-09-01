# Design tokens

Design tokens are the **single source of truth for every visual value**: colour, spacing, radius,
draw order, motion. Drawing code names a token; it never writes a raw value. This is what lets the
whole game restyle from one folder instead of a thousand scattered literals.

Tokens live in `src/shared/design/constants/`, one set per file.

| Category         | Token set | Example                                         |
| ---------------- | --------- | ----------------------------------------------- |
| Interface colour | `UI`      | `UI.surface`, `UI.ink`, `UI.accent`             |
| World colour     | `WORLD`   | `WORLD.shadow`, `WORLD.barTrack`, `WORLD.vital` |
| Spacing          | `SPACE`   | `SPACE.xs` … `SPACE.xl`                         |
| Radius           | `RADIUS`  | `RADIUS.sm`, `RADIUS.md`, `RADIUS.lg`           |
| Draw order       | `LAYER`   | `LAYER.actor`, `LAYER.roof`, `LAYER.hudPanel`   |
| Motion           | `MOTION`  | `MOTION.fast`, `MOTION.normal`                  |

Each set is a **small named scale**. Pick from the scale; don't invent an off-scale one-off.

## Core rule

### Rule

Never write a raw visual value in drawing code, no `'#9B1B30'`, no `13`, no `0.35` fade. Name the
token. If the value you need has no token, **add a token**; don't inline the literal.

### Why

- **One change updates everywhere.** Adjusting `UI.surface` reflows every panel; a hardcoded literal
  does not get the change.
- **Consistency for free.** A fixed palette prevents the drift of fifteen almost-identical greys and
  six almost-equal paddings.
- **Theming becomes possible at all.** Because drawing code references tokens, swapping the values
  swaps the look without touching a single draw call. See [theming.md](theming.md).

### How to apply

1. Add the entry to the matching set in `src/shared/design/constants/`, with a comment saying what
   it is for.
2. Import the set and name the token at the draw site.
3. Never keep a second copy of a token under a local alias, a local `const L = { bg: UI.surface }`
   is a second definition of the same thing, and the two drift.

## Two colour contexts

### Rule

`UI` is for interface chrome: panels, slots, buttons, labels, scrims. `WORLD` is for the world layer:
the shadow under an actor, the plate behind a health bar, the red that always means damage. Take
from the set that matches what you are drawing.

### Why

The two have different jobs. Chrome must stay legible over anything the world does; world colour
must sit inside the scene. Keeping them apart means re-theming the interface doesn't wash out the
forest, and vice versa.

## Draw order

### Rule

Stacking order is a **single ordered set of named layers** in `LAYER`. A canvas has no z-index -
whatever is painted last wins, so the ordering really lives in the sequence of draw calls. `LAYER`
is the written-down version of that sequence, and the renderer's call order must match it.

### Why

- The order becomes reviewable in one place instead of being inferred by reading the whole renderer
  top to bottom.
- A new layer gets a deliberate slot between named neighbours, rather than being dropped wherever it
  happened to look right.

### How to apply

Reach for the nearest existing layer first. If you must add one, give it a name and a value _between_
the neighbours it belongs between, and say what it is for.

## Palette

**The palette should read as saturated and alive, not washed out. Any new
colour must sit alongside the existing ones without looking faded.**

The world started too desaturated: greens near grey, sand near beige, water near
slate. Everything drawn on top then had nothing to sit against, so the whole
screen read flat.

Rules for anything new:

- **Ground and foliage carry the picture.** They cover the most pixels, so they
  set how alive the game looks. Keep them clearly hued, a green should read as
  green, not as grey-green.
- **Lift saturation before lightness.** Brightening a washed-out colour makes it
  look chalky. Push saturation first, then nudge lightness.
- **Keep hue identity.** When restating a colour, move its saturation and
  lightness, not its hue. A metal ore that drifts from tan to orange stops being
  recognisable.
- **Materials stay distinguishable at icon size.** Wood, stone, metal and sulfur
  must be tellable apart in a 20px glyph, which needs real hue separation, not
  four variations of grey-brown.
- **Interface colour is separate.** UI tokens live in their own palette and are
  deliberately restrained; the vividness belongs in the world, not the panels.

If a new colour has to be desaturated to fit in, the surrounding colours are
probably wrong, not the new one.

## Exceptions

### Rule

Two kinds of colour are **not** tokens:

- **Per-entity colour.** An item's colour, an enemy's colour, a biome's colour, a clan's colour.
  These live on that entity's definition, which is already the single source for them, putting them
  in the palette as well would create the second definition tokens exist to prevent.
- **Artwork inside one glyph.** The specific browns that make a boar look like a boar are drawing,
  not configuration. They stay in the routine that draws that one thing.

The test is whether the value is _shared_. A colour used by many different things is a token. A
colour that is part of what one specific thing looks like belongs to that thing.
