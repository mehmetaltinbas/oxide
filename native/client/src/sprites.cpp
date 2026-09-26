#include "sprites.hpp"

#include <algorithm>
#include <cmath>

#include "palette.hpp"
#include "sim/rng.hpp"

namespace client {

namespace {

constexpr float kTau = 6.28318530718f;
constexpr float kPi = 3.14159265359f;

Color darken(Color c, float amount = 0.72f) {
	return Color{static_cast<std::uint8_t>(c.r * amount), static_cast<std::uint8_t>(c.g * amount),
				 static_cast<std::uint8_t>(c.b * amount), c.a};
}

float v(int variant, int slot) {
	return static_cast<float>(sim::seeded(static_cast<std::uint64_t>(variant) * 7919 + 13, slot));
}

/**
 * How much bigger the baked drawing is than the thing it stands for.
 *
 * A sprite is drawn at a fixed radius and scaled down to whatever the node's
 * own radius is, so a pen laid on at its world weight comes out a third of it.
 * Every width and spacing below is multiplied by this, which is what makes the
 * ink land on the screen at the weight the TypeScript game inks at.
 */
float gInkScale = 1;

/** The pen, at the weight trees and ore are inked with. */
inline float pen() { return kInkWidth * kNodeLineScale * gInkScale; }
inline float mark() { return kInkMark * kNodeLineScale * gInkScale; }
inline float fine() { return kInkFine * kNodeLineScale * gInkScale; }
inline float hatch() { return kInkHatch * kNodeLineScale * gInkScale; }
inline float hatchStep() { return kHatchSpacing * gInkScale; }
inline float halftoneStep() { return kHalftoneSpacing * gInkScale; }
inline float stippleDot() { return kStippleDot * gInkScale; }

/** Whether a point falls inside a polygon: the clip, done by hand. */
bool insidePoly(const std::vector<Point>& pts, float x, float y) {
	bool in = false;
	for (std::size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
		if ((pts[i].y > y) != (pts[j].y > y) &&
			x < (pts[j].x - pts[i].x) * (y - pts[i].y) / (pts[j].y - pts[i].y) + pts[i].x) {
			in = !in;
		}
	}
	return in;
}

/**
 * A line cut back until it lies inside a shape.
 *
 * SDL has no clip path, so a mark that would run past the edge of a rock or a
 * branch is walked back towards its start until its far end is inside again.
 */
void markInside(Paint& paint, const std::vector<Point>& shape, float x0, float y0, float x1,
				float y1, float width, Color color) {
	// Both ends, because a hatching stroke starts below the shape as often as
	// it ends above it.
	const float mx = (x0 + x1) * 0.5f;
	const float my = (y0 + y1) * 0.5f;
	if (!insidePoly(shape, mx, my)) return;
	for (int i = 0; i < 12 && !insidePoly(shape, x0, y0); ++i) {
		x0 = x0 * 0.86f + mx * 0.14f;
		y0 = y0 * 0.86f + my * 0.14f;
	}
	for (int i = 0; i < 12 && !insidePoly(shape, x1, y1); ++i) {
		x1 = x1 * 0.86f + mx * 0.14f;
		y1 = y1 * 0.86f + my * 0.14f;
	}
	paint.line(x0, y0, x1, y1, width, color);
}

/** The quadratic curve a scallop is drawn with, as points. */
void quadTo(std::vector<Point>& out, Point from, Point control, Point to, int steps = 6) {
	for (int i = 1; i <= steps; ++i) {
		const float t = static_cast<float>(i) / steps;
		const float u = 1 - t;
		out.push_back({u * u * from.x + 2 * u * t * control.x + t * t * to.x,
					   u * u * from.y + 2 * u * t * control.y + t * t * to.y});
	}
}

/**
 * A band with a flat top and a wavy bottom, filled as a run of quads.
 *
 * The fan a polygon fill uses cannot carry a scalloped edge: a sag between two
 * teeth folds back over the middle. A strip of quads has no such trouble.
 */
void fillBand(Paint& paint, float top, const std::vector<Point>& bottom, Color color) {
	for (std::size_t i = 0; i + 1 < bottom.size(); ++i) {
		paint.fillPoly({{bottom[i].x, top},
						{bottom[i + 1].x, top},
						{bottom[i + 1].x, bottom[i + 1].y},
						{bottom[i].x, bottom[i].y}},
					   color);
	}
}

/**
 * The ink inside one tier of a conifer.
 *
 * Not an outline: the pen already drew that. These are the marks that say what
 * the shape is made of, which for a pine is the fishbone of its branches and
 * hatching down the side the light is not on.
 */
void markTier(Paint& paint, float cx, float y, float r, int variant) {
	const float apexY = y - r;
	const float baseY = y + r * 0.5f;
	const float height = baseY - apexY;
	const std::vector<Point> tier{{cx, apexY}, {cx - r, baseY}, {cx + r, baseY}};

	// Branches, two rows, the lower one wider: a tier spreads as it drops.
	for (const float t : {0.42f, 0.7f}) {
		const float py = apexY + height * t;
		const float reach = r * t * 0.62f;
		const float drop = reach * 0.42f;
		const float lean = static_cast<float>((variant % 3) - 1) * 0.6f;
		markInside(paint, tier, cx + lean, py, cx - reach + lean, py + drop, mark(), kInk);
		markInside(paint, tier, cx + lean, py, cx + reach + lean, py + drop, mark(), kInk);
	}

	// Shade down the right flank: hatching is how a press says the light comes
	// from the other side.
	const float step = hatchStep() * 0.75f;
	for (float hx = r * 0.18f; hx < r; hx += step) {
		markInside(paint, tier, cx + hx, baseY, cx + hx + height * 0.45f,
				   apexY + height * 0.25f, hatch(), kInk);
	}
}

/**
 * Snow lying on one tier of a conifer: a cap over the top with a scalloped
 * lower edge, the way snow hangs on branches, and clumps caught lower down.
 */
void snowTier(Paint& paint, float cx, float y, float r, int variant, bool crown, float shelf) {
	const float apexY = y - r;
	const float baseY = y + r * 0.5f;
	const float top = crown ? apexY - 1 : shelf - 2;
	const float bottom = crown ? apexY + (baseY - apexY) * 0.42f
							   : shelf + (baseY - shelf) * 0.35f;
	// How wide the tier is at a given height, so the cap stops where it does.
	const auto halfAt = [&](float at) {
		return std::max(0.0f, r * ((at - apexY) / (baseY - apexY)));
	};
	const float halfTop = halfAt(top);
	const float halfBottom = halfAt(bottom);

	std::vector<Point> edge{{cx - halfBottom, bottom}};
	const int scallops = crown ? 3 : 5;
	for (int k = 0; k < scallops; ++k) {
		const float x0 = cx - halfBottom + (2 * halfBottom / scallops) * k;
		const float x1 = cx - halfBottom + (2 * halfBottom / scallops) * (k + 1);
		const float sag = r * (0.1f + ((variant + k) % 3) * 0.03f);
		quadTo(edge, {x0, bottom}, {(x0 + x1) / 2, bottom + sag}, {x1, bottom});
	}
	// Kept off the tier's own edge, so the cap cannot hang in the air beside it.
	for (Point& p : edge) p.x = std::clamp(p.x, cx - halfTop - r, cx + halfTop + r);
	fillBand(paint, top, edge, kSnow);
	paint.outlinePoly(edge, fine(), kInk, false);

	// Clumps caught on the branches lower down, placed by the tree's own seed.
	const std::vector<Point> tier{{cx, apexY}, {cx - r, baseY}, {cx + r, baseY}};
	const int clumps = 2 + static_cast<int>(v(variant, 20) * 3);
	for (int k = 0; k < clumps; ++k) {
		const float t = 0.45f + v(variant, 21 + k) * 0.5f;
		const float cy = apexY + (baseY - apexY) * t;
		const float half = r * t * 0.85f;
		const float x = cx + (v(variant, 31 + k) * 2 - 1) * half;
		const float w = r * (0.12f + v(variant, 41 + k) * 0.12f);
		if (!insidePoly(tier, x, cy)) continue;
		paint.inkedPoly(ellipsePoints(x, cy, w, w * 0.45f), kSnow, fine());
	}
}

/** A pine: three tiers, the crown in front, leaning its own way. */
void paintPine(Paint& paint, float ox, float oy, float r, int variant, bool snowy) {
	const float wide = 0.88f + v(variant, 1) * 0.2f;
	const float tall = 0.9f + v(variant, 2) * 0.18f;
	const float lean = (v(variant, 3) - 0.5f) * r * 0.16f;
	const float trunk = 0.8f + v(variant, 4) * 0.4f;

	paint.fillRect(ox - r * 0.22f * trunk, oy - r * 0.5f, r * 0.44f * trunk, r, rgb(0x7b4a26));
	// Grain up the trunk, so it is timber rather than a brown bar.
	paint.line(ox - r * 0.08f * trunk, oy - r * 0.42f, ox - r * 0.08f * trunk, oy + r * 0.42f,
			   mark(), kInk);
	paint.line(ox + r * 0.1f * trunk, oy - r * 0.3f, ox + r * 0.1f * trunk, oy + r * 0.36f,
			   mark(), kInk);

	struct Tier {
		float y;
		float r;
		float dx;
	};
	const Tier tiers[3] = {
		{oy - r * 2.0f * tall, r * 1.0f * wide * (0.92f + v(variant, 5) * 0.16f), lean * 2},
		{oy - r * 1.35f * tall, r * 1.3f * wide * (0.92f + v(variant, 6) * 0.16f), lean},
		{oy - r * 0.65f * tall, r * 1.55f * wide * (0.94f + v(variant, 7) * 0.1f), 0},
	};
	// Bottom tier first, crown last: a conifer's top sits in front of the skirt
	// below it, and painting downward buried every crown.
	for (int i = 2; i >= 0; --i) {
		const Tier& t = tiers[i];
		const float x = ox + t.dx;
		paint.inkedPoly({{x, t.y - t.r}, {x - t.r, t.y + t.r * 0.5f}, {x + t.r, t.y + t.r * 0.5f}},
						i % 2 == 0 ? nodeColor(sim::NodeKind::Tree) : rgb(0x5cc063), pen());
		markTier(paint, x, t.y, t.r, variant + i);
		if (snowy) {
			const float shelf = i > 0 ? tiers[i - 1].y + tiers[i - 1].r * 0.5f : 0;
			snowTier(paint, x, t.y, t.r, variant + i, i == 0, shelf);
		}
	}
}

/**
 * A broadleaf, for the open grassland: a short trunk and a round crown built of
 * overlapping leaf clumps in two greens, with the scalloped edge of each clump
 * inked inside the crown and hatching down the shaded side.
 */
void paintBroadleaf(Paint& paint, float ox, float oy, float r, int variant) {
	const float size = 0.9f + v(variant, 1) * 0.18f;
	const float lean = (v(variant, 2) - 0.5f) * r * 0.2f;
	const float cx = ox + lean;
	const float cy = oy - r * 1.85f * size;
	const float cr = r * 1.3f * size;

	paint.inkedPoly({{ox - r * 0.2f, oy + r * 0.4f},
					 {ox + r * 0.2f, oy + r * 0.4f},
					 {cx + r * 0.14f, cy + cr * 0.4f},
					 {cx - r * 0.14f, cy + cr * 0.4f}},
					rgb(0x7b4a26), pen());

	const int clumps = 6 + static_cast<int>(v(variant, 3) * 3);
	struct Clump {
		float x;
		float y;
		float r;
	};
	std::vector<Clump> pts;
	for (int i = 0; i < clumps; ++i) {
		const float a = static_cast<float>(i) / clumps * kTau + v(variant, 4) * kTau +
						(v(variant, 50 + i) - 0.5f) * 0.6f;
		const float d = cr * (0.4f + v(variant, 10 + i) * 0.4f);
		const float rr = cr * (0.3f + v(variant, 20 + i) * 0.25f);
		pts.push_back({cx + std::cos(a) * d * 1.1f, cy + std::sin(a) * d * 0.78f, rr});
	}
	// A couple of stray clumps out at the edge, so the crown is a canopy and
	// not a ball.
	for (int i = 0; i < 2; ++i) {
		const float a = v(variant, 60 + i) * kTau;
		pts.push_back({cx + std::cos(a) * cr * 0.95f, cy + std::sin(a) * cr * 0.7f,
					   cr * (0.2f + v(variant, 62 + i) * 0.1f)});
	}
	pts.push_back({cx, cy, cr * 0.55f});

	// The pen round the silhouette only: every clump inked heavily first, then
	// the fills laid over the top, which buries the inner half of each line.
	for (const Clump& c : pts) paint.fillCircle(c.x, c.y, c.r + pen(), kInk);
	for (const Clump& c : pts) paint.fillCircle(c.x, c.y, c.r, kBroadleafDark);
	// The lit clumps, up and to the left, in the lighter green.
	for (const Clump& c : pts) {
		if (c.x - cx + (c.y - cy) > cr * 0.25f) continue;
		paint.fillCircle(c.x - c.r * 0.12f, c.y - c.r * 0.12f, c.r * 0.82f, kBroadleafLight);
	}
	// The crown's own edge, for the marks inside it to be cut against.
	std::vector<Point> crown;
	for (int i = 0; i < 28; ++i) {
		const float a = static_cast<float>(i) / 28 * kTau;
		float reach = 0;
		for (const Clump& c : pts) {
			// How far this clump carries the crown out along this bearing.
			const float dx = c.x - cx;
			const float dy = c.y - cy;
			const float along = std::cos(a) * dx + std::sin(a) * dy;
			const float off = std::abs(-std::sin(a) * dx + std::cos(a) * dy);
			if (off >= c.r) continue;
			reach = std::max(reach, along + std::sqrt(c.r * c.r - off * off));
		}
		crown.push_back({cx + std::cos(a) * reach, cy + std::sin(a) * reach});
	}

	// A few branches showing between the leaves.
	for (int k = 0; k < 3; ++k) {
		const float a = -kPi / 2 + (v(variant, 70 + k) - 0.5f) * 2.2f;
		paint.line(cx, cy + cr * 0.35f, cx + std::cos(a) * cr * 0.55f,
				   cy + cr * 0.35f + std::sin(a) * cr * 0.5f, r * 0.12f, rgb(0x5a3a20));
	}
	// The underside of every other clump, as a light mark, so the crown stays
	// leafy rather than lumpy.
	for (std::size_t k = 0; k < pts.size(); k += 2) {
		const Clump& c = pts[k];
		std::vector<Point> arc;
		for (int i = 0; i <= 8; ++i) {
			const float a = kPi * 0.25f + (kPi * 0.5f) * static_cast<float>(i) / 8;
			arc.push_back({c.x + std::cos(a) * c.r * 0.78f, c.y + std::sin(a) * c.r * 0.78f});
		}
		for (std::size_t i = 0; i + 1 < arc.size(); ++i) {
			markInside(paint, crown, arc[i].x, arc[i].y, arc[i + 1].x, arc[i + 1].y, fine(), kInk);
		}
	}
	// Hatching down the shaded side.
	const float step = hatchStep() * 0.75f;
	for (float hx = cx + cr * 0.25f; hx < cx + cr * 1.2f; hx += step) {
		markInside(paint, crown, hx, cy + cr, hx + cr * 0.6f, cy - cr * 0.1f, hatch(), kInk);
	}
}

/**
 * The inside of a boulder: a highlight, stipple down the shaded side, the
 * facets, and for ore the flecks of what it is worth. Everything cut to the
 * rock's own outline, so no mark lands beside the rock it belongs to.
 */
void markStone(Paint& paint, const std::vector<Point>& pts, float ox, float oy, float r,
			   sim::NodeKind kind, int variant) {
	// The lit face. Light, not an object, so it has no line round it.
	paint.fillPoly(ellipsePoints(ox - r * 0.2f, oy - r * 0.25f, r * 0.4f, r * 0.22f, -0.4f),
				   Color{255, 255, 255, 40});

	// Stipple down the side away from the light, which is how a press shades.
	const float step = halftoneStep() * 0.55f;
	for (float sy = -r; sy < r; sy += step) {
		for (float sx = -r; sx < r; sx += step) {
			const float shade = (sx + sy) / (r * 2);
			if (shade < 0.12f) continue;
			const float off = (static_cast<int>((sy + r) / step) % 2) * step / 2;
			if (!insidePoly(pts, ox + sx + off, oy + sy)) continue;
			paint.fillCircle(ox + sx + off, oy + sy, stippleDot() * (0.6f + shade * 0.9f), kInk);
		}
	}

	// Ore: flecks of the metal or the sulfur, chipped rather than round.
	if (kind == sim::NodeKind::Metal || kind == sim::NodeKind::Sulfur) {
		const Color fleck = kind == sim::NodeKind::Metal ? rgb(0xe0d0b0) : rgb(0xf4ec9a);
		for (int i = 0; i < 4; ++i) {
			const float a = (((variant + i * 23) % 100) / 100.0f) * kTau;
			const float fx = ox + std::cos(a) * r * 0.42f;
			const float fy = oy + std::sin(a) * r * 0.32f;
			const float fr = r * 0.13f;
			paint.fillPoly({{fx - fr, fy - fr * 0.2f},
							{fx - fr * 0.1f, fy - fr},
							{fx + fr, fy - fr * 0.1f},
							{fx + fr * 0.2f, fy + fr}},
						   fleck);
		}
	}

	// Facets: from a ridge near the top out to every other corner, drawn with
	// the pen the outline was drawn with. A comic has one weight on a rock.
	const float ridgeX = ox - r * 0.12f;
	const float ridgeY = oy - r * 0.14f;
	for (std::size_t i = variant % 2; i < pts.size(); i += 2) {
		markInside(paint, pts, ridgeX, ridgeY, pts[i].x, pts[i].y, pen(), kInk);
	}
}

/** A drift of snow across the top of a boulder, cut to the rock's own edge. */
void snowStone(Paint& paint, const std::vector<Point>& pts, float ox, float oy, float r,
			   int variant) {
	// How far down the drift comes differs rock to rock.
	const float line = oy - r * (0.35f - v(variant, 1) * 0.4f);
	std::vector<Point> edge{{ox - r * 1.3f, line}};
	const int waves = 4;
	for (int k = 0; k < waves; ++k) {
		const float x0 = ox - r * 1.3f + (2.6f * r / waves) * k;
		const float x1 = ox - r * 1.3f + (2.6f * r / waves) * (k + 1);
		const float dip = r * (0.12f + v(variant, 2 + k) * 0.18f);
		quadTo(edge, {x0, line}, {(x0 + x1) / 2, line + dip}, {x1, line});
	}
	// Cut to the rock: at each step the band runs only as wide as the stone is.
	for (std::size_t i = 0; i + 1 < edge.size(); ++i) {
		const float mid = (edge[i].x + edge[i + 1].x) * 0.5f;
		if (!insidePoly(pts, mid, oy - r * 1.29f) && !insidePoly(pts, mid, edge[i].y)) continue;
		float top = oy - r * 1.3f;
		float bottom = edge[i].y;
		// Walk the bottom up until it is on the stone.
		for (int k = 0; k < 10 && !insidePoly(pts, mid, bottom); ++k) bottom -= r * 0.08f;
		if (bottom <= top) continue;
		// And the top down.
		for (int k = 0; k < 20 && !insidePoly(pts, mid, top); ++k) top += r * 0.08f;
		if (top >= bottom) continue;
		paint.fillPoly({{edge[i].x, top},
						{edge[i + 1].x, top},
						{edge[i + 1].x, edge[i + 1].y},
						{edge[i].x, bottom}},
					   kSnow);
	}
	// Patches lying in the hollows and ledges lower down.
	const int patches = 2 + static_cast<int>(v(variant, 10) * 3);
	for (int k = 0; k < patches; ++k) {
		const float px = ox + (v(variant, 11 + k) * 2 - 1) * r * 0.7f;
		const float py = oy + r * (0.05f + v(variant, 21 + k) * 0.55f);
		const float w = r * (0.14f + v(variant, 31 + k) * 0.14f);
		if (!insidePoly(pts, px, py)) continue;
		paint.fillPoly(ellipsePoints(px, py, w, w * 0.5f, (v(variant, 41 + k) - 0.5f) * 0.8f),
					   kSnow);
	}
	// The rock's edge again, over the drift's.
	paint.outlinePoly(pts, pen(), kInk);
}

/** A boulder or an ore node: one lumpy outline with its facets marked inside. */
void paintStone(Paint& paint, float ox, float oy, float r, sim::NodeKind kind, int variant,
				bool snowy) {
	const int points = 7;
	std::vector<Point> pts;
	for (int i = 0; i < points; ++i) {
		const float a = static_cast<float>(i) / points * kTau;
		const float wob = 0.75f + (((variant * (i + 3)) % 37) / 37.0f) * 0.45f;
		pts.push_back({ox + std::cos(a) * r * wob, oy + std::sin(a) * r * 0.78f * wob});
	}
	paint.inkedPoly(pts, nodeColor(kind), pen());
	markStone(paint, pts, ox, oy, r, kind, variant);
	if (snowy) snowStone(paint, pts, ox, oy, r, variant);
}

/**
 * A nettle clump: all leaf, nothing that reads as a flower. Seven long leaves
 * radiating from the middle, each with a sawtooth edge and a vein down it, in
 * two greens so the clump has depth.
 */
void paintNettle(Paint& paint, float ox, float oy, float r, int variant) {
	// Drawn wider than it stands, as a clump spreads past its stem.
	const float spread = r * 1.5f;
	const int count = 7;
	const float turn = (variant % 13) * 0.17f;
	// Back leaves first, darker, then the front ones over them.
	for (int pass = 0; pass < 2; ++pass) {
		for (int i = pass; i < count; i += 2) {
			const float a = static_cast<float>(i) / count * kTau + turn;
			const float len = spread * (pass == 0 ? 0.95f : 0.8f);
			const float ca = std::cos(a);
			const float sa = std::sin(a);
			const float half = len * 0.26f;
			std::vector<Point> leaf;
			const auto put = [&](float along, float across) {
				leaf.push_back({ox + ca * along - sa * across, oy + sa * along + ca * across});
			};
			put(0, 0);
			const int teeth = 5;
			for (int k = 1; k <= teeth; ++k) {
				const float t = static_cast<float>(k) / teeth;
				const float w = half * std::sin(kPi * t);
				put(len * (t - 0.1f), -w * 1.25f);
				put(len * t, -w * 0.8f);
			}
			for (int k = teeth; k >= 1; --k) {
				const float t = static_cast<float>(k) / teeth;
				const float w = half * std::sin(kPi * t);
				put(len * t, w * 0.8f);
				put(len * (t - 0.1f), w * 1.25f);
			}
			const Color leafColor = nodeColor(sim::NodeKind::Nettle);
			paint.fillPoly(leaf, pass == 0 ? darken(leafColor) : leafColor);
			paint.outlinePoly(leaf, kInkWidth * gInkScale, kInk);
		}
	}
	// The veins, one down each leaf, fine: at full weight they turned the clump
	// into a black star.
	for (int i = 0; i < count; ++i) {
		const float a = static_cast<float>(i) / count * kTau + turn;
		const float len = spread * (i % 2 == 0 ? 0.95f : 0.8f);
		paint.line(ox + std::cos(a) * spread * 0.2f, oy + std::sin(a) * spread * 0.2f,
				   ox + std::cos(a) * len * 0.7f, oy + std::sin(a) * len * 0.7f,
				   kInkFine * 0.6f * gInkScale, kInk);
	}
}

/**
 * A steel drum seen from above: the lid, the rolled rim round it, the two bung
 * caps, and rust eating in from the edge. Blue or red by its seed, the two
 * colours road barrels come in.
 */
void paintBarrel(Paint& paint, float ox, float oy, float r, int variant) {
	const Color body = v(variant, 1) < 0.5f ? nodeColor(sim::NodeKind::Barrel) : rgb(0xb0473a);
	paint.inkedCircle(ox, oy, r, body, pen());
	paint.inkedCircle(ox, oy, r * 0.78f, darken(body), pen());
	// Rust, in from the rim: no line round it, because rust is a stain.
	for (int k = 0; k < 3; ++k) {
		const float a = v(variant, 2 + k) * kTau;
		paint.fillPoly(ellipsePoints(ox + std::cos(a) * r * 0.62f, oy + std::sin(a) * r * 0.62f,
									 r * 0.22f, r * 0.12f, a),
					   Color{122, 70, 30, 140});
	}
	// The bung caps.
	paint.inkedCircle(ox + r * 0.38f, oy - r * 0.2f, r * 0.14f, rgb(0xc9ced4), fine());
	paint.inkedCircle(ox - r * 0.42f, oy + r * 0.3f, r * 0.14f, rgb(0xc9ced4), fine());
	// The rolled rim, as a mark just inside the lid's edge.
	paint.outlineCircle(ox, oy, r * 0.9f, fine(), kInk);
}

}  // namespace

Sprites::~Sprites() {
	for (Baked& b : baked_) {
		if (b.texture) SDL_DestroyTexture(b.texture);
	}
}

const Sprites::Baked& Sprites::bake(SpriteKey key) {
	const int index = ((static_cast<int>(key.kind) * kVariants + key.variant) * 2 +
					   (key.snowy ? 1 : 0)) * 2 +
					  (key.broadleaf ? 1 : 0);
	Baked& slot = baked_[index];
	if (slot.texture) return slot;

	const float r = kBakeRadius;
	// The box each kind needs around its foot. Trees are tall and stand on the
	// bottom of theirs; everything else is centred on the ground it sits on.
	const bool tree = key.kind == sim::NodeKind::Tree;
	const float halfWidth = tree ? r * 2.4f : r * 2.2f;
	const float above = tree ? r * 4.0f : r * 1.6f;
	const float below = tree ? r * 1.2f : r * 1.6f;

	const int w = static_cast<int>(halfWidth * 2) + 4;
	const int h = static_cast<int>(above + below) + 4;
	slot.texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
									 SDL_TEXTUREACCESS_TARGET, w, h);
	if (!slot.texture) return slot;
	SDL_SetTextureBlendMode(slot.texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureScaleMode(slot.texture, SDL_SCALEMODE_LINEAR);
	slot.originX = halfWidth + 2;
	slot.originY = above + 2;
	slot.width = static_cast<float>(w);
	slot.height = static_cast<float>(h);

	SDL_Texture* was = SDL_GetRenderTarget(renderer_);
	SDL_SetRenderTarget(renderer_, slot.texture);
	SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
	SDL_RenderClear(renderer_);

	Paint paint(renderer_);
	// The drawing is made at kBakeRadius and shown at the node's own radius, so
	// the ink is laid on that much heavier to land at its world weight.
	gInkScale = kBakeRadius / static_cast<float>(sim::nodeDef(key.kind).radius);
	const float ox = slot.originX;
	const float oy = slot.originY;
	switch (key.kind) {
		case sim::NodeKind::Tree:
			if (key.broadleaf) {
				paintBroadleaf(paint, ox, oy, r, key.variant);
			} else {
				paintPine(paint, ox, oy, r, key.variant, key.snowy);
			}
			break;
		case sim::NodeKind::Stone:
		case sim::NodeKind::Metal:
		case sim::NodeKind::Sulfur:
			paintStone(paint, ox, oy, r, key.kind, key.variant, key.snowy);
			break;
		case sim::NodeKind::Nettle: paintNettle(paint, ox, oy, r, key.variant); break;
		case sim::NodeKind::Barrel: paintBarrel(paint, ox, oy, r, key.variant); break;
	}

	SDL_SetRenderTarget(renderer_, was);
	return slot;
}

void Sprites::draw(const sim::ResourceNode& node, float screenX, float screenY, float scale,
				   bool snowy, bool broadleaf, float alpha) {
	SpriteKey key{node.kind, static_cast<int>(node.seed % kVariants), snowy, broadleaf};
	const Baked& b = bake(key);
	if (!b.texture) return;
	const float factor = static_cast<float>(node.radius) / kBakeRadius * scale;
	SDL_FRect dst{screenX - b.originX * factor, screenY - b.originY * factor, b.width * factor,
				  b.height * factor};
	SDL_SetTextureAlphaModFloat(b.texture, alpha);
	SDL_RenderTexture(renderer_, b.texture, nullptr, &dst);
}

}  // namespace client
