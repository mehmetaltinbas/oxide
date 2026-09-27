#include "client/features/render/systems/particles.hpp"

#include <algorithm>
#include <cmath>

#include "client/design/tokens/world.tokens.hpp"
#include "sim/shared/utils/rng.util.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/particle.struct.hpp"
#include "client/features/render/types/point.struct.hpp"

namespace client {

namespace {

/** Beyond this many at once, the oldest are dropped rather than the frame. */
constexpr std::size_t kMost = 900;

}  // namespace

void Particles::burst(double x, double y, int count, Color color, double speed, double life,
					  double size, double gravity, double dir, double spread) {
	for (int i = 0; i < count; ++i) {
		sim::Rng rng(rolls_ += 0x9e3779b9u);
		const double a = dir + (rng.unit() - 0.5) * spread;
		const double v = speed * (0.4 + rng.unit() * 0.6);
		const double span = life * (0.6 + rng.unit() * 0.8);
		specks_.push_back(Particle{x, y, std::cos(a) * v, std::sin(a) * v, span, span,
								   size * (0.6 + rng.unit() * 0.8), gravity, color});
	}
	if (specks_.size() > kMost) {
		specks_.erase(specks_.begin(),
					  specks_.begin() + static_cast<long>(specks_.size() - kMost));
	}
}

void Particles::ember(double x, double y) {
	sim::Rng rng(rolls_ += 0x9e3779b9u);
	specks_.push_back(Particle{x + (rng.unit() - 0.5) * 10, y, (rng.unit() - 0.5) * 12,
							   -18 - rng.unit() * 20, 0.8, 0.8, 1.8, -12, Color{255, 190, 110, 255}});
}

void Particles::spentMagazine(double x, double y, double facing) {
	sim::Rng rng(rolls_ += 0x9e3779b9u);
	// Thrown down and away from the gun hand, which is to the right of you.
	const double away = facing + 1.3 + rng.range(-0.4, 0.4);
	const double speed = rng.range(40, 70);
	casings_.push_back(Casing{x, y, std::cos(away) * speed, std::sin(away) * speed,
							  rng.range(0, 6.283), rng.range(-9, 9), 2.6, 2.6});
}

void Particles::update(double dt) {
	for (Casing& mag : casings_) {
		mag.life -= dt;
		mag.x += mag.vx * dt;
		mag.y += mag.vy * dt;
		mag.angle += mag.spin * dt;
		// Slides to a stop rather than drifting for ever.
		const double drag = 1 - std::min(1.0, 5.0 * dt);
		mag.vx *= drag;
		mag.vy *= drag;
		mag.spin *= drag;
	}
	casings_.erase(std::remove_if(casings_.begin(), casings_.end(),
								  [](const Casing& mag) { return mag.life <= 0; }),
				   casings_.end());
	for (Particle& speck : specks_) {
		speck.life -= dt;
		speck.vy += speck.gravity * dt;
		speck.x += speck.vx * dt;
		speck.y += speck.vy * dt;
	}
	specks_.erase(std::remove_if(specks_.begin(), specks_.end(),
								 [](const Particle& speck) { return speck.life <= 0; }),
				  specks_.end());
	if (shakeLeft_ > 0) shakeLeft_ -= dt;
}

void Particles::draw(Paint& paint, double cameraX, double cameraY, double scale, int width,
					 int height) const {
	for (const Particle& speck : specks_) {
		const float sx = static_cast<float>((speck.x - cameraX) * scale) + width * 0.5f;
		const float sy = static_cast<float>((speck.y - cameraY) * scale) + height * 0.5f;
		if (sx < -20 || sy < -20 || sx > width + 20 || sy > height + 20) continue;
		Color color = speck.color;
		// Fading out rather than blinking away at the end of its life.
		color.a = static_cast<std::uint8_t>(255 * std::clamp(speck.life / speck.total, 0.0, 1.0));
		paint.fillCircle(sx, sy, static_cast<float>(speck.size * scale), color);
	}
	// The magazines last of all, so a thrown one lies over the dust it raised.
	for (const Casing& mag : casings_) {
		const float sx = static_cast<float>((mag.x - cameraX) * scale) + width * 0.5f;
		const float sy = static_cast<float>((mag.y - cameraY) * scale) + height * 0.5f;
		if (sx < -30 || sy < -30 || sx > width + 30 || sy > height + 30) continue;
		// Gone in its last half second rather than blinking out.
		const auto fade = static_cast<std::uint8_t>(
			255 * std::clamp(mag.life / 0.5, 0.0, 1.0));
		const float ca = static_cast<float>(std::cos(mag.angle));
		const float sa = static_cast<float>(std::sin(mag.angle));
		const float half = 5.0f * static_cast<float>(scale);
		const float wide = 2.0f * static_cast<float>(scale);
		const auto at = [&](float along, float across) {
			return Point{sx + ca * along - sa * across, sy + sa * along + ca * across};
		};
		const std::vector<Point> box{at(-half, -wide), at(half, -wide * 0.7f),
									 at(half, wide * 0.7f), at(-half, wide)};
		paint.fillPoly(box, Color{58, 60, 66, fade});
		paint.outlinePoly(box, kInkFine, Color{kInk.r, kInk.g, kInk.b, fade});
	}
}

void Particles::shake(double amount, double seconds) {
	// The bigger shock wins rather than the later one.
	if (amount <= shakeAmount_ && shakeLeft_ > 0) return;
	shakeAmount_ = amount;
	shakeLeft_ = seconds;
	shakeTotal_ = seconds;
}

void Particles::shakeOffset(double& x, double& y) const {
	if (shakeLeft_ <= 0) {
		x = 0;
		y = 0;
		return;
	}
	const double left = shakeLeft_ / shakeTotal_;
	sim::Rng rng(static_cast<std::uint32_t>(shakeLeft_ * 100000));
	x = (rng.unit() - 0.5) * 2 * shakeAmount_ * left;
	y = (rng.unit() - 0.5) * 2 * shakeAmount_ * left;
}

}  // namespace client
