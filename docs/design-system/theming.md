# Theming

A theme is a **set of token values**. The game ships one, and changing the look means changing values
in `src/shared/design/constants/`, never touching a draw call.

## Rule

To restyle the game, edit the token sets. Do not add per-theme branches to drawing code, and do not
give a component its own colour so it can look different.

### Why

Drawing code that asks "which theme is this?" has to ask it everywhere, and every place that forgets
is a visual bug. Keeping the question in the token layer means there is exactly one place to answer
it, and every draw call inherits the answer for free.

### How to apply

A second theme is a second set of values behind the same keys, chosen once at startup:

```ts
// src/shared/design/constants/ui.constant.ts
export const UI = <theme chosen once> ;
```

Everything downstream keeps naming `UI.surface` and gets whichever value the active theme supplies.

### Exceptions

**Day and night are not themes.** The world darkens through a simulated value that the renderer
applies as an overlay, because it is a property of the game state, it drives what the AI can see
too, not a property of the presentation. Only presentation belongs in the token layer.
