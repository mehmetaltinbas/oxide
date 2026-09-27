#include "client/features/render/systems/sprites.hpp"

#include <algorithm>
#include <cmath>

#include "client/design/tokens/world.tokens.hpp"
#include "sim/shared/utils/rng.util.hpp"
#include "sim/features/world/constants/node-defs.constant.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/point.struct.hpp"
#include "client/features/render/types/sprite-key.struct.hpp"
#include "client/features/render/utils/shape-points.util.hpp"

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
 * A lumpy round mass.
 *
 * A circle drawn with the radius wandering from point to point. Leaves in a
 * clump have no straight edge and no true curve either, and a perfect circle of
 * green reads as a drawn ball rather than as foliage.
 */
std::vector<Point> blob(float cx, float cy, float radius, int variant, int slot, float squash = 1.0f) {
	std::vector<Point> out;
	constexpr int kLobes = 13;
	for (int i = 0; i < kLobes; ++i) {
		const float a = static_cast<float>(i) / kLobes * kTau;
		const float wobble = 0.86f + v(variant, slot + i) * 0.26f;
		out.push_back({cx + std::cos(a) * radius * wobble,
					   cy + std::sin(a) * radius * wobble * squash});
	}
	return out;
}

/**
 * A prickly round mass: a blob with teeth round it.
 *
 * The radius alternates in and out as it goes round, so the edge comes out as a
 * ring of short spikes. This is what a conifer is from above: a dense dark mass
 * with the tips of its branches sticking out of it all the way round, and the
 * teeth are what say needles without drawing a needle.
 */
std::vector<Point> prickle(float cx, float cy, float radius, int teeth, int variant, int slot) {
	std::vector<Point> out;
	for (int i = 0; i < teeth * 2; ++i) {
		const float a = static_cast<float>(i) / (teeth * 2) * kTau;
		const bool tip = (i % 2) == 0;
		const float reach = tip ? 1.0f + v(variant, slot + i) * 0.22f
								: 0.6f + v(variant, slot + i) * 0.12f;
		out.push_back({cx + std::cos(a) * radius * reach,
					   cy + std::sin(a) * radius * reach * 0.94f});
	}
	return out;
}

/**
 * One frond: a long pointed leaf, wide near its base and tapering to a spike,
 * curving a little as it goes so a ring of them does not read as a starfish.
 */
std::vector<Point> frond(float ox, float oy, float angle, float len, float wide, float curve) {
	const float ca = std::cos(angle);
	const float sa = std::sin(angle);
	const auto at = [&](float along, float across) {
		// The curve bends the frond sideways as it runs out.
		const float bend = curve * along * along;
		const float off = across + bend;
		return Point{ox + ca * along - sa * off, oy + sa * along + ca * off};
	};
	std::vector<Point> out;
	// Out along one side, widest a quarter of the way up, then back along the
	// other: the shape of a needle cluster rather than of a leaf.
	constexpr int kSteps = 7;
	for (int i = 0; i <= kSteps; ++i) {
		const float t = static_cast<float>(i) / kSteps;
		const float w = wide * std::sin(std::pow(t, 0.55f) * kPi) * (1 - t * 0.35f);
		out.push_back(at(len * t, -w));
	}
	for (int i = kSteps; i >= 0; --i) {
		const float t = static_cast<float>(i) / kSteps;
		const float w = wide * std::sin(std::pow(t, 0.55f) * kPi) * (1 - t * 0.35f);
		out.push_back(at(len * t, w));
	}
	return out;
}

/**
 * The tree of the forest and the snow.
 *
 * The same fronds as a grassland tree, but four rings of them instead of three,
 * tighter, and in the near-grey green of a spruce: from above a conifer is a
 * cone of needle sprays, widest at the skirt and gathered to a point, and the
 * rings narrowing as they rise are what say cone rather than bush.
 */
