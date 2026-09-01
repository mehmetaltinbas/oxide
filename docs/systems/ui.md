# UI design system

**This document is the contract for how every surface in Oxide looks. Anything
new, a panel, a bar, a prompt, a screen, uses these tokens and primitives.
Do not introduce a new colour, radius or font size; add one here first.**

## The one decision

One system, two contexts. Which one a surface uses is decided by whether the
player _opened_ it:

| Context                                                                       | When                            | Treatment                                             |
| ----------------------------------------------------------------------------- | ------------------------------- | ----------------------------------------------------- |
| **Overlay**                                                                   | Always-on HUD over the world    | Transparent, frameless, light text with a soft shadow |
| **Panel**                                                                     | Something you opened on purpose | Opaque light card, hairline border, soft shadow       | The overlay never gets a border or a solid fill: at most a faint dark wash |
| behind text that would otherwise sit on noisy terrain. The panel is a reading |
| surface, so it is opaque and calm. Both share the same type scale, radii,     |
| spacing and accent colour, so they read as one design rather than two.        |

Panels dim the world behind them; overlays never do.

## Tokens

All defined in `src/game/ui.ts`. Never hardcode a colour in a draw call.

| Token        | Value              | Used for                          |
| ------------ | ------------------ | --------------------------------- |
| `surface`    | `#f7f7f5`          | Panel and card backgrounds        |
| `surfaceAlt` | `#eeeef0`          | Inset areas: slots, wells, tracks |
| `hairline`   | `rgba(0,0,0,0.10)` | Borders and dividers              |
| `ink`        | `#1c1c1e`          | Primary text                      |
| `subtle`     | `#8a8a8e`          | Secondary text, labels, units     |
| `disabled`   | `#b8b8bd`          | Unavailable text and icons        |
| `accent`     | `#0a6eeb`          | Primary action, selection         |
| `ok`         | `#2f9e4f`          | Sufficient, healthy, ready        |
| `warn`       | `#c2452f`          | Insufficient, damaged, danger     |
| `hot`        | `#c2732f`          | Fire, heat, burning               |

### Overlay tokens

| Token          | Value                    | Used for                            |
| -------------- | ------------------------ | ----------------------------------- |
| `glass`        | `rgba(18,20,18,0.30)`    | Faint wash behind overlay text      |
| `glassEdge`    | `rgba(255,255,255,0.07)` | The only border an overlay may have |
| `onDark`       | `#f2f1ec`                | Overlay primary text                |
| `onDarkSubtle` | `rgba(242,241,236,0.60)` | Overlay secondary text              |
| `onDarkFaint`  | `rgba(242,241,236,0.34)` | Overlay hints and shortcuts         |
| `track`        | `rgba(0,0,0,0.38)`       | Meter tracks in the overlay         |

Radii: `sm 6`, `md 10`, `lg 16`. Spacing is a 4px scale; panels pad by 24.

Type is one monospace family throughout. Sizes: `title 20/600`, `heading 17/600`,
`body 13`, `label 12`, `caption 11`, `micro 9`. Weight is either normal or 600 -
never bold beyond that.

## Primitives

Use these rather than raw canvas calls:

- `panel(x, y, w, h)`: card with shadow, fill and hairline border
- `glass(x, y, w, h)`: overlay wash, no border
- `textOnDark(str, x, y, opts)`: light text with a legibility shadow
- `meterOnDark` / `slotOnDark`: overlay variants of the same primitives
- `roundRect(x, y, w, h, r)`: path only
- `divider(x, y, w)`: hairline rule
- `meter(x, y, w, h, value, color)`: inset track plus fill
- `button(rect, label, { enabled, hovered })`: accent-filled action
- `slot(rect, { hovered, selected })`: inventory-style well
- `text(str, x, y, { size, weight, color, align })`

## Layout rules

- Panels are centred, capped at 880x560, and always inset at least 40 from the
  viewport edge.
