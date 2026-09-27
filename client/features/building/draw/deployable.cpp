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
	const float r = static_cast<float>(std::min(halfWide, halfDeep)) * scale;
	const float along = static_cast<float>(std::max(halfWide, halfDeep)) * scale;
	(void)along;
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
			box(paint, x, y, r * 0.9f, r * 0.9f, rgb(0x8a6034));
			box(paint, x, y, r * 0.74f, r * 0.74f, rgb(0xa8763f));
			for (int i = -1; i <= 1; ++i) {
				const float py = y + i * r * 0.44f;
				paint.line(x - r * 0.74f, py, x + r * 0.74f, py, kInkFine, rgb(0x5d4022));
			}
			break;
		}
		case sim::DeployKind::LargeBox: {
			// A chest from above: longer than it is deep, iron bands across
			// the ends and a hasp on the near side.
			box(paint, x, y, r * 1.45f, r * 0.82f, rgb(0x7a5a34));
			box(paint, x, y, r * 1.3f, r * 0.68f, rgb(0xa8834f));
			for (int i = -1; i <= 1; i += 2) {
				box(paint, x + i * r * 1.14f, y, r * 0.16f, r * 0.82f, rgb(0x4a4a52));
			}
			for (int i = -1; i <= 1; ++i) {
				const float py = y + i * r * 0.4f;
				paint.line(x - r * 1.0f, py, x + r * 1.0f, py, kInkFine, rgb(0x5d4022));
			}
			box(paint, x, y + r * 0.7f, r * 0.2f, r * 0.16f, rgb(0x6a6a74));
			break;
		}
		case sim::DeployKind::ToolCupboard: {
			box(paint, x, y, r * 0.9f, r * 0.9f, rgb(0x6b5540));
			box(paint, x, y - r * 0.15f, r * 0.55f, r * 0.4f, rgb(0x8a7a5a));
			break;
		}
		case sim::DeployKind::Workbench1:
		case sim::DeployKind::Workbench2:
		case sim::DeployKind::Workbench3: {
			// A bench with its work laid out on it, and a mark for its tier.
			box(paint, x, y, r * 0.98f, r * 0.7f, rgb(0x8a6034));
			box(paint, x, y - r * 0.18f, r * 0.7f, r * 0.3f, rgb(0xb0b9c1));
			const int tier = sim::benchTier(deployable.kind);
			for (int i = 0; i < tier; ++i) {
				paint.fillCircle(x - r * 0.5f + i * r * 0.36f, y + r * 0.42f, r * 0.12f,
								 rgb(0xc9a227));
			}
			break;
		}
		case sim::DeployKind::SleepingBag: {
			// Flat on the ground, and walked over rather than into.
			paint.inkedPoly({{x - r * 0.65f, y - r * 0.95f}, {x + r * 0.65f, y - r * 0.95f},
							 {x + r * 0.65f, y + r * 0.95f}, {x - r * 0.65f, y + r * 0.95f}},
							rgb(0xa05a5a), kInkFine);
			oval(paint, x, y - r * 0.6f, r * 0.45f, r * 0.3f, rgb(0xc98a8a), false);
			break;
		}
	}
}

}  // namespace client