void paintPine(Paint& paint, float ox, float oy, float r, int variant, bool snowy) {
	const float size = 0.92f + v(variant, 1) * 0.18f;
	const float lean = (v(variant, 3) - 0.5f) * r * 0.16f;

	// The trunk, thicker than a grassland tree's and barked across.
	const float trunkW = r * 0.23f * (0.85f + v(variant, 4) * 0.3f);
	paint.inkedPoly({{ox - trunkW, oy - r * 0.25f},
					 {ox + trunkW, oy - r * 0.25f},
					 {ox + trunkW * 1.3f, oy + r * 0.66f},
					 {ox - trunkW * 1.3f, oy + r * 0.66f}},
					rgb(0x6a5236), pen());
	for (int i = 0; i < 3; ++i) {
		const float ty = oy + r * (-0.08f + i * 0.22f);
		markInside(paint, {{ox - trunkW * 1.4f, oy - r * 0.35f},
						   {ox + trunkW * 1.4f, oy - r * 0.35f},
						   {ox + trunkW * 1.4f, oy + r * 0.66f},
						   {ox - trunkW * 1.4f, oy + r * 0.66f}},
				   ox - trunkW, ty, ox + trunkW, ty + r * 0.08f, fine(), kInk);
	}

	struct Ring {
		float y;
		float reach;
		Color fill;
		int teeth;
	};
	// Skirt first and crown last, so the top sits in front of what is under it,
	// and each ring narrower than the one below: that is the cone.
	const Ring rings[4] = {
		{oy - r * 0.35f * size, r * 1.42f * size, kPineDark, 13},
		{oy - r * 0.8f * size, r * 1.16f * size, kPineDark, 11},
		{oy - r * 1.25f * size, r * 0.9f * size, kPineMid, 10},
		{oy - r * 1.65f * size, r * 0.6f * size, kPineLight, 8},
	};

	// The shade inside the canopy. Without it the gaps between the rings show
	// the ground through the tree, and separate leaves round a hole is what a
	// nettle looks like.
	paint.fillPoly(blob(ox + lean * 0.2f, rings[0].y + r * 0.1f, rings[0].reach * 0.95f,
						variant, 110),
				   darken(kPineDark, 0.66f));

	for (int t = 0; t < 4; ++t) {
		const Ring& ring = rings[t];
		const float cx = ox + lean * (0.2f + t * 0.35f);
		const std::vector<Point> mass =
			prickle(cx, ring.y, ring.reach, ring.teeth, variant, 20 + t * 29);
		paint.inkedPoly(mass, ring.fill, pen() * 0.7f);

		// The branches under the needles: a few spokes from the trunk out to the
		// teeth, which is the only structure a conifer shows from above.
		const int spokes = ring.teeth / 2;
		for (int i = 0; i < spokes; ++i) {
			const float a = (static_cast<float>(i) / spokes + v(variant, 80 + t * 11) * 0.1f) * kTau;
			markInside(paint, mass, cx + std::cos(a) * ring.reach * 0.15f,
					   ring.y + std::sin(a) * ring.reach * 0.15f,
					   cx + std::cos(a) * ring.reach * 0.86f,
					   ring.y + std::sin(a) * ring.reach * 0.86f, fine(), kInk);
		}

		// Up in the snow it settles on top of the sprays: the same prickly
		// shape, smaller and sat a little high, so the teeth still show through
		// underneath it.
		if (snowy && t >= 1) {
			paint.inkedPoly(prickle(cx, ring.y - ring.reach * 0.1f, ring.reach * 0.6f,
									ring.teeth, variant, 150 + t * 29),
							kSnow, fine());
		}
	}

	if (snowy) {
		// And what has gathered in the crown, which is what you see first from
		// a distance. Not a circle: a drawn circle of white reads as a ball
		// stuck on the tree.
		paint.inkedPoly(blob(ox + lean * 1.4f, rings[3].y, r * 0.24f * size, variant, 170),
						kSnow, pen() * 0.5f);
	}
}

