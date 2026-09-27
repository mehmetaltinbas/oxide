# Design tokens

Every visual value has one home. Read this before typing a colour or a line
width into a draw call.

## Colour and weight come from a token

### Rule

No draw call contains a literal colour or a literal line width. They come from
`client/design/tokens/world.tokens.hpp` for the world and `client/design/tokens/interface.tokens.hpp` for the
interface.

### Why

One place to change the look, and one place to read it. It is also the only way
the game stays one drawing: an ink weight typed at a call site is a weight
nobody else can match.

### How to apply

```cpp
// No.
paint.outlinePoly(shape, 1.5f, Color{20, 17, 13, 255});
// Yes.
paint.outlinePoly(shape, kInkWidth, kInk);
```

The world's tokens, in `palette.hpp`:

| Token | What it is |
| --- | --- |
| `kInk` | the pen. A very dark green-black, never pure black |
| `kInkWidth` | the outline round anything solid. **Turn this first** |
| `kInkMark` | a mark inside an outline: grain, branches, a crease |
| `kInkHatch` | the strokes down a thing's shaded side |
| `kInkFine` | the finest mark: a vein, a rim, a leaf underside |
| `kNodeLineScale` | how much heavier trees and ore are inked than everything else |
| `kGlyphInk`, `kGlyphMark` | the pen on an item's picture |
| `kStippleDot` | one dot of the stipple inside a rock |
| `kHatchSpacing`, `kHalftoneSpacing` | how far apart hatching and the ground screen sit |
| `biomeColor`, `nodeColor` | one colour per biome and per node kind |

The interface's tokens, in `ui.hpp`: `kSurface`, `kSurfaceAlt`, `kHairline`,
`kAccent`, `kAccentHover`, `kAccentInk`, `kSelected`, `kHover`, `kWarn`,
`kSubtle`, `kFaint`, `kDisabled`, and the radii.

### Exceptions

A colour that belongs to one specific picture and nothing else stays in that
picture: the orange of a campfire's flame, the ivory of an elk's antler. Hoisting
every one of those into the palette would make the palette a list of fifty
names used once each.

## Tokens that must move together

### Rule

Some tokens are a family and have to keep their relationship. Changing one
without the others breaks the look in a way that is hard to name afterwards.

| Family | Relationship |
| --- | --- |
| `kInkWidth` ≥ `kInkMark` ≥ `kInkHatch` ≥ `kInkFine` | roughly 1, 0.85, 0.65, 0.55 of the pen |
| `kGlyphInk` and `kGlyphMark` | the mark about two thirds of the pen |
| `kHatchSpacing` and `kHalftoneSpacing` | the same number: they are the same press |

### Why

The last one is the one that catches people. The halftone is the dots over the
ground and the hatching is the strokes inside rocks and trees. If they sit at
different spacings the ground and the things standing on it read as two
different drawings by two different hands.

### How to apply

Turn `kInkWidth` first and bring the other three with it in proportion. If the
island goes flat, the marks have caught up with the pen; if everything turns
into a silhouette, the pen has run away from the marks.

## A width is in world units

### Rule

Every width passed to `Paint` is in **world units**, not pixels. `Paint::line`
multiplies by the view's scale, and every outline and inked shape goes through
`Paint::line`.

### Why

So the ink round a thing grows and shrinks with the thing. See
[world-scale.md](../systems/world-scale.md), which is the full rule and the
mistake it exists to prevent.

### How to apply

If you already have a width in screen pixels, hand it back through
`Paint::inWorld` rather than letting the pen scale it a second time. Scaling
twice squares the zoom: that is what made a held gun's outline nine times heavy
at three times in.
