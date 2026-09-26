#include "animal.hpp"

#include <algorithm>
#include <cmath>

#include <cmath>

#include "body_frame.hpp"
#include "palette.hpp"
#include "text.hpp"

namespace client {

namespace {

Color coatOf(sim::NpcKind kind) {
	switch (kind) {
		case sim::NpcKind::Chicken: return rgb(0xe8e2d4);
		case sim::NpcKind::Deer: return rgb(0xb07d4a);
		case sim::NpcKind::Wolf: return rgb(0x8e97a5);
		case sim::NpcKind::Bear: return rgb(0x88522c);
		// The people of the monuments: a lab coat and a field green.
		case sim::NpcKind::Scientist: return rgb(0xe6ebf0);
		case sim::NpcKind::Soldier: return rgb(0x6f7a52);
	}
	return rgb(0x966e50);
}

Color darkOf(sim::NpcKind kind) {
	switch (kind) {
		case sim::NpcKind::Chicken: return rgb(0xbdb3a0);
		case sim::NpcKind::Deer: return rgb(0x7d5530);
		case sim::NpcKind::Wolf: return rgb(0x505d6d);
		case sim::NpcKind::Bear: return rgb(0x523119);
		case sim::NpcKind::Scientist: return rgb(0x758494);
		case sim::NpcKind::Soldier: return rgb(0x454d31);
	}
	return rgb(0x573d27);
}

void oval(Paint& paint, const BodyFrame& f, float x, float y, float rx, float ry, Color color,
		  bool inked) {
	std::vector<Point> pts;
	for (int i = 0; i < 20; ++i) {
		const float a = static_cast<float>(i) / 20 * 6.28318530718f;
		pts.push_back(f.at(x + std::cos(a) * rx, y + std::sin(a) * ry));
	}
	if (inked) {
		paint.inkedPoly(pts, color, kInkWidth);
	} else {
		paint.fillPoly(pts, color);
	}
}

/** Whether it comes after things, which is what puts a light in its eye. */
bool npcDefIsHunter(const sim::NpcDef& def) { return def.hostile || def.eatsCount > 0; }

}  // namespace

/**
 * One animal, seen from above.
 *
 * Every one is built the same way, nose forward along positive x, and told
 * apart by the shape of its own body rather than by its colour: a deer is
 * long-legged with a neck and a rump, a wolf is lean with a long snout, a bear
 * is a slab with a low head, a hyena slopes down from the shoulder, and a
 * chicken is a ball with a beak. The proportions below are the whole of the
 * difference, so there is no place for two animals to drift into one drawing.
 */
void drawAnimal(Paint& paint, const sim::Npc& npc, float x, float y, float scale) {
	const sim::NpcDef& def = sim::npcDef(npc.kind);
	const float r = static_cast<float>(def.radius) * scale;
	// The animal is laid out nose-forward along positive x, and turned by the
	// frame, so nothing below has to know which way it is facing. The pen is
	// told the frame's scale, so its ink grows with it. See world-scale.md.
	const BodyFrame f{x, y, std::cos(static_cast<float>(npc.facing)),
					  std::sin(static_cast<float>(npc.facing)), 1.0f};
	const float wasScale = paint.worldScale();
	paint.useWorldScale(scale);

	const bool hurt = npc.flash > 0;
	const Color coat = hurt ? rgb(0xffdede) : coatOf(npc.kind);
	const Color dark = hurt ? rgb(0xffb0b0) : darkOf(npc.kind);

	/** The build of this animal, in fractions of its radius. */
	struct Build {
		float bodyLong;
		float bodyWide;
		/** How far forward the shoulders sit, which is what a slope is. */
		float shoulder;
		float neck;
		float headAt;
		float headSize;
		float snout;
		float legReach;
		float legLength;
		float ears;
		float tail;
	};
	Build b{};
	switch (npc.kind) {
		case sim::NpcKind::Chicken:
			// Round, tiny, no neck to speak of and a beak instead of a snout.
			b = Build{1.0f, 0.92f, 0.0f, 0.15f, 1.05f, 0.5f, 0.28f, 0.42f, 0.55f, 0.0f, 0.5f};
			break;
		case sim::NpcKind::Deer:
			// Long in the leg, long in the neck, narrow through the body.
			b = Build{1.25f, 0.62f, 0.1f, 0.62f, 1.7f, 0.42f, 0.3f, 0.78f, 0.95f, 0.3f, 0.3f};
			break;
		case sim::NpcKind::Wolf:
			b = Build{1.3f, 0.7f, 0.08f, 0.45f, 1.6f, 0.44f, 0.46f, 0.62f, 0.72f, 0.26f, 0.66f};
			break;
		case sim::NpcKind::Bear:
			// A slab with a head on the front of it.
			b = Build{1.28f, 1.02f, 0.05f, 0.22f, 1.42f, 0.62f, 0.34f, 0.72f, 0.5f, 0.22f, 0.16f};
			break;
		default: b = Build{1.2f, 0.85f, 0, 0.3f, 1.4f, 0.5f, 0.35f, 0.6f, 0.6f, 0.25f, 0.4f}; break;
	}

	// How it is moving, which is the whole of the animation: a walk is a short
	// even step, a run is a long one with the body stretched into it, and an
	// animal mid-blow lunges rather than steps.
	const float pace = static_cast<float>(std::hypot(npc.vx, npc.vy)) / static_cast<float>(def.speed);
	const bool striking = npc.state == sim::NpcState::Attack && npc.attackTimer > 0;
	// Nought just after a blow lands and back to one as it recovers: the lunge.
	const float lunge =
		striking ? std::max(0.0f, 1 - static_cast<float>(npc.attackTimer / def.attackCooldown) * 2)
				 : 0.0f;
	const float stride = 1.6f + std::min(1.4f, pace) * 2.2f;
	const float gait = std::sin(static_cast<float>(npc.animPhase)) * stride * scale;
	// Running, the body reaches forward and narrows; striking, it throws itself
	// at what it is hitting.
	const float reach = 1 + std::min(1.0f, pace) * 0.16f + (1 - lunge) * 0.22f * (striking ? 1 : 0);
	const float narrow = 1 - std::min(1.0f, pace) * 0.1f;
	// And it bobs, which is what stops a run reading as a slide.
	const float bob = std::sin(static_cast<float>(npc.animPhase) * 2) * std::min(1.0f, pace) * 0.05f;
	const float hind = -r * b.legReach;
	const float fore = r * b.legReach * reach;
	const float spread = r * b.bodyWide * 0.82f * narrow;
	for (int side = -1; side <= 1; side += 2) {
		const float s = static_cast<float>(side);
		// Each foot reaches out from the hip along the body, so a long-legged
		// deer takes a long stride and a chicken scurries.
		const float swing = gait * s;
		oval(paint, f, hind + swing * 0.5f, s * spread, r * 0.2f, r * 0.14f, dark, true);
		oval(paint, f, fore - swing * 0.5f, s * spread, r * 0.2f, r * 0.14f, dark, true);
		// The leg itself, from the body down to the foot.
		paint.line(f.at(hind * 0.7f, s * spread * 0.6f).x, f.at(hind * 0.7f, s * spread * 0.6f).y,
				   f.at(hind + swing * 0.5f, s * spread).x,
				   f.at(hind + swing * 0.5f, s * spread).y, r * 0.16f * b.legLength, dark);
		paint.line(f.at(fore * 0.7f, s * spread * 0.6f).x, f.at(fore * 0.7f, s * spread * 0.6f).y,
				   f.at(fore - swing * 0.5f, s * spread).x,
				   f.at(fore - swing * 0.5f, s * spread).y, r * 0.16f * b.legLength, dark);
	}

	// The tail, before the body, so it comes out from under the rump.
	if (b.tail > 0) {
		const float wag = std::sin(static_cast<float>(npc.animPhase) * 0.6f) * 0.3f;
		const Point root = f.at(-r * b.bodyLong * 0.92f, 0);
		const Point tip = f.at(-r * (b.bodyLong + b.tail), r * b.tail * wag);
		paint.line(root.x, root.y, tip.x, tip.y, r * 0.22f, dark);
		paint.line(root.x, root.y, tip.x, tip.y, r * 0.22f - kInkWidth * scale, coat);
	}

	// The body: an oval carrying the shoulders forward, which is what makes a
	// hyena slope and a deer look narrow.
	{
		std::vector<Point> hull;
		for (int i = 0; i < 26; ++i) {
			const float a = static_cast<float>(i) / 26 * 6.28318530718f;
			const float along = std::cos(a);
			// Wider at the shoulder end than at the rump.
			const float taper = 1 + b.shoulder * along;
			hull.push_back(f.at(along * r * b.bodyLong * (1 + bob),
								std::sin(a) * r * b.bodyWide * taper * narrow));
		}
		paint.inkedPoly(hull, coat, kInkWidth);
		// The shadow down its back, which is what stops it reading as a stone.
		oval(paint, f, -r * 0.12f, 0, r * b.bodyLong * 0.62f, r * b.bodyWide * 0.42f, dark, false);
	}

	// The neck, then the head on the end of it.
	const float headX = r * b.headAt * reach;
	if (b.neck > 0.2f) {
		const Point from = f.at(r * b.bodyLong * 0.7f, 0);
		const Point to = f.at(headX, 0);
		paint.line(from.x, from.y, to.x, to.y, r * b.headSize * 1.1f, kInk);
		paint.line(from.x, from.y, to.x, to.y, r * b.headSize * 1.1f - kInkWidth * scale * 2, coat);
	}
	oval(paint, f, headX, 0, r * b.headSize, r * b.headSize * 0.86f, coat, true);
	// The snout, out in front of the head.
	if (npc.kind == sim::NpcKind::Chicken) {
		// A beak: a wedge, in the one colour nothing else here uses.
		paint.fillPoly({f.at(headX + r * b.headSize * 0.7f, -r * 0.14f),
						f.at(headX + r * (b.headSize + b.snout), 0),
						f.at(headX + r * b.headSize * 0.7f, r * 0.14f)},
					   rgb(0xe8a12c));
		// The comb, which is the rest of what says chicken.
		for (int i = -1; i <= 1; ++i) {
			oval(paint, f, headX - r * 0.1f + i * r * 0.2f, 0, r * 0.12f, r * 0.1f,
				 rgb(0xc8433a), true);
		}
	} else {
		std::vector<Point> muzzle;
		for (int i = 0; i < 16; ++i) {
			const float a = static_cast<float>(i) / 16 * 6.28318530718f;
			muzzle.push_back(f.at(headX + r * b.snout * 0.7f + std::cos(a) * r * b.snout * 0.7f,
								  std::sin(a) * r * b.headSize * 0.5f));
		}
		paint.inkedPoly(muzzle, dark, kInkFine);
		// The nose on the end of it.
		oval(paint, f, headX + r * (b.snout * 1.25f), 0, r * 0.11f, r * 0.11f, kInk, false);
	}

	// Ears, and a deer's antlers, which is the one silhouette nothing else has.
	if (b.ears > 0) {
		for (int side = -1; side <= 1; side += 2) {
			const float s = static_cast<float>(side);
			oval(paint, f, headX - r * b.headSize * 0.35f, s * r * b.headSize * 0.82f,
				 r * b.ears * 0.5f, r * b.ears * 0.62f, dark, true);
		}
	}
	if (npc.kind == sim::NpcKind::Deer) {
		const Color horn = hurt ? rgb(0xffb0b0) : rgb(0xd8c8a8);
		for (int side = -1; side <= 1; side += 2) {
			const float s = static_cast<float>(side);
			const Point base = f.at(headX - r * 0.1f, s * r * 0.22f);
			const Point mid = f.at(headX + r * 0.5f, s * r * 0.72f);
			const Point tip = f.at(headX + r * 1.15f, s * r * 0.95f);
			paint.line(base.x, base.y, mid.x, mid.y, r * 0.13f, horn);
			paint.line(mid.x, mid.y, tip.x, tip.y, r * 0.1f, horn);
			// One tine off the middle of each, which is what makes it a rack.
			const Point tine = f.at(headX + r * 0.62f, s * r * 0.36f);
			paint.line(mid.x, mid.y, tine.x, tine.y, r * 0.09f, horn);
		}
	}

	// Eyes: the one thing that says which end is which at a glance. Dark in
	// every animal, because a red eye read as a status light rather than as a
	// creature, and what a bear is does not need announcing.
	const Color eye = rgb(0x2a2018);
	for (int side = -1; side <= 1; side += 2) {
		const float s = static_cast<float>(side);
		oval(paint, f, headX + r * b.headSize * 0.35f, s * r * b.headSize * 0.42f, r * 0.09f,
			 r * 0.09f, eye, false);
	}

	paint.useWorldScale(wasScale);
}


void drawAnimalTag(Paint& paint, const sim::Npc& npc, float x, float y, float scale) {
	const sim::NpcDef& def = sim::npcDef(npc.kind);
	const float r = static_cast<float>(def.radius) * scale;
	const bool hurt = npc.hp < def.hp;
	if (hurt) {
		// White in black, so it reads on snow and on grass alike.
		const float w = r * 2.4f;
		// Half what it was: at five points the bar was a plank across the animal.
		const float h = 2.5f * scale;
		const float edge = scale;
		const float bx = x - w / 2;
		const float by = y - r - 14 * scale;
		paint.fillRect(bx - edge, by - edge, w + edge * 2, h + edge * 2, kInk);
		paint.fillRect(bx, by, w * npc.hp / def.hp, h, rgb(0xffffff));
	}
	if (Text* lettering = paint.text()) {
		lettering->drawInked(def.name, x, y - r - (hurt ? 32 : 22) * scale, 13 * scale,
							 rgb(0xffffff), Face::Body, Align::Centre,
							 std::max(1, static_cast<int>(std::lround(scale))));
	}
}

}  // namespace client
