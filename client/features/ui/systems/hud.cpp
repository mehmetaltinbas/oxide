#include "client/features/ui/systems/hud.hpp"

#include <algorithm>

#include "client/features/items/draw/item-glyph.hpp"
#include "client/design/tokens/world.tokens.hpp"
#include "client/design/tokens/interface.tokens.hpp"
#include "client/features/render/systems/text.hpp"
#include "client/features/ui/draw/vital-icon.hpp"
#include "sim/features/survival/systems/survival.hpp"
#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/food.struct.hpp"
#include "sim/features/items/types/gun.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/align.enum.hpp"
#include "client/features/render/types/face.enum.hpp"
#include "client/features/ui/types/belt-slot.struct.hpp"
#include "client/features/ui/types/popup.struct.hpp"
#include "client/features/ui/utils/belt-slot-box.util.hpp"
#include "sim/features/items/utils/is-ranged.util.hpp"

namespace client {

namespace {

/** How long a floating number lasts, and how far it drifts up. */
constexpr double kPopupLife = 1.1;
constexpr double kPopupRise = 26;

constexpr Color kPlate{20, 17, 13, 170};
constexpr Color kSlot{40, 36, 30, 200};
constexpr Color kText = rgb(0xefeadd);

}  // namespace

BeltSlot beltSlotBox(int index, int width, int height, float uiScale) {
	const float slot = 56 * uiScale;
	const float gap = 6 * uiScale;
	const float total = sim::kHotbarSlots * slot + (sim::kHotbarSlots - 1) * gap;
	const float x0 = std::round((width - total) * 0.5f);
	return BeltSlot{x0 + index * (slot + gap), height - slot - 34 * uiScale, slot};
}

int beltSlotUnder(float px, float py, int width, int height, float uiScale) {
	for (int i = 0; i < sim::kHotbarSlots; ++i) {
		const BeltSlot box = beltSlotBox(i, width, height, uiScale);
		if (ui::inside(px, py, box.x, box.y, box.size, box.size)) return i;
	}
	return -1;
}

void Hud::say(const std::string& text, double x, double y, Color color) {
	popups_.push_back(Popup{text, x, y, kPopupLife, color});
}

void Hud::setAmmo(int carried, int loaded, double reloading, double bowDraw) {
	carried_ = carried;
	loaded_ = loaded;
	reloading_ = reloading;
	bowDraw_ = bowDraw;
}

void Hud::setVitals(double calories, double hydration, double temperature, double radiation,
					double bleeding, double applying) {
	calories_ = calories;
	hydration_ = hydration;
	temperature_ = temperature;
	radiation_ = radiation;
	bleeding_ = bleeding;
	applying_ = applying;
}

void Hud::update(double dt) {
	for (Popup& p : popups_) p.life -= dt;
	popups_.erase(std::remove_if(popups_.begin(), popups_.end(),
								 [](const Popup& p) { return p.life <= 0; }),
				  popups_.end());
}

void Hud::drawPopups(SDL_Renderer*, double cameraX, double cameraY, double scale, int width,
					 int height) const {
	for (const Popup& p : popups_) {
		const double t = 1 - p.life / kPopupLife;
		const float sx = static_cast<float>((p.x - cameraX) * scale) + width * 0.5f;
		const float sy = static_cast<float>((p.y - cameraY - t * kPopupRise) * scale) + height * 0.5f;
		const std::uint8_t fade = static_cast<std::uint8_t>(255 * std::min(1.0, p.life / 0.4));
		if (lettering_) {
			// White lettering inside a black line, whatever it says: a
			// coloured word is lost against grass, sand or water in turn.
			lettering_->drawInked(p.text, sx, sy, 11 * static_cast<float>(scale),
								  Color{p.color.r, p.color.g, p.color.b, fade}, Face::BodyBold,
								  Align::Centre, 2);
		}
	}
}

void Hud::draw(Paint& paint, const sim::Inventory& inventory, int health, int width, int height,
			   const char* prompt, float uiScale) const {
	SDL_Renderer* renderer = paint.renderer();
	// The belt: the same dark glass as the panels, rounded, with the slot you
	// are holding lit at its edge.
	const BeltSlot first = beltSlotBox(0, width, height, uiScale);
	const float slot = first.size;
	const float gap = 6 * uiScale;
	const float total = sim::kHotbarSlots * slot + (sim::kHotbarSlots - 1) * gap;
	const float x0 = first.x;
	const float y0 = first.y;

	float mouseX = 0;
	float mouseY = 0;
	SDL_GetMouseState(&mouseX, &mouseY);
	mouseX *= uiScale;
	mouseY *= uiScale;

	for (int i = 0; i < sim::kHotbarSlots; ++i) {
		const float x = beltSlotBox(i, width, height, uiScale).x;
		const bool active = i == inventory.activeSlot();
		const bool hovered = ui::inside(mouseX, mouseY, x, y0, slot, slot);
		paint.fillRoundRect(x, y0, slot, slot, ui::kRadiusSmall,
							active ? Color{26, 30, 26, 184} : ui::kGlass);
		paint.outlineRoundRect(x, y0, slot, slot, ui::kRadiusSmall, 1.5f * uiScale,
							   active     ? ui::kAccentInk
							   : hovered  ? Color{120, 180, 255, 178}
										  : ui::kGlassEdge);

		char number[4];
		SDL_snprintf(number, sizeof(number), "%d", i + 1);
		if (lettering_) {
			lettering_->draw(number, x + 5 * uiScale, y0 + 2 * uiScale, 11 * uiScale,
							 active ? ui::kAccentInk : ui::kFaint);
		}

		const sim::ItemStack& stack = inventory.hotbar()[i];
		if (stack.id == sim::ItemId::None) continue;
		drawItemIcon(paint, stack.id, x + slot * 0.5f, y0 + slot * 0.5f, slot - 20 * uiScale);
		if (stack.count > 1 && lettering_) {
			char count[8];
			SDL_snprintf(count, sizeof(count), "%d", stack.count);
			lettering_->draw(count, x + slot - 5 * uiScale, y0 + slot - 18 * uiScale,
							 11 * uiScale, ui::kInk, Face::BodyBold, Align::Right);
		}
		// What is in the gun, and nothing else: what is left in the pack is
		// already on the slot's own count and in the pack screen, and printing
		// it a third time here said nothing you could act on. Only the gun
		// actually in your hand: a rifle on the belt used to print the
		// pistol's rounds.
		const sim::Gun& gun = sim::itemDef(stack.id).gun;
		if (sim::isRanged(gun) && active && carried_ >= 0 && lettering_) {
			char ammo[16];
			if (gun.magazine > 0) {
				SDL_snprintf(ammo, sizeof(ammo), "%d", loaded_);
			} else {
				SDL_snprintf(ammo, sizeof(ammo), "%d", std::max(0, carried_));
			}
			const bool dry = gun.magazine > 0 ? loaded_ == 0 : carried_ <= 0;
			lettering_->draw(ammo, x + 5 * uiScale, y0 + slot - 18 * uiScale, 11 * uiScale,
							 dry ? ui::kWarn : ui::kSubtle);
		}
	}

	// The gauges, bottom left, out of the way of the belt: a picture at the
	// head of each where its name used to be, the bar after it, and the number
	// at the far end.
	const float icon = 18 * uiScale;
	const float gaugeX = 20 * uiScale;
	const float barX = gaugeX + icon + 8 * uiScale;
	const float barW = 190 * uiScale - (barX - gaugeX);
	const float rowH = 26 * uiScale;
	const float baseY = height - 134 * uiScale;

	const auto gauge = [&](Vital vital, double value, double max, Color color, int row) {
		const float y = baseY + row * rowH;
		drawVitalIcon(paint, vital, gaugeX + icon / 2, y + 15 * uiScale, icon, color);
		// The number, right-aligned at the end of the bar.
		char amount[8];
		SDL_snprintf(amount, sizeof(amount), "%d", static_cast<int>(std::lround(value)));
		if (lettering_) {
			lettering_->drawInked(amount, gaugeX + 190 * uiScale, y - 3 * uiScale, 13 * uiScale,
								  rgb(0xefeadd), Face::BodyBold, Align::Right, 1);
		}
		// The bar itself, inked like everything else on the page.
		const float h = 7 * uiScale;
		const float top = y + 14 * uiScale;
		paint.fillRect(barX - 2 * uiScale, top - 2 * uiScale, barW + 4 * uiScale,
					   h + 4 * uiScale, kInk);
		paint.fillRect(barX, top, barW, h, kPlate);
		paint.fillRect(barX, top, static_cast<float>(barW * std::clamp(value / max, 0.0, 1.0)), h,
					   color);
	};

	// While you bleed the health bar throbs between its own red and a darker
	// one, so the gauge itself says it is draining.
	const double pulse = 0.5 + 0.5 * std::sin(clock_ * 7);
	const Color healthColor = bleeding_ ? (pulse > 0.5 ? rgb(0x7a1c1c) : rgb(0xd8483a))
							  : health > 35 ? rgb(0xd8483a)
											: rgb(0xff6a5a);
	gauge(Vital::Health, health, 100, healthColor, 0);
	// Dark orange: the lighter one read as a warning rather than a meal.
	gauge(Vital::Food, calories_, 100, rgb(0xcf6a12), 1);
	gauge(Vital::Water, hydration_, 100, rgb(0x4a9ee8), 2);

	if (bleeding_ > 0) {
		// A drop beside the gauges, beating, and beside it what the wound is
		// still going to cost. A bleed takes one health a second, so the
		// seconds left and the health left to lose are the same number, and it
		// is written as health because that is what you are deciding about
		// when you look at it.
		drawVitalIcon(paint, Vital::Water, gaugeX + 208 * uiScale, baseY + 15 * uiScale,
					  icon * static_cast<float>(0.9 + pulse * 0.2), rgb(0x7a1c1c));
		char cost[16];
		SDL_snprintf(cost, sizeof(cost), "-%d",
					 static_cast<int>(std::ceil(bleeding_ * sim::PlayerVitals::kBleedDamage)));
		if (lettering_) {
			lettering_->drawInked(cost, gaugeX + 224 * uiScale, baseY + 7 * uiScale,
								  13 * uiScale, rgb(0xd8483a), Face::BodyBold, Align::Left, 1);
		}
	}

	{
		// The conditions, as one quiet line, and only when they apply.
		char line[128] = {0};
		SDL_snprintf(line, sizeof(line), "%d degrees", static_cast<int>(std::lround(temperature_)));
		if (radiation_ > 1) {
			char rads[32];
			SDL_snprintf(rads, sizeof(rads), "   rad %d",
						 static_cast<int>(std::lround(radiation_)));
			SDL_strlcat(line, rads, sizeof(line));
		}
		if (temperature_ < -2) SDL_strlcat(line, "   freezing", sizeof(line));
		if (lettering_) {
			lettering_->drawInked(line, gaugeX, baseY + 3 * rowH + 2 * uiScale, 11 * uiScale,
								  Color{190, 190, 178, 255}, Face::Body, Align::Left, 1);
		}
	}

	if (prompt && prompt[0]) {
		// One line, over the belt: what the key under your finger would do.
		const float size = 16 * uiScale;
		const float textW = lettering_ ? lettering_->widthOf(prompt, size) : 0;
		const float px = (width - textW) * 0.5f;
		const float py = y0 - 42 * uiScale;
		paint.fillRect(px - 12 * uiScale, py - 6 * uiScale, textW + 24 * uiScale,
					   size + 14 * uiScale, kPlate);
		if (lettering_) lettering_->draw(prompt, px, py, size, kText);
	}
}

}  // namespace client