- Every panel has a header: title on the left, dismissal hint on the right, a
  divider under it.
- Layered browsing goes left to right: **category → list → detail**, separated
  by vertical hairlines.
- Interactive rows fill their column width and are 36-42 tall.
- The world dims to `rgba(0,0,0,0.40)` behind any open panel.

## Item icons

Items are **drawn shapes**, never coloured squares. Each item has a glyph in
`src/game/icons.ts` rendered by `drawItemIcon`. A new item needs a glyph before
it ships; falling back to a square is not acceptable. Icons are drawn inside a
square box and scale to it, so the same glyph serves the belt, inventory,
crafting list and ground drops.

## The frame counter

A smoothed FPS read-out sits in the very top-left corner at `micro` size in
`onDarkFaint`, turning `hot` below 50 and `warn` below 30. It is diagnostic
furniture, so it is the quietest thing on screen and never gains a background.

## Colour

Palette rules live with the tokens they govern: see
[../design-system/design-tokens.md](../design-system/design-tokens.md).

## Invariants

- One accent colour. If something needs to stand out and is not an action,
  use weight or `subtle`, not a new hue.
- No pure black and no pure white text.
- Nothing blinks except a genuine alarm state.
- The overlay never draws a border thicker than 1px, and never a solid fill.

## One screen, several tabs

The inventory, the crafting list, an open crate and the sandbox shelf are pages of one screen, not
four separate screens. A tab strip sits on top of whichever panel is showing and switches between
them; `openContainer` survives the switch, so you can step over to crafting and back to the crate
without walking away from it.

`Tab` is the one key that means "put this away". Whatever is open closes, whatever the page. With
nothing open it brings up the pack. `c` is still a direct shortcut to crafting, and `f` to the
sandbox shelf when that mode is on.

## Channel bar

Anything you are part-way through draws one progress bar above the belt: bandages, syringes,
magazine changes. It carries the action's name and the rule that governs it, because the rule is
not obvious: you may keep walking, you may not run. It reads its fraction from `useLeft/useTotal`
or `reloadLeft/reloadTotal`, so adding another channel means feeding it another pair, not drawing
another bar.

## Sandbox

Creative mode, started from its own button on the title card. Nothing costs anything, nothing can
kill you, every workbench tier is assumed, crafting is instant, and the Sandbox tab hands you any
item in the game in ones, tens, hundreds or thousands. `[` and `]` push the clock an hour either
way. It is gated on `game.sandbox`; nothing else in the simulation knows the mode exists, because
the vitals are simply refilled every tick rather than having their drain conditionalised.

## Craft amounts

The amount row is a stepper, a max button and a field you can type into, and the craft button
carries Rust's modifiers: shift for ten more, ctrl for as many as you can pay for.

The text field takes ownership of the number while it has focus. Clamping the typed value back into
range on every frame would fight whoever is halfway through typing "12", so the raw text lives in
`amountText` and only the parsed value gets clamped.

Reading typed characters needs `Input.typed` and `Input.edits`, which collect printable keys and
backspace/enter/escape for the frame. Note that ctrl never reaches the key handler at all (the
browser keeps its own ctrl shortcuts), which is why `ctrlClick` and `shiftClick` are read off the
mouse event instead of from the key set.

## How enclosure is worked out

`BuildSystem.rebuildEnclosure` runs only when the layout changes, and works building by building:
foundations that touch (eight-way) form a clump, and each clump gets its own padded box. One box
around every structure on the island spans tens of thousands of empty cells and cost over 20ms to
fill; per building it is 0.3ms.

Inside a box it floods **from the outside in**: mark everything the open air can reach, then group
whatever is left into rooms. The obvious alternative, flood out from each cell and ask "did I
escape", shares one visited set between floods, which makes the answer depend on the order cells
happen to be scanned in. A breached room whose surrounding ground had already been visited looked
sealed, because the way out was marked seen and never followed, and blowing a wall left the roof on.
