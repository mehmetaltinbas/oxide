#include "monument_draw.hpp"

#include <SDL3/SDL.h>

#include <cmath>

#include "palette.hpp"
#include "text.hpp"

namespace client {

namespace {

constexpr float kTau = 6.28318530718f;

Color groundOf(sim::MonumentKind kind) {
    switch (kind) {
        case sim::MonumentKind::Cabins: return rgb(0x6b6250);
        case sim::MonumentKind::Lighthouse: return rgb(0x8a8f96);
        case sim::MonumentKind::Airfield: return rgb(0x6f6f68);
        case sim::MonumentKind::PowerPlant: return rgb(0x5f6663);
        case sim::MonumentKind::Military: return rgb(0x5a5f4a);
        case sim::MonumentKind::Town: return rgb(0x7a6a58);
    }
    return rgb(0x6b6250);
}

}  // namespace

void drawMonument(Paint& paint, const sim::Monument& monument, double cameraX, double cameraY,
                  double scale, int width, int height, float uiScale) {
    const sim::MonumentDef& def = sim::monumentDef(monument.kind);
    const float x = static_cast<float>((monument.x - cameraX) * scale) + width * 0.5f;
    const float y = static_cast<float>((monument.y - cameraY) * scale) + height * 0.5f;
    const float r = static_cast<float>(monument.radius * scale);

    Color ground = groundOf(monument.kind);
    ground.a = 140;
    paint.fillCircle(x, y, r, ground);

    // Concrete slabs, so it reads as a built-up place rather than a stain.
    const double seed = monument.x + monument.y;
    for (int i = 0; i < 9; ++i) {
        const float a = static_cast<float>(std::fmod(seed + i * 97, 360) / 360.0 * kTau);
        const float d = static_cast<float>(std::fmod(seed + i * 53, 100) / 100.0) * r * 0.7f;
        const float w = (40 + static_cast<float>(std::fmod(seed + i * 31, 70))) *
                        static_cast<float>(scale);
        paint.fillRect(x + std::cos(a) * d - w / 2, y + std::sin(a) * d - w / 3, w, w * 0.66f,
                       rgb(0x55564f));
    }

    if (def.rads > 0) {
        // A dashed ring, which is the only warning you get.
        for (int i = 0; i < 48; ++i) {
            if (i % 2) continue;
            const float a0 = static_cast<float>(i) / 48 * kTau;
            const float a1 = static_cast<float>(i + 1) / 48 * kTau;
            paint.line(x + std::cos(a0) * r, y + std::sin(a0) * r, x + std::cos(a1) * r,
                       y + std::sin(a1) * r, 3 * static_cast<float>(scale),
                       Color{180, 230, 90, 140});
        }
    }

    // The name across the top of it, in the shouting face: a monument is meant
    // to be read from across the island.
    if (Text* lettering = paint.text()) {
        char line[64];
        SDL_snprintf(line, sizeof(line), "%s%s", def.rads > 0 ? "! " : "", def.name);
        lettering->draw(line, x, y - r + 12 * uiScale, 22 * uiScale,
                        def.rads > 0 ? Color{180, 230, 90, 220} : Color{230, 228, 210, 200},
                        Face::Display, Align::Centre);
    }
}

void drawCrate(Paint& paint, const sim::LootCrate&, float x, float y, float scale) {
    const float w = 14 * scale;
    const float h = 11 * scale;
    paint.inkedPoly({{x - w, y - h}, {x + w, y - h}, {x + w, y + h}, {x - w, y + h}}, rgb(0x7a6a44),
                    kInkWidth);
    // The band round it, which is what says loot rather than box.
    paint.fillRect(x - w, y - h * 0.3f, w * 2, h * 0.36f, rgb(0xc9a227));
}

}  // namespace client
