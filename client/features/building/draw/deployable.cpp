#include "client/features/building/draw/deployable.hpp"

#include <algorithm>
#include <cmath>

#include "client/design/tokens/world.tokens.hpp"
#include "sim/features/building/constants/deployable-defs.constant.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/building/types/deployable.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/point.struct.hpp"
#include "sim/features/building/systems/build-system.hpp"

namespace client {

namespace {

constexpr float kTau = 6.28318530718f;

void oval(Paint& paint, float x, float y, float rx, float ry, Color color, bool inked) {
	std::vector<Point> pts;
	for (int i = 0; i < 20; ++i) {
		const float a = static_cast<float>(i) / 20 * kTau;
		pts.push_back({x + std::cos(a) * rx, y + std::sin(a) * ry});
	}
	if (inked) {
		paint.inkedPoly(pts, color, kInkWidth);
	} else {
		paint.fillPoly(pts, color);
	}
}

void box(Paint& paint, float x, float y, float halfW, float halfH, Color color) {
	paint.inkedPoly({{x - halfW, y - halfH}, {x + halfW, y - halfH}, {x + halfW, y + halfH},
					 {x - halfW, y + halfH}},
					color, kInkWidth);
}

}  // namespace

void drawDeployable(Paint& paint, const sim::Deployable& deployable, float x, float y, float scale,
					float clock) {
	// Drawn to its own footprint rather than to one size for everything: a
	// campfire and a furnace are not the same thing on the floor. `r` is the
	// short half of the box, so nothing ever spills out of the squares it was
	// put down on, and `along` is the long half for the things that are long.
	double halfWide = 0;
	double halfDeep = 0;
	sim::BuildSystem::deployBounds(deployable.kind, deployable.turned, deployable.x, deployable.y,
								   halfWide, halfDeep);
	// `hw` and `hd` are the box it stands on, so a thing fills the squares it
	// was put down on rather than a circle inside them: a two by one bench
	// drawn inside its short half was half the bench it said it was. `r` is
	// the short half, for the round things that have no long side.
	const float hw = static_cast<float>(halfWide) * scale;
	const float hd = static_cast<float>(halfDeep) * scale;
	const float r = std::min(hw, hd);
	// A flame is never still: it breathes on its own clock.
	const float flicker = 0.75f + std::sin(clock * 11.0f) * 0.25f;
	switch (deployable.kind) {
		case sim::DeployKind::Campfire: {
			// A ring of stones with the wood stacked inside it, and a flame in
			// the middle of that once it is lit.
			oval(paint, x, y, r, r * 0.9f, rgb(0x6f6a5e), true);
			for (int i = 0; i < 6; ++i) {
				const float a = static_cast<float>(i) / 6 * kTau + 0.3f;
				oval(paint, x + std::cos(a) * r * 0.82f, y + std::sin(a) * r * 0.74f, r * 0.24f,
					 r * 0.2f, rgb(0x9aa0a6), true);
			}
			paint.line(x - r * 0.45f, y + r * 0.2f, x + r * 0.45f, y - r * 0.25f, r * 0.2f,
					   rgb(0x6b4a2a));
			paint.line(x - r * 0.4f, y - r * 0.3f, x + r * 0.42f, y + r * 0.28f, r * 0.2f,
					   rgb(0x8a5a2e));
			if (deployable.lit) {
				paint.fillPoly({{x - r * 0.3f * flicker, y + r * 0.2f},
								{x, y - r * 0.75f * flicker},
								{x + r * 0.3f * flicker, y + r * 0.2f}},
							   rgb(0xff8c2e));
				paint.fillPoly({{x - r * 0.14f * flicker, y + r * 0.1f},
								{x, y - r * 0.4f * flicker},
								{x + r * 0.14f * flicker, y + r * 0.1f}},
							   rgb(0xffd98a));
			}
			break;
		}
		case sim::DeployKind::Furnace: {
			// A stone drum with a mouth at the front.
			oval(paint, x, y, r, r, rgb(0x7e858c), true);
			oval(paint, x, y, r * 0.62f, r * 0.62f, rgb(0x4f555b), false);
			paint.inkedPoly({{x - r * 0.3f, y + r * 0.35f}, {x + r * 0.3f, y + r * 0.35f},
							 {x + r * 0.22f, y + r * 0.95f}, {x - r * 0.22f, y + r * 0.95f}},
							deployable.lit ? rgb(0xff8c2e) : rgb(0x2e2a22), kInkFine);
			break;
		}
		case sim::DeployKind::WoodenBox: {
			// A crate from above: planks across the lid, and the rim round it.
			box(paint, x, y, hw * 0.94f, hd * 0.94f, rgb(0x8a6034));
			box(paint, x, y, hw * 0.76f, hd * 0.76f, rgb(0xa8763f));
			for (int i = -1; i <= 1; ++i) {
				const float py = y + i * hd * 0.46f;
				paint.line(x - hw * 0.76f, py, x + hw * 0.76f, py, kInkFine, rgb(0x5d4022));
			}
			break;
		}
		case sim::DeployKind::LargeBox: {
			// A chest from above: longer than it is deep, iron bands across
			// the ends and a hasp on the near side.
			box(paint, x, y, hw * 0.96f, hd * 0.94f, rgb(0x7a5a34));
			box(paint, x, y, hw * 0.86f, hd * 0.78f, rgb(0xa8834f));
			for (int i = -1; i <= 1; i += 2) {
				box(paint, x + i * hw * 0.78f, y, hw * 0.1f, hd * 0.94f, rgb(0x4a4a52));
			}
			for (int i = -1; i <= 1; ++i) {
				const float py = y + i * hd * 0.44f;
				paint.line(x - hw * 0.66f, py, x + hw * 0.66f, py, kInkFine, rgb(0x5d4022));
			}
			box(paint, x, y + hd * 0.74f, hw * 0.14f, hd * 0.18f, rgb(0x6a6a74));
			break;
		}
		case sim::DeployKind::ToolCupboard: {
			box(paint, x, y, hw * 0.94f, hd * 0.94f, rgb(0x6b5540));
			box(paint, x, y - hd * 0.16f, hw * 0.58f, hd * 0.42f, rgb(0x8a7a5a));
			break;
		}
		case sim::DeployKind::Workbench1:
		case sim::DeployKind::Workbench2:
		case sim::DeployKind::Workbench3: {
			// Three benches on the same floor, told apart by what they are
			// made of and what is lying on them. A tier you can read across a
			// room without counting pips on it.
			const int tier = sim::benchTier(deployable.kind);
			const Color top = tier == 1   ? rgb(0x9a6b3a)
							  : tier == 2 ? rgb(0x6f7a84)
										  : rgb(0x4a5a6b);
			const Color trim = tier == 1   ? rgb(0x6b4522)
							   : tier == 2 ? rgb(0x4a525a)
										   : rgb(0x2c3845);
			box(paint, x, y, hw * 0.96f, hd * 0.92f, top);
			if (tier == 1) {
				// Planks and a vice: rough carpentry, and it looks it.
				for (int i = -1; i <= 1; ++i) {
					const float py = y + i * hd * 0.46f;
					paint.line(x - hw * 0.9f, py, x + hw * 0.9f, py, kInkFine, trim);
				}
				box(paint, x + hw * 0.58f, y, hw * 0.2f, hd * 0.3f, rgb(0x6a6a74));
				box(paint, x - hw * 0.3f, y - hd * 0.2f, hw * 0.28f, hd * 0.3f, rgb(0xc8b89a));
			} else if (tier == 2) {
				// A steel top with a cutting mat on it and a lamp over the end.
				box(paint, x - hw * 0.15f, y, hw * 0.5f, hd * 0.55f, rgb(0x2f4a3a));
				box(paint, x + hw * 0.62f, y - hd * 0.28f, hw * 0.16f, hd * 0.2f, rgb(0xffd98a));
				paint.line(x - hw * 0.9f, y - hd * 0.66f, x + hw * 0.9f, y - hd * 0.66f, kInkFine,
						   trim);
				for (int i = -1; i <= 1; i += 2) {
					paint.fillCircle(x + i * hw * 0.74f, y + hd * 0.5f, std::min(hw, hd) * 0.12f,
									 rgb(0x9aa6b0));
				}
			} else {
				// Plate, bolted down, with a vat at one end and a press at the
				// other: the tier you build guns on.
				box(paint, x - hw * 0.5f, y, hw * 0.3f, hd * 0.55f, rgb(0x8a9aa8));
				paint.fillCircle(x + hw * 0.42f, y, std::min(hw, hd) * 0.42f, rgb(0x2a3a2a));
				paint.fillCircle(x + hw * 0.42f, y, std::min(hw, hd) * 0.3f, rgb(0x5fe08a));
				for (int i = -1; i <= 1; i += 2) {
					for (int j = -1; j <= 1; j += 2) {
						paint.fillCircle(x + i * hw * 0.84f, y + j * hd * 0.7f,
										 std::min(hw, hd) * 0.1f, rgb(0xc8d2da));
					}
				}
			}
			break;
		}
		case sim::DeployKind::SleepingBag: {
			// Flat on the ground, and walked over rather than into.
			paint.inkedPoly({{x - hw * 0.96f, y - hd * 0.96f}, {x + hw * 0.96f, y - hd * 0.96f},
							 {x + hw * 0.96f, y + hd * 0.96f}, {x - hw * 0.96f, y + hd * 0.96f}},
							rgb(0xa05a5a), kInkFine);
			oval(paint, x, y - r * 0.6f, r * 0.45f, r * 0.3f, rgb(0xc98a8a), false);
			break;
		}
	}
}

}  // namespace client
