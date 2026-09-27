#include "client/features/creatures/draw/animal.hpp"

#include <algorithm>
#include <cmath>

#include <cmath>

#include "client/features/creatures/types/body-frame.struct.hpp"
#include "client/design/tokens/world.tokens.hpp"
#include "client/features/render/systems/text.hpp"
#include "sim/features/wildlife/constants/npc-defs.constant.hpp"
#include "sim/features/wildlife/types/npc-def.struct.hpp"
#include "sim/features/wildlife/types/npc-kind.enum.hpp"
#include "sim/features/wildlife/types/npc-state.enum.hpp"
#include "sim/features/wildlife/types/npc.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/align.enum.hpp"
#include "client/features/render/types/face.enum.hpp"
#include "client/features/render/types/point.struct.hpp"
#include "sim/features/wildlife/constants/wildlife-tuning.constant.hpp"

namespace client {

namespace {

Color coatOf(sim::NpcKind kind) {
	switch (kind) {
		case sim::NpcKind::Rabbit: return rgb(0xc9bda8);
		case sim::NpcKind::Elk: return rgb(0x8a6038);
		case sim::NpcKind::Kangaroo: return rgb(0xb07a52);
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
		case sim::NpcKind::Rabbit: return rgb(0x8e8471);
		case sim::NpcKind::Elk: return rgb(0x5c3f22);
		case sim::NpcKind::Kangaroo: return rgb(0x7a5236);
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
		case sim::NpcKind::Rabbit:
			// A ball with a nose on it. The ears are the whole silhouette and
			// are drawn separately, long and swept back.
			b = Build{0.98f, 0.82f, 0.0f, 0.1f, 1.02f, 0.44f, 0.24f, 0.4f, 0.5f, 0.0f, 0.26f};
			break;
		case sim::NpcKind::Elk:
			// Long in the leg and the neck like a deer, but heavier through
			// the shoulder, and the rack over the head is enormous.
			b = Build{1.3f, 0.68f, 0.16f, 0.66f, 1.75f, 0.44f, 0.32f, 0.8f, 1.0f, 0.28f, 0.28f};
			break;
		case sim::NpcKind::Kangaroo:
			// Narrow at the front, heavy at the back, and the tail is half the
			// animal: drawn separately, thick where it leaves the body.
			b = Build{1.08f, 0.66f, -0.3f, 0.36f, 1.42f, 0.42f, 0.28f, 0.5f, 1.1f, 0.3f, 1.25f};
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
	// Nothing on this island fights with its face. A bear does not headbutt, a
	// wolf does not either, and neither does a kangaroo: throwing the whole
	// body forward is what that looked like, so the reach below is left alone
	// for all of them and the blow is drawn after the body as a limb.
	//
	// A kangaroo does it differently enough to be worth its own case: it sits
	// back on that tail and drives both hind feet forward at once, which is the
	// one thing everybody knows about them.
	const bool claws = !def.human;
	const bool kicks = npc.kind == sim::NpcKind::Kangaroo;
	// Running, the body reaches forward and narrows; anything that hits with
	// its head, rather than a paw, throws itself at what it is hitting.
	const float reach =
		1 + std::min(1.0f, pace) * 0.16f + (claws ? 0.0f : (1 - lunge) * 0.22f * (striking ? 1 : 0));
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
	// The snout, out in front of the head. Every one of them has one.
	{
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

	// Ears, and the rack or the tail that goes with each one.
	if (b.ears > 0) {
		for (int side = -1; side <= 1; side += 2) {
			const float s = static_cast<float>(side);
			oval(paint, f, headX - r * b.headSize * 0.35f, s * r * b.headSize * 0.82f,
				 r * b.ears * 0.5f, r * b.ears * 0.62f, dark, true);
		}
	}
	if (npc.kind == sim::NpcKind::Elk) {
		// The rack: a heavy beam back over each shoulder with three tines off
		// it, which is the one silhouette nothing else on the island has.
		const Color horn = hurt ? rgb(0xffb0b0) : rgb(0xcbb894);
		for (int side = -1; side <= 1; side += 2) {
			const float s = static_cast<float>(side);
			const Point base = f.at(headX - r * 0.12f, s * r * 0.24f);
			const Point mid = f.at(headX + r * 0.45f, s * r * 0.95f);
			const Point tip = f.at(headX + r * 1.5f, s * r * 1.25f);
			paint.line(base.x, base.y, mid.x, mid.y, r * 0.16f, horn);
			paint.line(mid.x, mid.y, tip.x, tip.y, r * 0.12f, horn);
			for (int t = 0; t < 3; ++t) {
				const float along = 0.25f + t * 0.42f;
				const Point from = f.at(headX - r * 0.12f + r * 1.62f * along * 0.55f,
										s * r * (0.24f + 1.01f * along));
				const Point out = f.at(headX + r * (0.55f + along * 0.95f),
									   s * r * (0.2f + along * 0.35f));
				paint.line(from.x, from.y, out.x, out.y, r * 0.09f, horn);
			}
		}
	}
	if (npc.kind == sim::NpcKind::Rabbit) {
		// Two long ears laid back along its spine: at this size they are the
		// only thing that tells a rabbit from a stone.
		for (int side = -1; side <= 1; side += 2) {
			const float s = static_cast<float>(side);
			std::vector<Point> ear;
			for (int i = 0; i <= 10; ++i) {
				const float t = static_cast<float>(i) / 10;
				const float along = headX - r * 0.2f - r * 1.75f * t;
				const float across = s * r * (0.26f + t * 0.42f);
				const float wide = r * 0.2f * std::sin(3.14159265f * (0.15f + t * 0.85f));
				ear.push_back(f.at(along, across - wide));
			}
			for (int i = 10; i >= 0; --i) {
				const float t = static_cast<float>(i) / 10;
				const float along = headX - r * 0.2f - r * 1.75f * t;
				const float across = s * r * (0.26f + t * 0.42f);
				const float wide = r * 0.2f * std::sin(3.14159265f * (0.15f + t * 0.85f));
				ear.push_back(f.at(along, across + wide));
			}
			paint.inkedPoly(ear, coat, kInkWidth);
		}
	}
	if (npc.kind == sim::NpcKind::Kangaroo) {
		// The haunches, which is where all its weight is, and the big hind
		// feet turned out under them.
		for (int side = -1; side <= 1; side += 2) {
			const float s = static_cast<float>(side);
			oval(paint, f, -r * 0.5f, s * r * 0.58f, r * 0.42f, r * 0.34f, dark, true);
			oval(paint, f, -r * 0.15f + gait * s * 0.4f, s * r * 0.74f, r * 0.46f, r * 0.17f,
				 dark, true);
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

	if (kicks && striking) {
		// Both hind feet forward together, past the head, with the tail
		// planted behind: a kangaroo kicks off its tail like a tripod.
		const float out = 0.4f + lunge * 1.15f;
		const Color pad = hurt ? rgb(0xffb0b0) : rgb(0x5f4028);
		for (int side = -1; side <= 1; side += 2) {
			const float sd = static_cast<float>(side);
			const float along = r * (0.3f + out * 1.25f);
			const float across = sd * r * (0.52f - out * 0.08f);
			const Point hip = f.at(-r * 0.35f, sd * r * 0.6f);
			const Point foot = f.at(along, across);
			paint.line(hip.x, hip.y, foot.x, foot.y, r * 0.2f + kInkWidth * scale, kInk);
			paint.line(hip.x, hip.y, foot.x, foot.y, r * 0.2f, dark);
			// The foot itself, long and turned along the kick.
			paint.inkedPoly({f.at(along - r * 0.34f, across - r * 0.19f),
							 f.at(along + r * 0.4f, across - r * 0.1f),
							 f.at(along + r * 0.4f, across + r * 0.1f),
							 f.at(along - r * 0.34f, across + r * 0.19f)},
							pad, kInkFine);
			// Three claws off the end of it.
			const Color claw = hurt ? rgb(0xffb0b0) : rgb(0xe8e2d0);
			for (int i = -1; i <= 1; ++i) {
				const float spread = i * 0.11f;
				paint.inkedPoly(
					{f.at(along + r * 0.36f, across + r * (spread - 0.05f)),
					 f.at(along + r * 0.62f, across + r * spread * 1.3f),
					 f.at(along + r * 0.36f, across + r * (spread + 0.05f))},
					claw, kInkFine);
			}
		}
	} else if (claws && striking) {
		// A foreleg out and across, with three claws on the end of it: which
		// side it swings from is the animal's own, so a pack does not swipe in
		// unison. The paw is furthest out just as the blow lands.
		const float side = (npc.seed % 2 == 0) ? 1.0f : -1.0f;
		const float out = 0.5f + lunge * 0.85f;
		// The paw stays on its own side of the animal rather than crossing the
		// centre line: swung across the face it read as a moustache.
		// Well outside the body: a wolf's head reaches r * 1.6 forward and is
		// r * 0.7 wide, so anything nearer than this lands on its own face.
		const float pawAlong = r * (0.7f + out * 0.8f);
		const float pawAcross = side * r * (1.25f - out * 0.22f);
		const Point shoulder = f.at(r * 0.4f, side * r * b.bodyWide * 0.75f);
		const Point paw = f.at(pawAlong, pawAcross);
		paint.line(shoulder.x, shoulder.y, paw.x, paw.y, r * 0.22f + kInkWidth * scale, kInk);
		paint.line(shoulder.x, shoulder.y, paw.x, paw.y, r * 0.22f, dark);
		oval(paint, f, pawAlong, pawAcross, r * 0.21f, r * 0.18f, dark, true);
		const Color claw = hurt ? rgb(0xffb0b0) : rgb(0xe8e2d0);
		for (int i = -1; i <= 1; ++i) {
			// Three of them, fanned forward off the front of the paw.
			const float spread = i * 0.34f;
			paint.inkedPoly({f.at(pawAlong + r * 0.12f, pawAcross + r * (spread - 0.09f)),
							 f.at(pawAlong + r * 0.52f, pawAcross + r * (spread * 1.5f)),
							 f.at(pawAlong + r * 0.12f, pawAcross + r * (spread + 0.09f))},
							claw, kInkFine);
		}
	}

	paint.useWorldScale(wasScale);
}


void drawAnimalTag(Paint& paint, const sim::Npc& npc, float x, float y, float scale) {
	const sim::NpcDef& def = sim::npcDef(npc.kind);
	const float r = static_cast<float>(def.radius) * scale;
	// Hurt, and hurt recently enough to still be saying so. A bar over
	// everything you have ever shot at is a map of your own past rather than
	// anything about now: see sim::kHealthShownFor. The health itself does not
	// change when the bar goes.
	const bool hurt = npc.hp < def.hp && npc.sinceHurt < sim::kHealthShownFor;
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
