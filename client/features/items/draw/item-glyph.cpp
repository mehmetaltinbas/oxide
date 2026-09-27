#include "client/features/items/draw/item-glyph.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>
#include <vector>

#include "client/design/tokens/world.tokens.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/creatures/types/body-frame.struct.hpp"
#include "client/features/items/types/melee-pose.struct.hpp"
#include "client/features/items/types/melee-style.enum.hpp"
#include "client/features/render/types/point.struct.hpp"

namespace client {

namespace {

constexpr Color kSteel = rgb(0xd5dae0);
constexpr Color kSteelDark = rgb(0x8d959e);
constexpr Color kWoodHandle = rgb(0x8a5a2e);
constexpr Color kWood = rgb(0x8a6034);
constexpr Color kWoodDark = rgb(0x5d4022);

/** A shape given in the tool's own frame, laid into the body's. */
struct ToolFrame {
	const BodyFrame& body;
	Point grip;
	float angle;
	float stretch;

	Point at(float along, float across) const {
		// `along` runs down the handle, `across` is at right angles to it. The
		// stretch squashes the length only, which is what foreshortening is.
		const float a = along * stretch;
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		return body.at(grip.x + a * s + across * c, grip.y - a * c + across * s);
	}
};

void inked(Paint& paint, const std::vector<Point>& pts, Color fill) {
	paint.fillPoly(pts, fill);
	paint.outlinePoly(pts, kInkFine, kInk);
}

/** The handle every tool hangs off, from the grip forward. */
void handle(Paint& paint, const ToolFrame& t, float from, float to, float half) {
	inked(paint, {t.at(from, -half), t.at(to, -half), t.at(to, half), t.at(from, half)}, kWoodHandle);
}

}  // namespace

MeleeStyle meleeStyleOf(sim::ItemId item) {
	switch (item) {
		case sim::ItemId::Hatchet:
		case sim::ItemId::Pickaxe:
		case sim::ItemId::Hammer: return MeleeStyle::Chop;
		case sim::ItemId::Torch:
		case sim::ItemId::Spear:
		case sim::ItemId::Bow:
		case sim::ItemId::Revolver:
		case sim::ItemId::Waterpipe:
		case sim::ItemId::PumpShotgun:
		case sim::ItemId::Rifle:
		case sim::ItemId::Ak47: return MeleeStyle::Thrust;
		default: return MeleeStyle::Smash;
	}
}

/**
 * How each item sits in a fist.
 *
 * One row per item held some particular way, read off its own glyph: where the
 * handle ends, and which way the blade or the barrel points. Anything without
 * a row is held in the middle of the fist at a size that reads as a thing in a
 * hand rather than a thing on the ground. Carried over from the TypeScript
 * game's table, which is why the numbers match its icons.
 */
const HeldPose& heldPoseOf(sim::ItemId item) {
	static constexpr HeldPose kDefault{14, 0, 0, 0};
	static constexpr float kHalfPi = 1.57079632679f;
	switch (item) {
		// A stone in the fist, gripped from behind so it shows past the
		// knuckles; held by its middle the hand covered it completely.
		case sim::ItemId::Rock: {
			static constexpr HeldPose p{17, 0, 0.2f, 0};
			return p;
		}
		// Tools by the end of the handle, head forward. The hatchet's glyph
		// leans its handle about sixteen degrees; turned back upright.
		case sim::ItemId::Hatchet: {
			static constexpr HeldPose p{28, 0.11f, 0.28f, 0.28f};
			return p;
		}
		case sim::ItemId::Pickaxe: {
			static constexpr HeldPose p{30, 0, 0.28f, 0};
			return p;
		}
		case sim::ItemId::Hammer: {
			static constexpr HeldPose p{24, 0, 0.28f, 0};
			return p;
		}
		case sim::ItemId::Torch: {
			static constexpr HeldPose p{26, 0, 0.3f, 0};
			return p;
		}
		case sim::ItemId::BuildingPlan: {
			static constexpr HeldPose p{20, 0, 0.22f, 0};
			return p;
		}
		// The spear's shaft runs corner to corner in its glyph; turned to
		// point ahead.
		case sim::ItemId::Spear: {
			static constexpr HeldPose p{44, -0.12f, 0.16f, -0.69f};
			return p;
		}
		// Guns point along +x in their glyphs.
		case sim::ItemId::Revolver: {
			static constexpr HeldPose p{26, -0.17f, 0.16f, -kHalfPi};
			return p;
		}
		case sim::ItemId::Rifle: {
			static constexpr HeldPose p{40, -0.12f, 0.12f, -kHalfPi};
			return p;
		}
		case sim::ItemId::RocketLauncher: {
			static constexpr HeldPose p{46, -0.05f, 0.1f, -kHalfPi};
			return p;
		}
		case sim::ItemId::Waterpipe: {
			static constexpr HeldPose p{36, -0.12f, 0.1f, -kHalfPi};
			return p;
		}
		case sim::ItemId::PumpShotgun: {
			static constexpr HeldPose p{38, -0.12f, 0.1f, -kHalfPi};
			return p;
		}
		case sim::ItemId::Ak47: {
			static constexpr HeldPose p{42, -0.1f, 0.08f, -kHalfPi};
			return p;
		}
		default: return kDefault;
	}
}

/**
 * What is in someone's hand.
 *
 * Drawn from the item's own picture rather than from a second set of handles
 * and blades kept beside it: there is one hatchet in this game and the one on
 * your belt is the one you swing. `pose` says where the hand is and how far
 * through a blow it is; the item's own row says how it is gripped.
 */
void drawHeldItem(Paint& paint, const BodyFrame& body, sim::ItemId item, const MeleePose& pose,
				  float bowDraw, bool flip, bool magOut) {
	if (item == sim::ItemId::None) return;
	// The bow is the one thing not drawn from its own picture: it is held
	// across the body with a string that moves, which no flat glyph can say.
	const float bowShift = pose.overHand ? 9.0f * pose.stretch : 0.0f;
	const ToolFrame tool{body, {pose.hand.x, pose.hand.y + bowShift}, pose.angle, pose.stretch};
	switch (item) {
		case sim::ItemId::Bow: {
			// Seen from above with the grip in the fist: the limbs sweep back
			// towards the archer, the string runs between their tips, and
			// drawing pulls it into a V with an arrow on it.
			const float tipAcross = 9.5f;
			// The tips sit behind the hand, and come back further as it draws.
			const float tipAlong = -(5.5f - bowDraw * 1.5f);
			const float nock = -(4.5f + bowDraw * 9.0f);
			const Point leftTip = tool.at(tipAlong, -tipAcross);
			const Point rightTip = tool.at(tipAlong, tipAcross);
			const Point nockAt = tool.at(nock, 0);
			paint.line(leftTip.x, leftTip.y, nockAt.x, nockAt.y, 1.0f, kInk);
			paint.line(nockAt.x, nockAt.y, rightTip.x, rightTip.y, 1.0f, kInk);
			if (bowDraw > 0) {
				// The arrow on the string, its head out ahead of the grip.
				const float head = nock + 26;
				const Point shaft = tool.at(head - 3, 0);
				paint.line(nockAt.x, nockAt.y, shaft.x, shaft.y, 1.8f, kWoodHandle);
				inked(paint, {tool.at(head + 1, 0), tool.at(head - 3, -2), tool.at(head - 3, 2)},
					  rgb(0xcfd8e0));
			}
			// The limbs: one curve from tip to tip, bowing out ahead of the
			// hand, which is what makes it a bow rather than a stick.
			std::vector<Point> limbs;
			for (int i = 0; i <= 12; ++i) {
				const float u = i / 12.0f;
				const float w0 = (1 - u) * (1 - u);
				const float w1 = 2 * u * (1 - u);
				const float w2 = u * u;
				const float along = w0 * tipAlong + w1 * (-tipAlong * 2.0f) + w2 * tipAlong;
				const float across = w0 * -tipAcross + w2 * tipAcross;
				limbs.push_back(tool.at(along, across));
			}
			paint.outlinePoly(limbs, 2.6f + kInkWidth, kInk, false);
			paint.outlinePoly(limbs, 2.6f, kWoodHandle, false);
			break;
		}

		default: break;
	}

	const HeldPose& grip = heldPoseOf(item);
	// The picture is foreshortened by the same amount the swing is: a tool
	// raised over the shoulder points half at the sky and looks shorter.
	const float size = grip.size * pose.stretch * body.scale;
	// Held upright, what stands over the fist is the head, so the picture is
	// slid down until its head is where the hand is. -0.2 is where the
	// business end of every one of these glyphs sits along its own length.
	constexpr float kHeadY = -0.2f;
	const float shift = pose.overHand ? (grip.gripY - kHeadY) * grip.size * pose.stretch : 0.0f;
	const Point hand = body.at(pose.hand.x, pose.hand.y + shift);

	// The body's own facing, plus the swing, plus how the item is gripped.
	const float facing = std::atan2(body.sin, body.cos);
	const float turn = facing + pose.angle;
	const float angle = turn + grip.angle;
	// The grip is the point of the picture that lands on the knuckles, so the
	// picture's middle goes the other way from it.
	const float offX = -grip.gripX * size;
	const float offY = -grip.gripY * size;
	const float cx = hand.x + offX * std::cos(angle) - offY * std::sin(angle);
	const float cy = hand.y + offX * std::sin(angle) + offY * std::cos(angle);
	drawItemIcon(paint, item, cx, cy, size, angle, flip, magOut);
}

/**
 * Every item, as a drawn glyph rather than a coloured square.
 *
 * The shapes are given in a unit box centred on nought, spanning -0.5 to 0.5,
 * and scaled to whatever size the caller wants, so one glyph serves the belt,
 * the pack, the crafting list and a stack lying on the ground. Ported shape for
 * shape from the TypeScript game's icon sheet.
 *
 * Inking follows the same rule the canvas game applies to a glyph: a fill gets
 * the pen round it, a thin mark is redrawn as a bold black one, and a shape
 * drawn as a line gets ink laid under it and its own colour on top.
 */
void drawItemIcon(Paint& paint, sim::ItemId item, float x, float y, float size, float angle,
				  bool flip, bool magOut) {
	const float u = size;
	// The pen, in proportion to the picture rather than in flat pixels.
	//
	// A fixed pixel width is a trap on a dense display: the belt's icons are
	// laid out in points and drawn at twice that, so a flat 1.0 came out half a
	// point wide and vanished. Reading kGlyphInk as "ink per twenty six pixels
	// of icon" makes it hold at any size and any display.
	//
	// Measured against the picture's size IN WORLD UNITS, because Paint scales
	// every stroke by the view as well. A gun in your hand is drawn larger as
	// you zoom in, so working the pen out from its on-screen size and then
	// letting Paint scale it again squared the zoom: at three times in, the
	// held item's outline came out nine times heavy.
	const float measure = std::max(1.0f, paint.inWorld(u) / 26.0f);
	const float ink = kGlyphInk * measure;
	const float mark = kGlyphMark * measure;
	const float ca = std::cos(angle);
	const float sa = std::sin(angle);
	// Turned over along its own length when asked: the picture is drawn in
	// profile, so mirroring it across that axis puts the magazine on the other
	// side of the gun without pointing the barrel back at its owner.
	const float side = flip ? -1.0f : 1.0f;
	const auto P = [&](float ux, float uyIn) {
		const float uy = uyIn * side;
		return Point{x + (ux * ca - uy * sa) * u, y + (ux * sa + uy * ca) * u};
	};
	const auto pts = [&](std::initializer_list<std::pair<float, float>> list) {
		std::vector<Point> out;
		out.reserve(list.size());
		for (const auto& [ux, uy] : list) out.push_back(P(ux, uy));
		return out;
	};
	// A filled shape: the pen goes round it, at the weight a glyph is inked at.
	const auto fill = [&](const std::vector<Point>& shape, Color color) {
		paint.fillPoly(shape, color);
		paint.outlinePoly(shape, ink, kInk);
	};
	// The same, with the shape's own edge colour drawn back over the pen.
	// The shape's own edge colour, with the pen laid UNDER it rather than over.
	// It was the other way round, and since the colour is the wider of the two
	// it painted over every black line in the sheet: the ink was being drawn
	// and then hidden, which is why turning kGlyphInk up did nothing.
	const auto edged = [&](const std::vector<Point>& shape, Color color, Color edge) {
		paint.fillPoly(shape, color);
		const float own = paint.inWorld(0.045f * u);
		paint.outlinePoly(shape, own + ink * 2, kInk);
		paint.outlinePoly(shape, own, edge);
	};
	const auto box = [&](float ux, float uy, float uw, float uh, Color color) {
		fill(pts({{ux, uy}, {ux + uw, uy}, {ux + uw, uy + uh}, {ux, uy + uh}}), color);
	};
	// A disc is round whatever the picture is turned to, so it needs no frame.
	const auto disc = [&](float ux, float uy, float ur, Color color) {
		const Point at = P(ux, uy);
		paint.inkedCircle(at.x, at.y, ur * u, color, ink);
	};
	const auto plainDisc = [&](float ux, float uy, float ur, Color color) {
		const Point at = P(ux, uy);
		paint.fillCircle(at.x, at.y, ur * u, color);
	};
	// A mark too fine to be a shape: black, whatever colour it was asked for.
	const auto detail = [&](float x0, float y0, float x1, float y1, float w) {
		const Point a = P(x0, y0);
		const Point b = P(x1, y1);
		paint.line(a.x, a.y, b.x, b.y, std::max(paint.inWorld(w * u), mark), kInk);
	};
	// A shape drawn as a line: ink under it, its own colour over.
	const auto stroke = [&](float x0, float y0, float x1, float y1, float w, Color color) {
		const Point a = P(x0, y0);
		const Point b = P(x1, y1);
		const float thick = paint.inWorld(w * u);
		paint.line(a.x, a.y, b.x, b.y, thick + ink * 2, kInk);
		paint.line(a.x, a.y, b.x, b.y, thick, color);
	};
	// An arc drawn as a line, for a bow's limb and a lock's shackle.
	const auto arc = [&](float ux, float uy, float ur, float from, float to, float w, Color color) {
		std::vector<Point> along;
		for (int i = 0; i <= 14; ++i) {
			const float a = from + (to - from) * (static_cast<float>(i) / 14);
			along.push_back(P(ux + std::cos(a) * ur, uy + std::sin(a) * ur));
		}
		(void)0;
		const float thick = paint.inWorld(w * u);
		paint.outlinePoly(along, thick + ink * 2, kInk, false);
		paint.outlinePoly(along, thick, color, false);
	};
	// The wooden handle every tool hangs off.
	const auto haft = [&](float x0, float y0, float x1, float y1, float w = 0.07f) {
		stroke(x0, y0, x1, y1, w, kWood);
	};
	/** A loose pile of chunks, which is what every raw resource looks like. */
	const auto pile = [&](Color body, Color edge) {
		const float bits[4][3] = {
			{-0.2f, 0.12f, 0.17f}, {0.16f, 0.14f, 0.15f}, {-0.02f, -0.02f, 0.19f},
			{0.1f, -0.18f, 0.13f}};
		for (const auto& b : bits) {
			const float bx = b[0];
			const float by = b[1];
			const float r = b[2];
			edged(pts({{bx - r, by + r * 0.6f},
					   {bx - r * 0.4f, by - r},
					   {bx + r * 0.8f, by - r * 0.5f},
					   {bx + r, by + r * 0.7f}}),
				  body, edge);
		}
	};

	switch (item) {
		// ------------------------------------------------------- resources
		case sim::ItemId::Wood: {
			// Stacked logs, end on.
			const float ends[3][2] = {{-0.17f, 0.1f}, {0.17f, 0.1f}, {0, -0.16f}};
			for (const auto& e : ends) {
				disc(e[0], e[1], 0.19f, kWood);
				plainDisc(e[0], e[1], 0.11f, rgb(0xa97a44));
				plainDisc(e[0], e[1], 0.045f, kWoodDark);
			}
			break;
		}
		case sim::ItemId::Stone: pile(rgb(0x9aa2aa), rgb(0x6f767d)); break;
		case sim::ItemId::MetalOre:
			pile(rgb(0x9c8060), rgb(0x6d5840));
			plainDisc(-0.05f, -0.05f, 0.05f, rgb(0xd8c9a8));
			plainDisc(0.14f, 0.13f, 0.04f, rgb(0xd8c9a8));
			break;
		case sim::ItemId::SulfurOre:
			pile(rgb(0xc9c05a), rgb(0x8a832f));
			plainDisc(-0.04f, -0.04f, 0.05f, rgb(0xf2ea9a));
			break;
		case sim::ItemId::Metal:
			// Flat fragments.
			edged(pts({{-0.3f, 0.05f}, {-0.05f, -0.12f}, {0.12f, 0.02f}, {-0.1f, 0.2f}}), kSteel,
				  kSteelDark);
			edged(pts({{0.02f, -0.06f}, {0.28f, -0.18f}, {0.32f, 0.06f}, {0.12f, 0.12f}}),
				  rgb(0xdbe4ec), kSteelDark);
			break;
		case sim::ItemId::Sulfur:
			edged(pts({{-0.26f, 0.1f}, {-0.08f, -0.16f}, {0.1f, 0.04f}, {-0.06f, 0.22f}}),
				  rgb(0xe8dc6a), rgb(0xa89a2f));
			edged(pts({{0.04f, -0.02f}, {0.28f, -0.14f}, {0.3f, 0.1f}, {0.1f, 0.16f}}),
				  rgb(0xf2ea9a), rgb(0xa89a2f));
			break;
		case sim::ItemId::Charcoal: pile(rgb(0x4a4a4a), rgb(0x2a2a2a)); break;
		case sim::ItemId::HqMetalOre:
			// The same rock as metal ore, shot through with something brighter.
			pile(rgb(0x6f7c86), rgb(0x3f484f));
			plainDisc(-0.05f, -0.05f, 0.055f, rgb(0x9fe4ff));
			plainDisc(0.14f, 0.12f, 0.045f, rgb(0x9fe4ff));
			plainDisc(-0.17f, 0.14f, 0.035f, rgb(0x7ec8e8));
			break;
		case sim::ItemId::HqMetal:
			// Milled bar stock, stacked: flat, bright and squared off, which is
			// what says refined next to a pile of chunks.
			edged(pts({{-0.3f, 0.02f}, {0.18f, -0.14f}, {0.3f, 0.02f}, {-0.18f, 0.18f}}),
				  rgb(0xcfe6f2), rgb(0x7a94a4));
			edged(pts({{-0.3f, -0.14f}, {0.18f, -0.3f}, {0.3f, -0.14f}, {-0.18f, 0.02f}}),
				  rgb(0xeaf6ff), rgb(0x7a94a4));
			detail(-0.1f, -0.16f, 0.12f, -0.23f, 0.03f);
			break;
		case sim::ItemId::Cloth:
			// A folded bolt of fabric.
			edged(pts({{-0.3f, -0.14f}, {0.3f, -0.22f}, {0.3f, 0.06f}, {-0.3f, 0.14f}}),
				  rgb(0xc8b89a), rgb(0x9a8b6e));
			edged(pts({{-0.3f, 0.06f}, {0.3f, -0.02f}, {0.3f, 0.2f}, {-0.3f, 0.28f}}),
				  rgb(0xdccbaa), rgb(0x9a8b6e));
			break;
		case sim::ItemId::Leather: {
			// Nothing on it is a ruled line: a cut of leather holds no straight
			// edge, and the square version read as a crate.
			std::vector<Point> hide{P(-0.3f, -0.18f), P(-0.16f, -0.24f), P(-0.02f, -0.25f),
									P(0.08f, -0.21f), P(0.18f, -0.18f), P(0.3f, 0.06f),
									P(0.16f, 0.18f), P(-0.06f, 0.26f), P(-0.3f, 0.18f)};
			edged(hide, rgb(0xa3714a), rgb(0x5d3d22));
			// The far corner curling back, its underside lighter: this is what
			// says leather at pack size, where the stitching is a few pixels.
			edged(pts({{0.18f, -0.18f}, {0.3f, -0.15f}, {0.3f, 0.06f}, {0.2f, -0.04f}}),
				  rgb(0xc9935c), rgb(0x5d3d22));
			for (int i = 0; i < 4; ++i) {
				const float t = i * 0.12f;
				detail(-0.22f + t, 0.16f, -0.17f + t, 0.145f, 0.028f);
			}
			detail(-0.2f, -0.04f, -0.02f, 0.0f, 0.028f);
			detail(-0.02f, 0.0f, 0.12f, -0.06f, 0.028f);
			break;
		}
		case sim::ItemId::Bone:
			stroke(-0.2f, 0.18f, 0.2f, -0.18f, 0.13f, rgb(0xddd6c0));
			disc(-0.24f, 0.18f, 0.09f, rgb(0xeee7d2));
			disc(-0.16f, 0.25f, 0.08f, rgb(0xeee7d2));
			disc(0.24f, -0.18f, 0.09f, rgb(0xeee7d2));
			disc(0.16f, -0.25f, 0.08f, rgb(0xeee7d2));
			break;
		case sim::ItemId::Scrap:
			// Torn, rusted plate, two pieces of it, one lying over the other.
			edged(pts({{-0.34f, 0.02f}, {-0.12f, -0.16f}, {0.1f, -0.06f}, {0.04f, 0.22f},
					   {-0.26f, 0.28f}}),
				  rgb(0x7d4426), rgb(0x4a2617));
			edged(pts({{-0.06f, -0.28f}, {0.3f, -0.18f}, {0.34f, 0.06f}, {0.06f, 0.16f}}),
				  rgb(0xb4643a), rgb(0x5e3220));
			fill(pts({{-0.06f, -0.28f}, {0.14f, -0.23f}, {0.2f, 0.11f}, {0.06f, 0.16f}}),
				 rgb(0xcc8452));
			plainDisc(0.11f, -0.12f, 0.035f, rgb(0x4a2617));
			plainDisc(0.16f, 0.03f, 0.035f, rgb(0x4a2617));
			plainDisc(-0.2f, 0.06f, 0.03f, rgb(0x4a2617));
			plainDisc(-0.14f, 0.17f, 0.022f, rgb(0x4a2617));
			plainDisc(0.26f, -0.04f, 0.022f, rgb(0x5e3220));
			break;
		case sim::ItemId::Gunpowder:
			// A pouch of powder.
			edged(pts({{-0.22f, 0.3f}, {-0.26f, -0.02f}, {-0.1f, -0.2f}, {0.1f, -0.2f},
					   {0.26f, -0.02f}, {0.22f, 0.3f}}),
				  rgb(0x6d6a5a), rgb(0x43413a));
			box(-0.08f, -0.3f, 0.16f, 0.12f, rgb(0x4a483f));
			plainDisc(-0.06f, 0.08f, 0.03f, rgb(0x3a382f));
			plainDisc(0.06f, 0.16f, 0.03f, rgb(0x3a382f));
			break;
		case sim::ItemId::AnimalFat:
			// A trimmed slab: pale, marbled, faintly greasy.
			edged(pts({{-0.26f, -0.06f}, {-0.14f, -0.24f}, {0.16f, -0.26f}, {0.28f, -0.02f},
					   {0.16f, 0.24f}, {-0.18f, 0.22f}}),
				  rgb(0xf0e6c8), rgb(0xc2b48c));
			plainDisc(-0.04f, -0.06f, 0.07f, rgb(0xfff8e4));
			plainDisc(0.11f, 0.09f, 0.045f, rgb(0xfff8e4));
			plainDisc(-0.12f, 0.11f, 0.035f, rgb(0xd9cca6));
			break;
		case sim::ItemId::LowGrade:
			// A jerry can.
			edged(pts({{-0.22f, -0.2f}, {0.2f, -0.2f}, {0.2f, 0.28f}, {-0.22f, 0.28f}}),
				  rgb(0xc9a94a), rgb(0x8a7226));
			box(-0.08f, -0.3f, 0.14f, 0.1f, rgb(0x8a7226));
			detail(-0.14f, -0.1f, 0.12f, 0.2f, 0.04f);
			break;
		case sim::ItemId::Water:
			edged(pts({{0, -0.3f}, {0.22f, 0.02f}, {0.16f, 0.22f}, {-0.16f, 0.22f},
					   {-0.22f, 0.02f}}),
				  rgb(0x5aa8d8), rgb(0x2f7aa8));
			plainDisc(-0.07f, 0.06f, 0.05f, rgb(0xa8d8f0));
			break;
		case sim::ItemId::MeatRaw:
			edged(pts({{-0.26f, 0.04f}, {-0.12f, -0.22f}, {0.16f, -0.24f}, {0.28f, 0.02f},
					   {0.12f, 0.26f}, {-0.16f, 0.24f}}),
				  rgb(0xb45c5c), rgb(0x7d3535));
			plainDisc(0.02f, 0.0f, 0.09f, rgb(0xd78585));
			box(-0.05f, 0.16f, 0.1f, 0.14f, rgb(0xeee7d2));
			break;
		case sim::ItemId::MeatCooked:
			edged(pts({{-0.26f, 0.04f}, {-0.12f, -0.22f}, {0.16f, -0.24f}, {0.28f, 0.02f},
					   {0.12f, 0.26f}, {-0.16f, 0.24f}}),
				  rgb(0x8a5a34), rgb(0x573418));
			plainDisc(0.02f, 0.0f, 0.09f, rgb(0xa87244));
			box(-0.05f, 0.16f, 0.1f, 0.14f, rgb(0xeee7d2));
			break;

		// ----------------------------------------------------------- tools
		case sim::ItemId::Rock:
			edged(pts({{-0.28f, 0.06f}, {-0.14f, -0.22f}, {0.14f, -0.26f}, {0.3f, -0.02f},
					   {0.2f, 0.24f}, {-0.12f, 0.26f}}),
				  rgb(0x8b8b8b), rgb(0x5f5f5f));
			fill(pts({{-0.1f, -0.12f}, {0.08f, -0.16f}, {0.12f, 0.0f}, {-0.04f, 0.06f}}),
				 rgb(0xa5a5a5));
			break;
		case sim::ItemId::Hatchet:
			haft(0.12f, 0.32f, -0.02f, -0.16f);
			edged(pts({{-0.04f, -0.3f}, {0.2f, -0.2f}, {0.16f, 0.02f}, {-0.06f, -0.06f}}), kSteel,
				  kSteelDark);
			edged(pts({{-0.04f, -0.3f}, {-0.2f, -0.22f}, {-0.2f, -0.02f}, {-0.06f, -0.06f}}),
				  rgb(0xe2ebf3), kSteelDark);
			break;
		case sim::ItemId::Pickaxe: {
			haft(0.0f, 0.34f, 0.0f, -0.1f);
			// The head, as one curved bar with a point at each end.
			std::vector<Point> head;
			for (int i = 0; i <= 12; ++i) {
				const float t = static_cast<float>(i) / 12;
				const float ux = -0.3f + 0.6f * t;
				const float uy = (1 - t) * (1 - t) * -0.02f + 2 * (1 - t) * t * -0.34f +
								 t * t * -0.02f;
				head.push_back(P(ux, uy));
			}
			const float bar = paint.inWorld(0.1f * u);
			paint.outlinePoly(head, bar + ink * 2, kInk, false);
			paint.outlinePoly(head, bar, kSteel, false);
			fill(pts({{-0.3f, -0.02f}, {-0.34f, -0.12f}, {-0.22f, -0.1f}}), kSteelDark);
			fill(pts({{0.3f, -0.02f}, {0.34f, -0.12f}, {0.22f, -0.1f}}), kSteelDark);
			break;
		}
		case sim::ItemId::Hammer:
			haft(0.0f, 0.34f, 0.0f, -0.12f);
			box(-0.24f, -0.3f, 0.48f, 0.2f, rgb(0xd8c08a));
			box(-0.24f, -0.3f, 0.12f, 0.2f, rgb(0xb09a63));
			break;
		case sim::ItemId::Torch:
			// A stick with a rag alight on the end of it.
			haft(-0.08f, 0.34f, 0.02f, -0.02f, 0.075f);
			edged(pts({{0.02f, -0.02f}, {0.12f, -0.08f}, {0.1f, -0.2f}, {-0.06f, -0.2f},
					   {-0.08f, -0.08f}}),
				  rgb(0x6a5236), rgb(0x3f3020));
			fill(pts({{0.02f, -0.36f}, {0.14f, -0.18f}, {0.02f, -0.1f}, {-0.1f, -0.18f}}),
				 rgb(0xff8c2e));
			fill(pts({{0.02f, -0.28f}, {0.08f, -0.18f}, {0.02f, -0.12f}, {-0.04f, -0.18f}}),
				 rgb(0xffd98a));
			break;
		case sim::ItemId::BuildingPlan:
			// A rolled blueprint with a framing square on it.
			edged(pts({{-0.28f, -0.24f}, {0.24f, -0.24f}, {0.24f, 0.28f}, {-0.28f, 0.28f}}),
				  rgb(0xf2efe6), rgb(0xb9b3a2));
			stroke(-0.17f, -0.13f, 0.13f, -0.13f, 0.045f, rgb(0x0a6eeb));
			stroke(0.13f, -0.13f, 0.13f, 0.17f, 0.045f, rgb(0x0a6eeb));
			stroke(0.13f, 0.17f, -0.17f, 0.17f, 0.045f, rgb(0x0a6eeb));
			stroke(-0.17f, 0.17f, -0.17f, -0.13f, 0.045f, rgb(0x0a6eeb));
			stroke(-0.17f, 0.02f, 0.13f, 0.02f, 0.045f, rgb(0x0a6eeb));
			break;

		// --------------------------------------------------------- weapons
		case sim::ItemId::Spear:
			haft(-0.24f, 0.32f, 0.14f, -0.14f, 0.055f);
			edged(pts({{0.14f, -0.14f}, {0.3f, -0.34f}, {0.24f, -0.06f}}), rgb(0xcfd8e0),
				  rgb(0x8f9aa4));
			break;
		case sim::ItemId::Bow:
			arc(0.06f, 0, 0.3f, 3.14159265f * 0.62f, 3.14159265f * 1.38f, 0.075f, kWood);
			detail(-0.11f, -0.28f, -0.11f, 0.28f, 0.028f);
			break;
		case sim::ItemId::Arrow:
			// A stone-tipped arrow, point to the north east. Mostly stick: the
			// head is about a fifth of its length and no wider than a thumb.
			// It was drawn as a spearhead on a stub, which is what made it read
			// as a spear. No binding round the joint: a band of cord across the
			// shaft at this size is a lump, not a lashing.
			stroke(-0.36f, 0.36f, 0.25f, -0.25f, 0.042f, kWood);
			// The fork at the back, where the string sits: two prongs with the
			// notch open between them, not a solid wedge.
			fill(pts({{-0.46f, 0.31f}, {-0.34f, 0.3f}, {-0.31f, 0.34f}, {-0.41f, 0.37f}}),
				 rgb(0xb08a55));
			fill(pts({{-0.31f, 0.46f}, {-0.3f, 0.34f}, {-0.34f, 0.31f}, {-0.37f, 0.41f}}),
				 rgb(0xb08a55));
			// The knapped head: narrow, barbed at the shoulders, long point.
			edged(pts({{0.45f, -0.45f},
					   {0.27f, -0.32f},
					   {0.21f, -0.34f},
					   {0.24f, -0.26f},
					   {0.26f, -0.24f},
					   {0.34f, -0.21f},
					   {0.32f, -0.27f}}),
				  rgb(0xb8b2a6), rgb(0x7f7a70));
			// One flake off the face of it, which is what says knapped stone.
			detail(0.38f, -0.38f, 0.28f, -0.29f, 0.018f);
			break;
		case sim::ItemId::Revolver:
			box(-0.1f, -0.12f, 0.38f, 0.11f, rgb(0x4a4a52));
			disc(-0.04f, -0.02f, 0.11f, rgb(0x6a6a74));
			plainDisc(-0.04f, -0.02f, 0.045f, rgb(0x3a3a42));
			edged(pts({{-0.14f, -0.02f}, {-0.04f, -0.02f}, {-0.1f, 0.28f}, {-0.24f, 0.26f}}),
				  kWood, kWoodDark);
			arc(-0.02f, 0.12f, 0.08f, 3.14159265f * 0.1f, 3.14159265f * 0.9f, 0.035f,
				rgb(0x4a4a52));
			break;
		case sim::ItemId::Rifle:
			box(-0.34f, -0.06f, 0.66f, 0.09f, rgb(0x4a4a52));
			box(0.1f, -0.1f, 0.24f, 0.05f, rgb(0x6a6a74));
			edged(pts({{-0.34f, -0.06f}, {-0.2f, -0.06f}, {-0.16f, 0.16f}, {-0.34f, 0.2f}}), kWood,
				  kWoodDark);
			// The magazine, which is gone while it is being changed.
			if (!magOut) box(-0.06f, 0.03f, 0.06f, 0.16f, rgb(0x4a4a52));
			box(-0.14f, 0.14f, 0.1f, 0.16f, rgb(0x3a3a42));
			break;
		case sim::ItemId::Ak47:
			// An AK in profile, barrel to the right: wooden stock, dark
			// receiver, the curved magazine that makes it an AK, the wooden
			// handguard, a gas tube over the barrel and the front sight post.
			edged(pts({{-0.44f, -0.07f}, {-0.2f, -0.05f}, {-0.2f, 0.03f}, {-0.42f, 0.12f}}),
				  rgb(0x9a5a2e), kWoodDark);
			box(-0.2f, -0.08f, 0.26f, 0.11f, rgb(0x3c3c44));
			// The curved magazine, which is gone while it is being changed.
			if (!magOut) {
				edged(pts({{-0.02f, 0.03f}, {0.06f, 0.03f}, {0.1f, 0.3f}, {0.02f, 0.32f}}),
					  rgb(0x35353c), rgb(0x1f1f24));
			}
			edged(pts({{-0.16f, 0.03f}, {-0.08f, 0.03f}, {-0.12f, 0.18f}, {-0.19f, 0.17f}}),
				  rgb(0x6e4122), kWoodDark);
			box(0.06f, -0.07f, 0.18f, 0.09f, rgb(0xb0703a));
			box(0.06f, -0.11f, 0.26f, 0.035f, rgb(0x4a4a52));
			box(0.24f, -0.05f, 0.22f, 0.035f, rgb(0x4a4a52));
			box(0.4f, -0.11f, 0.025f, 0.07f, rgb(0x4a4a52));
			break;
		case sim::ItemId::Waterpipe:
			// A length of water pipe lashed onto a rough wooden stock.
			edged(pts({{-0.44f, -0.04f}, {-0.08f, -0.04f}, {-0.06f, 0.06f}, {-0.42f, 0.16f}}),
				  kWood, kWoodDark);
			box(-0.12f, -0.1f, 0.56f, 0.1f, rgb(0x8a8f94));
			box(0.4f, -0.12f, 0.05f, 0.14f, rgb(0x6f767d));
			box(-0.02f, -0.12f, 0.08f, 0.14f, rgb(0xc8b89a));
			box(0.2f, -0.12f, 0.06f, 0.14f, rgb(0xc8b89a));
			box(-0.14f, 0.0f, 0.05f, 0.12f, rgb(0x3a3a42));
			break;
		case sim::ItemId::PumpShotgun:
			edged(pts({{-0.45f, -0.06f}, {-0.2f, -0.05f}, {-0.2f, 0.04f}, {-0.43f, 0.14f}}),
				  rgb(0x5a4a3a), kWoodDark);
			box(-0.2f, -0.08f, 0.2f, 0.12f, rgb(0x3c3c44));
			box(0.0f, -0.08f, 0.44f, 0.06f, rgb(0x4a4a52));
			box(0.0f, -0.01f, 0.4f, 0.05f, rgb(0x35353c));
			box(0.12f, -0.03f, 0.16f, 0.09f, kWood);
			box(-0.15f, 0.04f, 0.05f, 0.12f, rgb(0x3a3a42));
			break;
		case sim::ItemId::RocketLauncher:
			// The tube, a flared rear end, a grip and guard, a sight on top.
			box(-0.42f, -0.1f, 0.84f, 0.16f, rgb(0x5f6b4a));
			edged(pts({{-0.42f, -0.13f}, {-0.3f, -0.1f}, {-0.3f, 0.06f}, {-0.42f, 0.09f}}),
				  rgb(0x4a5439), rgb(0x2f3624));
			box(0.36f, -0.12f, 0.07f, 0.2f, rgb(0x3c3c44));
			box(-0.04f, 0.06f, 0.08f, 0.2f, rgb(0x3c3c44));
			box(0.12f, 0.06f, 0.07f, 0.14f, rgb(0x3c3c44));
			box(-0.1f, -0.2f, 0.14f, 0.1f, rgb(0x3c3c44));
			break;
		case sim::ItemId::Rocket:
			// Nose to the right: red warhead, grey body, tail fins.
			box(-0.24f, -0.07f, 0.42f, 0.14f, rgb(0x9aa2aa));
			edged(pts({{0.18f, -0.07f}, {0.36f, 0}, {0.18f, 0.07f}}), rgb(0xc8433a), rgb(0x7a1c1c));
			fill(pts({{-0.24f, -0.07f}, {-0.36f, -0.18f}, {-0.3f, -0.07f}}), rgb(0x6f767d));
			fill(pts({{-0.24f, 0.07f}, {-0.36f, 0.18f}, {-0.3f, 0.07f}}), rgb(0x6f767d));
			detail(0.02f, -0.07f, 0.02f, 0.07f, 0.03f);
			break;

		// ------------------------------------------------------ ammunition
		case sim::ItemId::ShotgunShell:
			// Three shells: red hulls, brass bases.
			for (const float sx : {-0.18f, 0.0f, 0.18f}) {
				box(sx - 0.06f, -0.2f, 0.12f, 0.3f, rgb(0xc8433a));
				box(sx - 0.07f, 0.1f, 0.14f, 0.1f, rgb(0xd8b44a));
			}
			break;
		case sim::ItemId::PistolAmmo:
			for (const float sx : {-0.18f, 0.0f, 0.18f}) {
				box(sx - 0.055f, -0.05f, 0.11f, 0.3f, rgb(0xc9a94a));
				fill(pts({{sx - 0.055f, -0.05f}, {sx, -0.24f}, {sx + 0.055f, -0.05f}}),
					 rgb(0xd8c08a));
			}
			break;
		case sim::ItemId::RifleAmmo:
			for (const float sx : {-0.16f, 0.06f}) {
				box(sx - 0.06f, -0.1f, 0.12f, 0.36f, rgb(0xb8a04a));
				fill(pts({{sx - 0.06f, -0.1f}, {sx, -0.32f}, {sx + 0.06f, -0.1f}}), rgb(0xcfe2f5));
			}
			break;

		// ----------------------------------------------------- consumables
		case sim::ItemId::Bandage:
			edged(pts({{-0.3f, -0.12f}, {0.3f, -0.24f}, {0.3f, 0.06f}, {-0.3f, 0.18f}}),
				  rgb(0xf0ece2), rgb(0xc3bcae));
			box(-0.07f, -0.19f, 0.14f, 0.055f, rgb(0xc2452f));
			box(-0.0425f, -0.235f, 0.085f, 0.145f, rgb(0xc2452f));
			break;
		case sim::ItemId::Medkit:
			box(-0.07f, -0.3f, 0.14f, 0.4f, rgb(0xe8e8ee));
			box(-0.05f, -0.26f, 0.1f, 0.28f, rgb(0xd84a6a));
			box(-0.03f, 0.1f, 0.06f, 0.14f, rgb(0x9aa2aa));
			detail(0, 0.24f, 0, 0.32f, 0.03f);
			break;

		// -------------------------------------------------------- clothing
		case sim::ItemId::Clothing:
			edged(pts({{-0.3f, -0.14f}, {-0.12f, -0.26f}, {0.12f, -0.26f}, {0.3f, -0.14f},
					   {0.2f, -0.02f}, {0.2f, 0.28f}, {-0.2f, 0.28f}, {-0.2f, -0.02f}}),
				  rgb(0xa3714a), rgb(0x6d4a2c));
			detail(0, -0.26f, 0, 0.28f, 0.03f);
			break;
		case sim::ItemId::MetalSuit:
			// A road sign strapped over a jacket: the sign is the picture.
			edged(pts({{-0.3f, -0.14f}, {-0.12f, -0.26f}, {0.12f, -0.26f}, {0.3f, -0.14f},
					   {0.2f, -0.02f}, {0.2f, 0.28f}, {-0.2f, 0.28f}, {-0.2f, -0.02f}}),
				  rgb(0x7a6a58), rgb(0x4a4038));
			edged(pts({{0, -0.16f}, {0.2f, 0.04f}, {0, 0.24f}, {-0.2f, 0.04f}}), rgb(0xd8d2c4),
				  rgb(0x8f8a7e));
			// The sign is bare metal, with the bolts that hold it on. It had a
			// red diamond inside it, which at belt size was a red square in the
			// middle of the suit and read as a fault in the picture.
			plainDisc(0, -0.08f, 0.022f, rgb(0x8f8a7e));
			plainDisc(0, 0.16f, 0.022f, rgb(0x8f8a7e));
			plainDisc(-0.12f, 0.04f, 0.022f, rgb(0x8f8a7e));
			plainDisc(0.12f, 0.04f, 0.022f, rgb(0x8f8a7e));
			break;
		case sim::ItemId::HeavyMetalSuit:
			// Full plate: a chest piece with a mask over it and rivets down it.
			edged(pts({{-0.3f, -0.1f}, {-0.12f, -0.22f}, {0.12f, -0.22f}, {0.3f, -0.1f},
					   {0.22f, 0.02f}, {0.22f, 0.28f}, {-0.22f, 0.28f}, {-0.22f, 0.02f}}),
				  rgb(0xa9b6c0), rgb(0x5d6a74));
			edged(pts({{-0.15f, -0.3f}, {0.15f, -0.3f}, {0.15f, -0.08f}, {0, 0.0f},
					   {-0.15f, -0.08f}}),
				  rgb(0x8d9aa4), rgb(0x4a5560));
			// A slit, in the same steel as the rest: a black rectangle read as
			// a hole punched in the icon.
			box(-0.1f, -0.235f, 0.2f, 0.035f, rgb(0x6e7d88));
			plainDisc(-0.16f, 0.1f, 0.03f, rgb(0x5d6a74));
			plainDisc(0.16f, 0.1f, 0.03f, rgb(0x5d6a74));
			plainDisc(-0.16f, 0.21f, 0.03f, rgb(0x5d6a74));
			plainDisc(0.16f, 0.21f, 0.03f, rgb(0x5d6a74));
			break;
		case sim::ItemId::Hazmat:
			edged(pts({{-0.28f, -0.1f}, {-0.1f, -0.26f}, {0.1f, -0.26f}, {0.28f, -0.1f},
					   {0.18f, 0.0f}, {0.18f, 0.28f}, {-0.18f, 0.28f}, {-0.18f, 0.0f}}),
				  rgb(0x5ac8a0), rgb(0x2f8a6a));
			disc(0, -0.14f, 0.11f, rgb(0x2f3a3a));
			plainDisc(-0.03f, -0.17f, 0.04f, rgb(0x8fe8c8));
			break;

		// ----------------------------------------------------- deployables
		case sim::ItemId::Campfire:
			stroke(-0.26f, 0.24f, 0.26f, 0.12f, 0.07f, kWood);
			stroke(0.26f, 0.24f, -0.26f, 0.12f, 0.07f, kWood);
			fill(pts({{0, -0.3f}, {0.16f, -0.02f}, {0.1f, 0.12f}, {-0.1f, 0.12f}, {-0.16f, -0.02f}}),
				 rgb(0xff8c2e));
			fill(pts({{0, -0.14f}, {0.08f, 0.0f}, {0, 0.1f}, {-0.08f, 0.0f}}), rgb(0xffd98a));
			break;
		case sim::ItemId::Furnace:
			edged(pts({{-0.28f, 0.3f}, {-0.2f, -0.24f}, {0.2f, -0.24f}, {0.28f, 0.3f}}),
				  rgb(0x6b5540), rgb(0x3f3226));
			box(-0.12f, 0.04f, 0.24f, 0.26f, rgb(0x241f18));
			fill(pts({{0, 0.1f}, {0.08f, 0.2f}, {0, 0.28f}, {-0.08f, 0.2f}}), rgb(0xff8c2e));
			box(-0.2f, -0.3f, 0.4f, 0.07f, rgb(0x4f4238));
			break;
		case sim::ItemId::ToolCupboard:
			box(-0.24f, -0.26f, 0.48f, 0.54f, rgb(0x7a6034));
			box(-0.24f, -0.26f, 0.48f, 0.12f, rgb(0xc9a227));
			plainDisc(0.13f, 0.06f, 0.035f, rgb(0xc9a227));
			break;
		case sim::ItemId::WoodenBox:
			box(-0.28f, -0.2f, 0.56f, 0.44f, kWood);
			detail(-0.28f, -0.2f, 0.28f, 0.24f, 0.04f);
			detail(0.28f, -0.2f, -0.28f, 0.24f, 0.04f);
			break;
		case sim::ItemId::SleepingBag:
			edged(pts({{-0.3f, -0.16f}, {0.3f, -0.16f}, {0.3f, 0.18f}, {-0.3f, 0.18f}}),
				  rgb(0xa05a5a), rgb(0x6d3838));
			box(-0.26f, -0.12f, 0.2f, 0.26f, rgb(0xc98a8a));
			detail(0.06f, -0.16f, 0.06f, 0.18f, 0.03f);
			break;
		case sim::ItemId::Lock:
			arc(0, -0.08f, 0.14f, 3.14159265f, 6.28318531f, 0.06f, rgb(0x9aa2aa));
			box(-0.2f, -0.08f, 0.4f, 0.32f, rgb(0xd8c08a));
			plainDisc(0, 0.08f, 0.05f, rgb(0x5f5340));
			break;
		case sim::ItemId::Workbench1:
		case sim::ItemId::Workbench2:
		case sim::ItemId::Workbench3: {
			// A bench top on two legs, with a tally of its level on the front.
			box(-0.32f, -0.16f, 0.64f, 0.12f, kWood);
			box(-0.26f, -0.04f, 0.08f, 0.3f, kWoodDark);
			box(0.18f, -0.04f, 0.08f, 0.3f, kWoodDark);
			const int level = item == sim::ItemId::Workbench1   ? 1
							  : item == sim::ItemId::Workbench2 ? 2
																: 3;
			for (int i = 0; i < level; ++i) {
				box(-0.1f + i * 0.09f, 0.04f, 0.05f, 0.16f, rgb(0xc9a227));
			}
			break;
		}

		// ------------------------------------------------------ explosives
		case sim::ItemId::Satchel:
			edged(pts({{-0.26f, -0.12f}, {0.26f, -0.12f}, {0.22f, 0.26f}, {-0.22f, 0.26f}}),
				  rgb(0xc9702e), rgb(0x8a4a1c));
			box(-0.26f, -0.12f, 0.52f, 0.08f, rgb(0x8a4a1c));
			detail(0.06f, -0.12f, 0.16f, -0.26f, 0.035f);
			detail(0.16f, -0.26f, 0.06f, -0.34f, 0.035f);
			break;
		case sim::ItemId::C4:
			edged(pts({{-0.28f, -0.1f}, {0.28f, -0.1f}, {0.28f, 0.24f}, {-0.28f, 0.24f}}),
				  rgb(0xd8d0c0), rgb(0x9a9384));
			box(-0.28f, 0.0f, 0.56f, 0.08f, rgb(0xc2452f));
			box(-0.06f, -0.28f, 0.12f, 0.18f, rgb(0x4a4a52));
			plainDisc(0, -0.3f, 0.05f, rgb(0xc2452f));
			break;

		default: disc(0, 0, 0.3f, rgb(0x9aa2aa)); break;
	}
}

}  // namespace client
