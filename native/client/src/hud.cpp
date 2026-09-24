#include "hud.hpp"

#include <algorithm>

#include "held.hpp"
#include "palette.hpp"

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

void Hud::drawPopups(SDL_Renderer* renderer, double cameraX, double cameraY, double scale,
                     int width, int height) const {
    for (const Popup& p : popups_) {
        const double t = 1 - p.life / kPopupLife;
        const float sx = static_cast<float>((p.x - cameraX) * scale) + width * 0.5f;
        const float sy = static_cast<float>((p.y - cameraY - t * kPopupRise) * scale) + height * 0.5f;
        const std::uint8_t fade = static_cast<std::uint8_t>(255 * std::min(1.0, p.life / 0.4));
        SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, fade);
        SDL_SetRenderScale(renderer, 2.0f, 2.0f);
        SDL_RenderDebugText(renderer, sx * 0.5f, sy * 0.5f, p.text.c_str());
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
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
        const float digits = uiScale;
        SDL_SetRenderScale(renderer, digits, digits);
        SDL_SetRenderDrawColor(renderer, active ? 40 : 200, active ? 36 : 200, active ? 30 : 200, 255);
        SDL_RenderDebugTextFormat(renderer, (x + 6 * uiScale) / digits, (y0 + 6 * uiScale) / digits,
                                  "%d", i + 1);
        if (stack.count > 1) {
            // Right-aligned inside the slot: a four-figure stack used to run
            // out over the slot beside it.
            char count[8];
            SDL_snprintf(count, sizeof(count), "%d", stack.count);
            const float size = digits * 1.6f;
            const float textW = static_cast<float>(SDL_strlen(count)) * 8 * size;
            SDL_SetRenderScale(renderer, size, size);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, (x + slot - 5 * uiScale - textW) / size,
                                (y0 + slot - 20 * uiScale) / size, count);
        }
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    }

    // Health, along the bottom left, where it is out of the way of the belt.
    const float barW = 240 * uiScale;
    const float barH = 18 * uiScale;
    const float bx = 28 * uiScale;
    const float by = height - barH - 28 * uiScale;
    paint.fillRect(bx - 3 * uiScale, by - 3 * uiScale, barW + 6 * uiScale, barH + 6 * uiScale, kInk);
    paint.fillRect(bx, by, barW, barH, kPlate);
    const float fill = barW * std::max(0, std::min(100, health)) / 100.0f;
    paint.fillRect(bx, by, fill, barH, rgb(0x8cf08c));
    SDL_SetRenderScale(renderer, uiScale, uiScale);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugTextFormat(renderer, (bx + 8 * uiScale) / uiScale, (by + 5 * uiScale) / uiScale,
                              "%d", health);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    // Food and water under the health bar, and the cold and the radiation only
    // when they are worth knowing about.
    const auto bar = [&](float row, double value, double max, Color color) {
        const float h = 12 * uiScale;
        const float y = by - row * (h + 5 * uiScale);
        paint.fillRect(bx - 3 * uiScale, y - 3 * uiScale, barW + 6 * uiScale, h + 6 * uiScale, kInk);
        paint.fillRect(bx, y, barW, h, kPlate);
        paint.fillRect(bx, y, static_cast<float>(barW * std::clamp(value / max, 0.0, 1.0)), h,
                       color);
    };
    bar(1, calories_, 100, rgb(0xd8a24a));
    bar(2, hydration_, 100, rgb(0x5aa8d8));
    if (radiation_ > 1) bar(3, radiation_, 100, rgb(0xb4e65a));

    {
        // The thermometer, as a word: a number in degrees says nothing at a
        // glance about whether you are about to freeze.
        const char* how = temperature_ < -2  ? "FREEZING"
                          : temperature_ < 8 ? "COLD"
                          : temperature_ > 34 ? "SWELTERING"
                                              : "";
        if (how[0]) {
            SDL_SetRenderScale(renderer, 1.6f * uiScale, 1.6f * uiScale);
            SDL_SetRenderDrawColor(renderer, 150, 200, 255, 255);
            SDL_RenderDebugText(renderer, (bx) / (1.6f * uiScale),
                                (by - 4 * (12 * uiScale + 5 * uiScale)) / (1.6f * uiScale), how);
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        }
        if (bleeding_) {
            SDL_SetRenderScale(renderer, 1.6f * uiScale, 1.6f * uiScale);
            SDL_SetRenderDrawColor(renderer, 216, 72, 58, 255);
            SDL_RenderDebugText(renderer, (bx + 120 * uiScale) / (1.6f * uiScale),
                                (by - 4 * (12 * uiScale + 5 * uiScale)) / (1.6f * uiScale),
                                "BLEEDING");
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
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
        SDL_SetRenderScale(renderer, 2.0f * uiScale, 2.0f * uiScale);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugTextFormat(renderer, ax / (2.0f * uiScale), ay / (2.0f * uiScale), "%d / %d",
                                  loaded_, carried_ - loaded_);
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
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
        const float size = 1.5f * uiScale;
        const std::uint8_t fade =
            static_cast<std::uint8_t>(255 * std::min(1.0, notice.life / 0.8));
        SDL_SetRenderScale(renderer, size, size);
        SDL_SetRenderDrawColor(renderer, kText.r, kText.g, kText.b, fade);
        SDL_RenderDebugText(renderer, (20 * uiScale) / size,
                            (40 * uiScale + i * 18 * uiScale) / size, notice.text.c_str());
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    }

    if (prompt && prompt[0]) {
        // One line, over the belt: what the key under your finger would do.
        const float scale = 2.0f * uiScale;
        const float textW = static_cast<float>(SDL_strlen(prompt)) * 8 * scale;
        const float px = (width - textW) * 0.5f;
        const float py = y0 - 40 * uiScale;
        paint.fillRect(px - 12 * uiScale, py - 8 * uiScale, textW + 24 * uiScale,
                       16 * scale + 16 * uiScale, kPlate);
        SDL_SetRenderScale(renderer, scale, scale);
        SDL_SetRenderDrawColor(renderer, kText.r, kText.g, kText.b, 255);
        SDL_RenderDebugText(renderer, px / scale, py / scale, prompt);
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    }
}

}  // namespace client
