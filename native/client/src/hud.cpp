#include "hud.hpp"

#include <algorithm>

#include "held.hpp"
#include "palette.hpp"
#include "text.hpp"
#include "vital_icon.hpp"

namespace client {

namespace {

/** How long a floating number lasts, and how far it drifts up. */
constexpr double kPopupLife = 1.1;
constexpr double kPopupRise = 26;

constexpr Color kPlate{20, 17, 13, 170};
constexpr Color kSlot{40, 36, 30, 200};
constexpr Color kSlotActive{232, 226, 212, 235};
constexpr Color kText = rgb(0xefeadd);

}  // namespace

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
                    bool bleeding, double applying) {
    calories_ = calories;
    hydration_ = hydration;
    temperature_ = temperature;
    radiation_ = radiation;
    bleeding_ = bleeding;
    applying_ = applying;
}

void Hud::notify(const std::string& text) {
    // Newest at the bottom, and never more than a handful at once.
    notices_.push_back(Popup{text, 0, 0, 4.0, kText});
    if (notices_.size() > 5) notices_.erase(notices_.begin());
}

void Hud::update(double dt) {
    for (Popup& p : notices_) p.life -= dt;
    notices_.erase(std::remove_if(notices_.begin(), notices_.end(),
                                  [](const Popup& p) { return p.life <= 0; }),
                   notices_.end());
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
            lettering_->draw(p.text, sx, sy, 11 * static_cast<float>(scale),
                             Color{p.color.r, p.color.g, p.color.b, fade}, Face::BodyBold,
                             Align::Centre);
        }
    }
}

void Hud::draw(Paint& paint, const sim::Inventory& inventory, int health, int width, int height,
               const char* prompt, float uiScale) const {
    SDL_Renderer* renderer = paint.renderer();
    const float slot = 64 * uiScale;
    const float gap = 8 * uiScale;
    const float total = sim::kHotbarSlots * slot + (sim::kHotbarSlots - 1) * gap;
    const float x0 = (width - total) * 0.5f;
    const float y0 = height - slot - 28 * uiScale;

    for (int i = 0; i < sim::kHotbarSlots; ++i) {
        const float x = x0 + i * (slot + gap);
        const bool active = i == inventory.activeSlot();
        paint.fillRect(x - 3 * uiScale, y0 - 3 * uiScale, slot + 6 * uiScale, slot + 6 * uiScale, kInk);
        paint.fillRect(x, y0, slot, slot, active ? kSlotActive : kSlot);
        const sim::ItemStack& stack = inventory.hotbar()[i];
        if (stack.id != sim::ItemId::None) {
            drawItemIcon(paint, stack.id, x + slot * 0.5f, y0 + slot * 0.5f, slot * 0.7f);
        }
        // The slot's own number, and how many are in it.
        if (lettering_) {
            char number[4];
            SDL_snprintf(number, sizeof(number), "%d", i + 1);
            lettering_->draw(number, x + 6 * uiScale, y0 + 2 * uiScale, 12 * uiScale,
                             active ? Color{40, 36, 30, 255} : Color{200, 200, 200, 255});
            if (stack.count > 1) {
                // Right-aligned inside the slot: a four-figure stack used to
                // run out over the slot beside it.
                char count[8];
                SDL_snprintf(count, sizeof(count), "%d", stack.count);
                lettering_->draw(count, x + slot - 5 * uiScale, y0 + slot - 22 * uiScale,
                                 15 * uiScale, rgb(0xffffff), Face::BodyBold, Align::Right);
            }
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
            lettering_->draw(amount, gaugeX + 190 * uiScale, y - 3 * uiScale, 13 * uiScale,
                             rgb(0xefeadd), Face::BodyBold, Align::Right);
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
    gauge(Vital::Food, calories_, 100, rgb(0xff8a1c), 1);
    gauge(Vital::Water, hydration_, 100, rgb(0x4a9ee8), 2);

    if (bleeding_) {
        // A drop beside the gauges, beating.
        drawVitalIcon(paint, Vital::Water, gaugeX + 208 * uiScale, baseY + 15 * uiScale,
                      icon * static_cast<float>(0.9 + pulse * 0.2), rgb(0x7a1c1c));
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
            lettering_->draw(line, gaugeX, baseY + 3 * rowH + 2 * uiScale, 11 * uiScale,
                             Color{150, 150, 140, 255});
        }
    }

    if (applying_ > 0) {
        // What is being applied, running down over the belt.
        const float w = 200 * uiScale;
        const float h = 8 * uiScale;
        const float px = (width - w) * 0.5f;
        const float py = y0 - 30 * uiScale;
        paint.fillRect(px - 2, py - 2, w + 4, h + 4, kInk);
        paint.fillRect(px, py, static_cast<float>(w * (1 - applying_)), h, rgb(0x8cf08c));
    }

    if (carried_ >= 0) {
        // Over on the right, where the belt ends: what is in the gun and what
        // is left in the pack for it.
        const float ax = x0 + total + 20 * uiScale;
        const float ay = y0 + slot * 0.5f - 10 * uiScale;
        char ammo[32];
        SDL_snprintf(ammo, sizeof(ammo), "%d / %d", loaded_, carried_ - loaded_);
        if (lettering_) {
            lettering_->draw(ammo, ax, ay, 20 * uiScale, rgb(0xffffff), Face::Display);
        }
    }
    if (reloading_ > 0 || bowDraw_ > 0) {
        // A sliver over the belt: the reload running down, or the draw coming
        // up to full.
        const float w = 200 * uiScale;
        const float h = 8 * uiScale;
        const float px = (width - w) * 0.5f;
        const float py = y0 - 18 * uiScale;
        const double progress = reloading_ > 0 ? 1 - reloading_ : std::min(1.0, bowDraw_);
        paint.fillRect(px - 2, py - 2, w + 4, h + 4, kInk);
        paint.fillRect(px, py, static_cast<float>(w * progress), h, rgb(0xe8c87a));
    }

    // What just happened, stacked in the top left under the frame counter.
    for (std::size_t i = 0; i < notices_.size(); ++i) {
        const Popup& notice = notices_[i];
        const std::uint8_t fade =
            static_cast<std::uint8_t>(255 * std::min(1.0, notice.life / 0.8));
        if (lettering_) {
            lettering_->draw(notice.text, 20 * uiScale, 36 * uiScale + i * 20 * uiScale,
                             13 * uiScale, Color{kText.r, kText.g, kText.b, fade});
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
