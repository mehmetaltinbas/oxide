# No hardcoded values

How tuning numbers, colours and configurable values are kept out of the code that reads them. Read
this any time you are about to type a number into a system file.

## Rule

Never write a tuning value inline. Every number that could reasonably be changed to tune the game -
a speed, a damage figure, a radius, a cooldown, a price, a capacity, a threshold, lives in a
`*.constant.ts` in the feature that owns it, and the code that uses it imports it by name.

This covers:

- **Balance numbers**: damage, health, speed, cost, yield, wage.
- **Timings**: cooldowns, durations, intervals, delays.
- **Distances and radii**: ranges, leashes, spacing, reach.
- **Capacities and thresholds**: stack limits, crew caps, queue lengths.
- **Colours, spacing and radii for anything drawn**: see [../design-system/design-tokens.md](../design-system/design-tokens.md).

### Why

- Balance lives in one reviewable place. Tuning the game is reading a folder of named numbers, not
  grepping for `0.35` and guessing which of the eleven hits is the one you meant.
- A named constant documents intent. `CREW_BLEED_OUT` says what 20 is for; `20` does not.
- The name gives the number somewhere to hang the reasoning. Every non-obvious constant in this
  codebase carries a comment saying what was measured to arrive at it.

### How to apply

```ts
// features/<feature>/constants/<name>.constant.ts

/**
 * Why this number is this number. If it came from a measurement, the
 * measurement goes here.
 */
export const <NAME> = 20;
```

```ts
// the system that uses it
import { <NAME> } from 'src/features/<feature>/constants/<name>.constant';
```

### Exceptions

Three kinds of literal are not tuning values and stay inline:

- **Structural constants of an algorithm**: `0.5` in a midpoint, `2` in a diameter, `1 / 60` for the
  fixed step. Naming these adds indirection without adding meaning.
- **Local drawing geometry inside one glyph**: the offsets that make a specific icon look like the
  thing it depicts are artwork, not configuration. See the exception in
  [../design-system/design-tokens.md](../design-system/design-tokens.md).
- **A value used exactly once, in the thing it describes**, where the surrounding line already reads
  as its own documentation.

If you are unsure which side of the line a number falls on, ask whether someone tuning the game
would ever want to change it. If yes, it is a constant.
