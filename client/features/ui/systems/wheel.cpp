#include "client/features/ui/systems/wheel.hpp"

#include <cmath>

#include "client/design/tokens/interface.tokens.hpp"
#include "client/design/tokens/world.tokens.hpp"
#include "client/features/items/draw/item-glyph.hpp"
#include "client/features/render/systems/text.hpp"

namespace client {

namespace {

constexpr float kTau = 6.28318530717959f;
/** How far out the ring sits, and how thick it is, in interface points. */
constexpr float kInner = 46.0f;
constexpr float kOuter = 124.0f;
/**
 * How far the cursor must leave the middle before a slice counts.
 *
 * Short of this, nothing is chosen: letting go without moving is how you
 * change your mind, and a wheel that picks whatever was nearest the moment it
 * opened cannot be dismissed.
 */
constexpr float kDeadZone = 26.0f;

}  // namespace

void Wheel::show(std::vector<WheelOption> options, float x, float y) {
	options_ = std::move(options);
	x_ = x;
	y_ = y;
	open_ = !options_.empty();
}

int Wheel::hovered(float mouseX, float mouseY, float uiScale) const {
	if (!open_ || options_.empty()) return -1;
	const float dx = mouseX - x_;
	const float dy = mouseY - y_;
	if (std::hypot(dx, dy) < kDeadZone * uiScale) return -1;
	// Measured from straight up, going clockwise, which is the order the
	// slices are drawn in and the order they were given in.
	float a = std::atan2(dx, -dy);
	if (a < 0) a += kTau;
	const float span = kTau / static_cast<float>(options_.size());
	return static_cast<int>(a / span) % static_cast<int>(options_.size());
}

int Wheel::release(float mouseX, float mouseY, float uiScale) {
	const int at = hovered(mouseX, mouseY, uiScale);
	open_ = false;
	if (at < 0) return -1;
	return options_[static_cast<std::size_t>(at)].allowed ? at : -1;
}

void Wheel::draw(Paint& paint, float mouseX, float mouseY, float uiScale) const {
	if (!open_ || options_.empty()) return;
	const int count = static_cast<int>(options_.size());
	const int on = hovered(mouseX, mouseY, uiScale);
	const float inner = kInner * uiScale;
	const float outer = kOuter * uiScale;
	const float span = kTau / static_cast<float>(count);

	for (int i = 0; i < count; ++i) {
		const WheelOption& slice = options_[static_cast<std::size_t>(i)];
		const float from = -kTau * 0.25f + span * i;
		const float to = from + span;
		// The slice itself: an arc band, built as a strip of quads because a
		// fan cannot carry a ring.
		std::vector<Point> band;
		constexpr int kSteps = 10;
		for (int k = 0; k <= kSteps; ++k) {
			const float a = from + (to - from) * (static_cast<float>(k) / kSteps);
			band.push_back({x_ + std::cos(a) * inner, y_ + std::sin(a) * inner});
		}
		for (int k = kSteps; k >= 0; --k) {
			const float a = from + (to - from) * (static_cast<float>(k) / kSteps);
			band.push_back({x_ + std::cos(a) * outer, y_ + std::sin(a) * outer});
		}
		// An opaque backing under the token, so the ring reads over grass as
		// well as over a dark panel: the interface colours are translucent by
		// design and a menu you are choosing from cannot be.
		paint.fillPoly(band, Color{18, 20, 18, 240});
		const Color fill = !slice.allowed ? Color{40, 34, 30, 190}
							 : i == on    ? ui::kSelected
										  : ui::kSurfaceAlt;
		paint.fillPoly(band, fill);
		paint.outlinePoly(band, 1.5f * uiScale, kInk);

		// What is on it, half way along the slice and half way out.
		const float mid = (from + to) * 0.5f;
		const float at = (inner + outer) * 0.5f;
		const float cx = x_ + std::cos(mid) * at;
		const float cy = y_ + std::sin(mid) * at;
		if (slice.icon != sim::ItemId::None) {
			drawItemIcon(paint, slice.icon, cx, cy - 10 * uiScale, 30 * uiScale);
		}
		if (Text* lettering = paint.text()) {
			lettering->draw(slice.label, cx, cy + 10 * uiScale, 12 * uiScale,
							slice.allowed ? ui::kInk : ui::kFaint, Face::BodyBold, Align::Centre);
			if (slice.note[0] != '\0') {
				lettering->draw(slice.note, cx, cy + 24 * uiScale, 10 * uiScale, ui::kSubtle,
								Face::Body, Align::Centre);
			}
		}
	}
}

}  // namespace client