/**
 * The tree of the open grassland.
 *
 * A broadleaf from above is a billowing mass of leaf, not a rosette: clumps of
 * foliage piled on each other, rounded at every edge, lighter where the sun
 * reaches the top of them. Drawn as rings of lumpy blobs rather than as a ring
 * of pointed fronds, which is what had it reading as a nettle with a trunk.
 */
void paintBroadleaf(Paint& paint, float ox, float oy, float r, int variant) {
	const float size = 0.94f + v(variant, 1) * 0.16f;
	const float lean = (v(variant, 2) - 0.5f) * r * 0.22f;

	// The trunk, with the bark marked across it, showing under the skirt.
	const float trunkW = r * 0.19f * (0.85f + v(variant, 9) * 0.3f);
	paint.inkedPoly({{ox - trunkW, oy - r * 0.2f},
					 {ox + trunkW, oy - r * 0.2f},
					 {ox + trunkW * 1.25f, oy + r * 0.62f},
					 {ox - trunkW * 1.25f, oy + r * 0.62f}},
					rgb(0x8a7256), pen());
	for (int i = 0; i < 3; ++i) {
		const float ty = oy + r * (-0.05f + i * 0.2f);
		markInside(paint, {{ox - trunkW * 1.3f, oy - r * 0.3f},
						   {ox + trunkW * 1.3f, oy - r * 0.3f},
						   {ox + trunkW * 1.3f, oy + r * 0.62f},
						   {ox - trunkW * 1.3f, oy + r * 0.62f}},
				   ox - trunkW, ty, ox + trunkW, ty + r * 0.07f, fine(), kInk);
	}

	struct Tier {
		float y;
		float reach;
		Color fill;
		int count;
	};
	// Bottom first and the crown last, so the top sits in front of the skirt
	// below it. Three greens, lightest at the top, which is where the sun is.
	// Kept close together: spread up the screen they stack into a cone, and a
	// broadleaf seen from above is one round mass, not a pine's tiers.
	const Tier tiers[3] = {
		{oy - r * 0.34f * size, r * 1.46f * size, kBroadleafDark, 11},
		{oy - r * 0.7f * size, r * 1.24f * size, kBroadleafMid, 9},
		{oy - r * 1.02f * size, r * 0.94f * size, kBroadleafLight, 7},
	};

	// The shade under the canopy, so the gaps between clumps show dark leaf
	// rather than the grass beneath the tree.
	paint.fillPoly(blob(ox + lean * 0.3f, tiers[0].y, tiers[0].reach * 0.95f, variant, 110),
				   darken(kBroadleafDark, 0.68f));

	for (int t = 0; t < 3; ++t) {
		const Tier& tier = tiers[t];
		const float cx = ox + lean * (0.3f + t * 0.45f);
		const float turn = v(variant, 20 + t) * kTau;
		for (int i = 0; i < tier.count; ++i) {
			// Jittered off the even spacing: clumps at exact intervals come out
			// as the scales of a pine cone.
			const float a = turn + static_cast<float>(i) / tier.count * kTau +
							(v(variant, 90 + t * 13 + i) - 0.5f) * 0.55f;
			// The clumps sit out on the ring rather than at the middle, which
			// is what gives the canopy its cauliflower edge.
			const float ride = tier.reach * (0.5f + v(variant, 30 + t * 13 + i) * 0.28f);
			// Small clumps, and a lot of them: a few big ones read as the
			// leaves of a cabbage rather than as the crown of a tree.
			const float lobe = tier.reach * (0.24f + v(variant, 60 + t * 13 + i) * 0.24f);
			paint.inkedPoly(blob(cx + std::cos(a) * ride, tier.y + std::sin(a) * ride * 0.92f,
								 lobe, variant, 130 + t * 17 + i),
							tier.fill, pen() * 0.5f);
		}
		// One more over the middle of the tier, so the ring of clumps closes up
		// into a mass instead of leaving a hole at its heart.
		paint.inkedPoly(blob(cx, tier.y, tier.reach * 0.5f, variant, 200 + t * 7), tier.fill,
						pen() * 0.6f);
	}

	// The shaded flank: hatching down the side the sun is not on. Kept out on
	// the flank rather than through the middle, where one stroke standing on
	// end read as a post sticking out of the tree.
	const float cx = ox + lean * 1.2f;
	const float cy = tiers[2].y;
	const std::vector<Point> crown = blob(cx, cy, tiers[1].reach, variant, 214);
	const float step = std::max(hatchStep() * 0.7f, tiers[1].reach * 0.16f);
	for (float hx = tiers[1].reach * 0.3f; hx < tiers[1].reach; hx += step) {
		markInside(paint, crown, cx + hx, cy + tiers[1].reach * 0.62f,
				   cx + hx + tiers[1].reach * 0.34f, cy - tiers[1].reach * 0.2f, fine(), kInk);
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

/** A colour lightened or darkened by a fraction, for shading one facet. */
Color shade(Color c, float amount) {
	const auto mix = [&](std::uint8_t v) {
		const float f = amount >= 0 ? v + (255 - v) * amount : v * (1 + amount);
		return static_cast<std::uint8_t>(std::clamp(f, 0.0f, 255.0f));
	};
	return Color{mix(c.r), mix(c.g), mix(c.b), c.a};
}

/**
 * A boulder, and the ore that comes out of one.
 *
 * Not an outline with lines drawn inside it: a rock seen from above is a set of
 * broken planes, and the only thing that makes one read as stone rather than as
 * a grey puddle is that each plane catches the light differently. So the shape
 * is built as a fan of facets off an off-centre ridge, each one filled at its
 * own brightness by which way it faces, each one inked. The silhouette comes
 * out of the facets rather than being drawn round them.
 *
 * Ore is the same rock with its own colour and with the metal or the sulfur
 * showing in the cracks between the planes.
 */
void paintStone(Paint& paint, float ox, float oy, float r, sim::NodeKind kind, int variant,
				bool snowy) {
	const Color body = nodeColor(kind);
	// The light comes from up and to the left, as it does on everything here.
	constexpr float kLightX = -0.55f;
	constexpr float kLightY = -0.84f;

	// A lumpy ring, and a ridge off the middle for the facets to run from.
	const int points = 9;
	std::vector<Point> hull;
	for (int i = 0; i < points; ++i) {
		const float a = static_cast<float>(i) / points * kTau;
		const float wob = 0.72f + (((variant * (i + 3)) % 37) / 37.0f) * 0.5f;
		hull.push_back({ox + std::cos(a) * r * wob, oy + std::sin(a) * r * 0.8f * wob});
	}
	// A crest rather than a peak. Every facet running to one point made a
	// sliced cake; a rock breaks along a line, and the planes fall away from
	// that line to either side of it.
	const float crestAngle = v(variant, 4) * kTau;
	const float crestLen = r * (0.22f + v(variant, 5) * 0.22f);
	const float midX = ox - r * (0.04f + v(variant, 6) * 0.12f);
	const float midY = oy - r * (0.06f + v(variant, 7) * 0.14f);
	const Point crestA{midX + std::cos(crestAngle) * crestLen,
					   midY + std::sin(crestAngle) * crestLen * 0.8f};
	const Point crestB{midX - std::cos(crestAngle) * crestLen,
					   midY - std::sin(crestAngle) * crestLen * 0.8f};

	// The facets. Each is one edge of the ring taken back to whichever end of
	// the crest is nearer, shaded by which way that edge faces the light.
	for (int i = 0; i < points; ++i) {
		const Point& a = hull[i];
		const Point& b = hull[(i + 1) % points];
		const float ex = (a.x + b.x) * 0.5f;
		const float ey = (a.y + b.y) * 0.5f;
		const float toA = (ex - crestA.x) * (ex - crestA.x) + (ey - crestA.y) * (ey - crestA.y);
		const float toB = (ex - crestB.x) * (ex - crestB.x) + (ey - crestB.y) * (ey - crestB.y);
		const Point& root = toA <= toB ? crestA : crestB;
		const float mx = ex - root.x;
		const float my = ey - root.y;
		const float len = std::max(0.0001f, std::sqrt(mx * mx + my * my));
		// One at the face square to the light, minus one at the face away.
		const float lit = (mx / len) * kLightX + (my / len) * kLightY;
		// Never white and never black: a rock is stone all over, only turned.
		const float amount = lit * 0.3f - 0.06f;
		// Four sided where the crest is not the nearer end, so the two halves
		// meet along the crest instead of leaving a wedge between them.
		const Point& far = toA <= toB ? crestB : crestA;
		paint.inkedPoly({root, a, b, far}, shade(body, amount), pen() * 0.55f);
	}
	// The crest itself, at the weight of the outline: it is the break in the
	// rock, not a mark on it.
	paint.line(crestA.x, crestA.y, crestB.x, crestB.y, pen() * 0.8f, kInk);
	// The pen round the whole thing, heavier than the lines between planes.
	paint.outlinePoly(hull, pen(), kInk);

	// Ore shows in the cracks: a fleck at the foot of every other facet, in
	// the line where two planes meet, which is where a vein would break out.
	if (kind == sim::NodeKind::Metal || kind == sim::NodeKind::Sulfur) {
		const Color fleck = kind == sim::NodeKind::Metal ? rgb(0xd9c7a4) : rgb(0xfaf07e);
		for (int i = static_cast<int>(variant) % 2; i < points; i += 2) {
			const Point& a = hull[i];
			// Part way down the crack from the crest to the corner.
			const float t = 0.45f + v(variant, 30 + i) * 0.3f;
			const float fx = crestA.x + (a.x - crestA.x) * t;
			const float fy = crestA.y + (a.y - crestA.y) * t;
			const float fr = r * (0.09f + v(variant, 50 + i) * 0.06f);
			paint.inkedPoly({{fx - fr, fy - fr * 0.25f},
							 {fx - fr * 0.15f, fy - fr},
							 {fx + fr, fy - fr * 0.1f},
							 {fx + fr * 0.25f, fy + fr}},
							fleck, fine());
		}
	}

	// Stipple down the side away from the light, which is how a press shades,
	// laid over the facets so it crosses them rather than sitting in one.
	const float step = halftoneStep() * 0.6f;
	for (float sy = -r; sy < r; sy += step) {
		for (float sx = -r; sx < r; sx += step) {
			const float deep = (sx * -kLightX + sy * -kLightY) / r;
			if (deep < 0.25f) continue;
			const float off = (static_cast<int>((sy + r) / step) % 2) * step / 2;
			if (!insidePoly(hull, ox + sx + off, oy + sy)) continue;
			paint.fillCircle(ox + sx + off, oy + sy, stippleDot() * (0.5f + deep * 0.7f), kInk);
		}
	}

	// A chip or two fallen at its foot, which is what says it has been worked
	// and what stops a field of them reading as a field of the same rock.
	const int chips = 1 + static_cast<int>(v(variant, 70) * 3);
	for (int i = 0; i < chips; ++i) {
		const float a = v(variant, 80 + i) * kTau;
		const float d = r * (1.0f + v(variant, 90 + i) * 0.25f);
		const float cx = ox + std::cos(a) * d;
		const float cy = oy + std::sin(a) * d * 0.7f;
		const float cr = r * (0.1f + v(variant, 100 + i) * 0.09f);
		paint.inkedPoly({{cx - cr, cy}, {cx - cr * 0.3f, cy - cr * 0.85f}, {cx + cr, cy - cr * 0.2f},
						 {cx + cr * 0.4f, cy + cr * 0.8f}},
						shade(body, -0.16f), pen() * 0.6f);
	}

	if (snowy) snowStone(paint, hull, ox, oy, r, variant);
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
