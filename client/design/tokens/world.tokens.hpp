#pragma once

#include "client/features/render/systems/paint.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"
#include "client/design/types/color.struct.hpp"

namespace client {

/**
 * The colours of the world, carried over from the TypeScript game's tokens so
 * the rewrite looks like the game it replaces rather than like a new one.
 */

/**
 * The pen.
 *
 * Not pure black: a very dark green-black sits better on foliage than 0x000000,
 * which reads as a hole rather than as ink.
 */
constexpr Color kInk = rgb(0x14110d);

// ---------------------------------------------------------------- the widths
//
// All of these are in WORLD UNITS, not pixels. Paint multiplies every stroke by
// the view's scale, so a line of 1.5 is 1.5 units of island wide and grows and
// shrinks as you zoom. See docs/systems/world-scale.md.
//
// They are a family, not five free numbers. Keep them in this order, heaviest
// first, or the drawing stops reading as one hand:
//
//     kInkWidth  >=  kInkMark  >=  kInkHatch  >=  kInkFine
//
// A rough ratio that holds up: mark about 0.85 of the pen, hatch about 0.65,
// fine about 0.55. Turning the pen up without the rest turns everything into a
// silhouette; turning it down without the rest makes the interior marks look
// like mistakes.

/**
 * The outline round anything solid in the world: a tree, a rock, an animal, a
 * wall, a crate. The single most important number in the look of the game. Turn
 * this first, then bring the three below along with it.
 */
constexpr float kInkWidth = 1.0f;
/**
 * A mark that says what a thing is made of, drawn INSIDE an outline: the
 * fishbone of a pine's branches, the grain up a trunk, the crease in a hide.
 * Should be just under the pen, so it reads as the same nib pressed lighter.
 */
constexpr float kInkMark = 1.0f;
/**
 * The finest mark there is: a vein down a nettle leaf, the underside of a leaf
 * clump, the rim on a barrel lid. Anything at this weight is texture, not shape.
 */
constexpr float kInkFine = 1.5f;
/**
 * Hatching: the parallel strokes down the shaded side of a thing, which is how
 * a printed page says the light comes from the other way. Between a mark and a
 * fine line. Too heavy and the shaded side goes solid black.
 */
constexpr float kInkHatch = 1.7f;

/**
 * How much heavier trees and ore are inked than everything else.
 *
 * They are the things you look at most and they sit against busy ground, so
 * they carry a heavier line. 1.0 makes the island uniform and a little flat;
 * above about 2.0 the trees start to look like stickers laid on the grass.
 * Multiplies the four widths above, for nodes only.
 */
constexpr float kNodeLineScale = 1.5f;

/**
 * The marks *inside* a node, as a fraction of its outline.
 *
 * The facets of a boulder, the bedding planes of an ore seam: the lines that
 * say what a thing is made of, not the line that says where it ends. At the
 * full pen they read as cracks all the way through the rock and the ore
 * disappears behind them. The one number to turn if a stone or an ore looks
 * scribbled on.
 */
constexpr float kNodeInnerScale = 0.38f;

/**
 * The pen on a stack lying on the ground, as a fraction of kGlyphInk.
 *
 * An item on the belt is read at twenty pixels against a flat panel; the same
 * item lying in the grass is small, moving and surrounded by ink already. The
 * outline that makes it legible on the belt makes it a blot on the floor.
 */
constexpr float kGroundInkScale = 0.6f;

// ------------------------------------------------------------ item pictures
//
// These two are in PIXELS, not world units, because an item's picture is drawn
// at the same size on the belt whatever the view is doing. They are separate
// from the world's pen on purpose: a glyph is read at twenty pixels and a tree
// at two hundred, and one weight cannot serve both.

/**
 * The outline round every filled shape in an item's picture. The one number to
 * turn if the icons look too heavy or too faint.
 *
 * Roughly: 2.6 is bold and comic, 1.6 is a clean pen line, below about 1.0 the
 * glyphs start to look unfinished at belt size.
 */
constexpr float kGlyphInk = 1.0f;
/**
 * The thin mark inside a glyph: stitching on a hide, a crease, a rivet line,
 * the fold in a sheet of scrap. Keep it around two thirds of kGlyphInk.
 */
constexpr float kGlyphMark = 1.0f;

// --------------------------------------------------------------- the screens
//
// The dotted and stippled shading a printed comic is made of. Both in world
// units, both scaling with the view.

/**
 * One dot of the stipple inside a rock. Smaller than a mark by definition: at
 * anything above about 2 the shaded side of a boulder fills in solid.
 */
constexpr float kStippleDot = 1.4f;
/**
 * How far apart the hatching strokes sit. Larger is airier. Below about 6 they
 * merge into a wash and stop reading as separate strokes.
 */
constexpr float kHatchSpacing = 9.0f;
/**
 * How far apart the dots of the ground screen sit, the faint grid over every
 * biome. Same number as the hatching on purpose: they are the same press.
 * Changing one without the other makes the ground and the rocks look like two
 * different drawings.
 */
constexpr float kHalftoneSpacing = 9.0f;

// -------------------------------------------------------------- the colours

/** Snow on trees and rock. Not pure white: a touch of blue, or it reads as paper. */
constexpr Color kSnow = rgb(0xf2f7fb);
/** Unclothed skin, and the shade under it is this darkened by the drawing code. */
constexpr Color kSkin = rgb(0xc08a5e);
/** Behind everything: the letterbox bars and whatever is off the map. */
constexpr Color kVoid = rgb(0x0a0c0a);

inline Color biomeColor(sim::Biome biome) {
	switch (biome) {
		case sim::Biome::Water: return rgb(0x2f80c8);
		case sim::Biome::Grass: return rgb(0x6c9c3b);
		case sim::Biome::Forest: return rgb(0x508a31);
		case sim::Biome::Beach: return rgb(0xe8cf96);
		case sim::Biome::SnowBeach: return rgb(0xece9dd);
		case sim::Biome::Desert: return rgb(0xd8b762);
		case sim::Biome::Snow: return rgb(0xdde9f2);
		case sim::Biome::Road: return rgb(0x726d62);
	}
	return rgb(0x6c9c3b);
}

inline Color nodeColor(sim::NodeKind kind) {
	switch (kind) {
		case sim::NodeKind::Tree: return rgb(0x3aa24a);
		case sim::NodeKind::Stone: return rgb(0x9aa6b2);
		case sim::NodeKind::Metal: return rgb(0xb69269);
		case sim::NodeKind::Sulfur: return rgb(0xe7dd64);
		case sim::NodeKind::Nettle: return rgb(0x8cc94a);
		case sim::NodeKind::Barrel: return rgb(0x7a6a44);
	}
	return rgb(0x9aa6b2);
}

/**
 * The three greens of a forest pine: the near-grey green of a spruce, darker
 * and colder than the grassland's, so walking from one into the other is a
 * change you can see.
 */
constexpr Color kPineLight = rgb(0x6f8a63);
constexpr Color kPineMid = rgb(0x5b7452);
constexpr Color kPineDark = rgb(0x4a6244);

/**
 * The three greens of a grassland tree, lightest at the crown where the sun
 * is. Lighter than the forest's pines on purpose: the grassland is open
 * country and its trees read as scrub rather than as timber.
 */
constexpr Color kBroadleafLight = rgb(0x9ecc6e);
constexpr Color kBroadleafMid = rgb(0x84b45a);
constexpr Color kBroadleafDark = rgb(0x6d9a48);

}  // namespace client
