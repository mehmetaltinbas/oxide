#include "particles.hpp"

#include <algorithm>
#include <cmath>

#include "sim/rng.hpp"

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

void Particles::update(double dt) {
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
