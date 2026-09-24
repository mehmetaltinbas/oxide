#include "sprites.hpp"

#include <cmath>

#include "palette.hpp"
#include "sim/rng.hpp"

namespace client {

namespace {

constexpr float kTau = 6.28318530718f;

Color darken(Color c, float amount = 0.72f) {
    return Color{static_cast<std::uint8_t>(c.r * amount), static_cast<std::uint8_t>(c.g * amount),
                 static_cast<std::uint8_t>(c.b * amount), c.a};
}

float v(int variant, int slot) {
    return static_cast<float>(sim::seeded(static_cast<std::uint64_t>(variant) * 7919 + 13, slot));
}

/** The pen, at the weight trees and ore are inked with. */
constexpr float kPen = kInkWidth * kNodeLineScale;

/**
 * The ink inside one tier of a conifer.
 *
 * Not an outline: the pen already drew that. These are the marks that say what
 * the shape is made of, which for a pine is the fishbone of its branches and
 * hatching down the side the light is not on.
 */
void markTier(Paint& paint, float cx, float y, float r, int variant) {
    const float apexY = y - r;
    const float baseY = y + r * 0.5f;
    const float height = baseY - apexY;

    // Branches, two rows, the lower one wider: a tier spreads as it drops.
    for (const float t : {0.42f, 0.7f}) {
        const float py = apexY + height * t;
        const float reach = r * t * 0.62f;
        const float drop = reach * 0.42f;
        const float lean = static_cast<float>((variant % 3) - 1) * 0.6f;
        paint.line(cx + lean, py, cx - reach + lean, py + drop, kInkMark, kInk);
        paint.line(cx + lean, py, cx + reach + lean, py + drop, kInkMark, kInk);
    }

    // Hatching down the right flank, which is how a press says the light comes
    // from the other side. Clipped to the tier by hand: each stroke is cut off
    // where the tier's own edge is at that height.
    const float step = 9 * 0.75f;
    for (float hx = r * 0.18f; hx < r; hx += step) {
        const float x0 = cx + hx;
        const float y0 = baseY;
        float x1 = cx + hx + height * 0.45f;
        float y1 = apexY + height * 0.25f;
        // The tier's edge runs from the apex out to the base corner; walk the
        // stroke back until it is inside it.
        for (int i = 0; i < 8; ++i) {
            const float t = (y1 - apexY) / height;
            const float halfAt = r * t;
            if (std::abs(x1 - cx) <= halfAt) break;
            x1 = x1 * 0.85f + x0 * 0.15f;
            y1 = y1 * 0.85f + y0 * 0.15f;
        }
        if (std::abs(x0 - cx) > r) continue;
        paint.line(x0, y0, x1, y1, kInkFine, kInk);
    }
}

/** A pine: three tiers, the crown in front, leaning its own way. */
void paintPine(Paint& paint, float ox, float oy, float r, int variant, bool snowy) {
    const float wide = 0.88f + v(variant, 1) * 0.2f;
    const float tall = 0.9f + v(variant, 2) * 0.18f;
    const float lean = (v(variant, 3) - 0.5f) * r * 0.16f;
    const float trunk = 0.8f + v(variant, 4) * 0.4f;

    paint.fillRect(ox - r * 0.22f * trunk, oy - r * 0.5f, r * 0.44f * trunk, r, rgb(0x7b4a26));
    // Grain up the trunk, so it is timber rather than a brown bar.
    paint.line(ox - r * 0.08f * trunk, oy - r * 0.42f, ox - r * 0.08f * trunk, oy + r * 0.42f,
               kInkMark * 0.6f, kInk);
    paint.line(ox + r * 0.1f * trunk, oy - r * 0.3f, ox + r * 0.1f * trunk, oy + r * 0.36f,
               kInkMark * 0.6f, kInk);

    struct Tier {
        float y;
        float r;
        float dx;
    };
    const Tier tiers[3] = {
        {oy - r * 2.0f * tall, r * 1.0f * wide * (0.92f + v(variant, 5) * 0.16f), lean * 2},
        {oy - r * 1.35f * tall, r * 1.3f * wide * (0.92f + v(variant, 6) * 0.16f), lean},
        {oy - r * 0.65f * tall, r * 1.55f * wide * (0.94f + v(variant, 7) * 0.1f), 0},
    };
    // Bottom tier first, crown last: a conifer's top sits in front of the skirt
    // below it, and painting downward buried every crown.
    for (int i = 2; i >= 0; --i) {
        const Tier& t = tiers[i];
        const float x = ox + t.dx;
        const std::vector<Point> shape{
            {x, t.y - t.r}, {x - t.r, t.y + t.r * 0.5f}, {x + t.r, t.y + t.r * 0.5f}};
        paint.inkedPoly(shape, i % 2 == 0 ? nodeColor(sim::NodeKind::Tree) : rgb(0x5cc063), kPen);
        markTier(paint, x, t.y, t.r, variant + i);
        if (snowy) {
            // The crown gets a cap; a lower tier's top is under the tier above,
            // so its snow lies on the shelf that shows.
            const float shelf = i > 0 ? tiers[i - 1].y + tiers[i - 1].r * 0.5f : t.y - t.r;
            const float top = i > 0 ? shelf : t.y - t.r;
            const float depth = (t.y + t.r * 0.5f - top) * (i > 0 ? 0.35f : 0.42f);
            const float halfAt = [&](float y) {
                return t.r * ((y - (t.y - t.r)) / (t.r * 1.5f));
            }(top + depth);
            const float halfTop = i > 0 ? t.r * ((top - (t.y - t.r)) / (t.r * 1.5f)) : 0.0f;
            paint.inkedPoly({{x - halfTop, top},
                             {x + halfTop, top},
                             {x + halfAt, top + depth},
                             {x - halfAt * 0.8f, top + depth * 0.8f}},
                            kSnow, kInkFine);
        }
    }
}

/** A broadleaf, for the open grassland: a round crown of leaf clumps. */
void paintBroadleaf(Paint& paint, float ox, float oy, float r, int variant) {
    const float size = 0.9f + v(variant, 1) * 0.18f;
    const float lean = (v(variant, 2) - 0.5f) * r * 0.2f;
    const float cx = ox + lean;
    const float cy = oy - r * 1.85f * size;
    const float cr = r * 1.3f * size;

    paint.inkedPoly({{ox - r * 0.2f, oy + r * 0.4f},
                     {ox + r * 0.2f, oy + r * 0.4f},
                     {cx + r * 0.14f, cy + cr * 0.4f},
                     {cx - r * 0.14f, cy + cr * 0.4f}},
                    rgb(0x7b4a26), kPen);

    const int clumps = 6 + static_cast<int>(v(variant, 3) * 3);
    struct Clump {
        float x;
        float y;
        float r;
    };
    std::vector<Clump> pts;
    for (int i = 0; i < clumps; ++i) {
        const float a = static_cast<float>(i) / clumps * kTau + v(variant, 4) * kTau +
                        (v(variant, 50 + i) - 0.5f) * 0.6f;
        const float d = cr * (0.4f + v(variant, 10 + i) * 0.4f);
        const float rr = cr * (0.3f + v(variant, 20 + i) * 0.25f);
        pts.push_back({cx + std::cos(a) * d * 1.1f, cy + std::sin(a) * d * 0.78f, rr});
    }
    pts.push_back({cx, cy, cr * 0.55f});
    // The pen round the silhouette only: every clump inked heavily first, then
    // the fills laid over the top, which buries the inner half of each line.
    for (const Clump& c : pts) paint.fillCircle(c.x, c.y, c.r + kPen, kInk);
    for (const Clump& c : pts) paint.fillCircle(c.x, c.y, c.r, kBroadleafDark);
    // The lit clumps, up and to the left, in the lighter green.
    for (const Clump& c : pts) {
        if (c.x - cx + (c.y - cy) > cr * 0.25f) continue;
        paint.fillCircle(c.x - c.r * 0.12f, c.y - c.r * 0.12f, c.r * 0.82f, kBroadleafLight);
    }
}

/** A boulder or an ore node: one lumpy outline with its facets marked inside. */
void paintStone(Paint& paint, float ox, float oy, float r, sim::NodeKind kind, int variant,
                bool snowy) {
    const int points = 7;
    std::vector<Point> pts;
    for (int i = 0; i < points; ++i) {
        const float a = static_cast<float>(i) / points * kTau;
        const float wob = 0.75f + v(variant, 30 + i) * 0.45f;
        pts.push_back({ox + std::cos(a) * r * wob, oy + std::sin(a) * r * 0.78f * wob});
    }
    paint.inkedPoly(pts, nodeColor(kind), kPen);
    // The lit face. Light, not an object, so it has no line round it.
    paint.fillPoly(circlePoints(ox - r * 0.2f, oy - r * 0.25f, r * 0.3f),
                   Color{255, 255, 255, 40});
    // Stipple down the side away from the light, which is how a press shades.
    const float step = 9 * 0.55f;
    for (float sy = -r; sy < r; sy += step) {
        for (float sx = -r; sx < r; sx += step) {
            const float shade = (sx + sy) / (r * 2);
            if (shade < 0.12f) continue;
            if (sx * sx + sy * sy * 1.6f > r * r * 0.82f) continue;
            const float off = (static_cast<int>((sy + r) / step) % 2) * step / 2;
            paint.fillCircle(ox + sx + off, oy + sy, 1.4f * (0.6f + shade * 0.9f), kInk);
        }
    }
    if (snowy) {
        // Snow lies on the top half, following the rock's own edge.
        std::vector<Point> cap;
        for (const Point& p : pts) {
            if (p.y <= oy) cap.push_back(p);
        }
        if (cap.size() >= 3) paint.fillPoly(cap, kSnow);
    }
    // Facets, from a ridge near the top out to every other corner.
    const float ridgeX = ox - r * 0.12f;
    const float ridgeY = oy - r * 0.14f;
    for (std::size_t i = variant % 2; i < pts.size(); i += 2) {
        paint.line(ridgeX, ridgeY, pts[i].x * 0.85f + ox * 0.15f, pts[i].y * 0.85f + oy * 0.15f,
                   kInkMark, kInk);
    }
    // Ore: flecks of what it is worth, chipped rather than round.
    if (kind == sim::NodeKind::Metal || kind == sim::NodeKind::Sulfur) {
        const Color fleck = kind == sim::NodeKind::Metal ? rgb(0xe0d0b0) : rgb(0xf4ec9a);
        for (int i = 0; i < 4; ++i) {
            const float a = v(variant, 40 + i) * kTau;
            const float fx = ox + std::cos(a) * r * 0.42f;
            const float fy = oy + std::sin(a) * r * 0.32f;
            const float fr = r * 0.13f;
            paint.fillPoly({{fx - fr, fy - fr * 0.2f},
                            {fx - fr * 0.1f, fy - fr},
                            {fx + fr, fy - fr * 0.1f},
                            {fx + fr * 0.2f, fy + fr}},
                           fleck);
        }
    }
}

/** A nettle clump: seven long serrated leaves in two greens. */
void paintNettle(Paint& paint, float ox, float oy, float r, int variant) {
    const float spread = r * 1.5f;
    const int count = 7;
    const float turn = v(variant, 1) * kTau;
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = pass; i < count; i += 2) {
            const float a = static_cast<float>(i) / count * kTau + turn;
            const float len = spread * (pass == 0 ? 0.95f : 0.8f);
            const float ca = std::cos(a);
            const float sa = std::sin(a);
            const float half = len * 0.26f;
            std::vector<Point> leaf;
            const auto put = [&](float along, float across) {
                leaf.push_back({ox + ca * along - sa * across, oy + sa * along + ca * across});
            };
            put(0, 0);
            const int teeth = 5;
            for (int k = 1; k <= teeth; ++k) {
                const float t = static_cast<float>(k) / teeth;
                const float w = half * std::sin(3.14159265f * t);
                put(len * (t - 0.1f), -w * 1.25f);
                put(len * t, -w * 0.8f);
            }
            for (int k = teeth; k >= 1; --k) {
                const float t = static_cast<float>(k) / teeth;
                const float w = half * std::sin(3.14159265f * t);
                put(len * t, w * 0.8f);
                put(len * (t - 0.1f), w * 1.25f);
            }
            const Color leafColor = nodeColor(sim::NodeKind::Nettle);
            paint.fillPoly(leaf, pass == 0 ? darken(leafColor) : leafColor);
            paint.outlinePoly(leaf, kInkFine * 0.8f, kInk);
        }
    }
}

/** A roadside barrel, seen from above: a lid, its rim, and rust in from it. */
void paintBarrel(Paint& paint, float ox, float oy, float r, int variant) {
    const Color body = v(variant, 1) < 0.5f ? rgb(0x3a6fa8) : rgb(0xb0473a);
    paint.inkedCircle(ox, oy, r, body, kPen);
    paint.fillCircle(ox, oy, r * 0.78f, darken(body));
    for (int k = 0; k < 3; ++k) {
        const float a = v(variant, 2 + k) * kTau;
        paint.fillCircle(ox + std::cos(a) * r * 0.62f, oy + std::sin(a) * r * 0.62f, r * 0.17f,
                         Color{122, 70, 30, 140});
    }
    // The bung caps.
    paint.fillCircle(ox + r * 0.38f, oy - r * 0.2f, r * 0.14f, rgb(0xc9ced4));
    paint.fillCircle(ox - r * 0.42f, oy + r * 0.3f, r * 0.14f, rgb(0xc9ced4));
    paint.outlineCircle(ox, oy, r * 0.9f, kInkFine, kInk);
}

}  // namespace

Sprites::~Sprites() {
    for (Baked& b : baked_) {
        if (b.texture) SDL_DestroyTexture(b.texture);
    }
}

const Sprites::Baked& Sprites::bake(SpriteKey key) {
    const int index = ((static_cast<int>(key.kind) * kVariants + key.variant) * 2 +
                       (key.snowy ? 1 : 0)) * 2 +
                      (key.broadleaf ? 1 : 0);
    Baked& slot = baked_[index];
    if (slot.texture) return slot;

    const float r = kBakeRadius;
    // The box each kind needs around its foot. Trees are tall and stand on the
    // bottom of theirs; everything else is centred on the ground it sits on.
    const bool tree = key.kind == sim::NodeKind::Tree;
    const float halfWidth = tree ? r * 2.4f : r * 2.2f;
    const float above = tree ? r * 4.0f : r * 1.6f;
    const float below = tree ? r * 1.2f : r * 1.6f;

    const int w = static_cast<int>(halfWidth * 2) + 4;
    const int h = static_cast<int>(above + below) + 4;
    slot.texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                     SDL_TEXTUREACCESS_TARGET, w, h);
    if (!slot.texture) return slot;
    SDL_SetTextureBlendMode(slot.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(slot.texture, SDL_SCALEMODE_LINEAR);
    slot.originX = halfWidth + 2;
    slot.originY = above + 2;
    slot.width = static_cast<float>(w);
    slot.height = static_cast<float>(h);

    SDL_Texture* was = SDL_GetRenderTarget(renderer_);
    SDL_SetRenderTarget(renderer_, slot.texture);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);

    Paint paint(renderer_);
    const float ox = slot.originX;
    const float oy = slot.originY;
    switch (key.kind) {
        case sim::NodeKind::Tree:
            if (key.broadleaf) {
                paintBroadleaf(paint, ox, oy, r, key.variant);
            } else {
                paintPine(paint, ox, oy, r, key.variant, key.snowy);
            }
            break;
        case sim::NodeKind::Stone:
        case sim::NodeKind::Metal:
        case sim::NodeKind::Sulfur:
            paintStone(paint, ox, oy, r, key.kind, key.variant, key.snowy);
            break;
        case sim::NodeKind::Nettle: paintNettle(paint, ox, oy, r, key.variant); break;
        case sim::NodeKind::Barrel: paintBarrel(paint, ox, oy, r, key.variant); break;
    }

    SDL_SetRenderTarget(renderer_, was);
    return slot;
}

void Sprites::draw(const sim::ResourceNode& node, float screenX, float screenY, float scale,
                   bool snowy, bool broadleaf, float alpha) {
    SpriteKey key{node.kind, static_cast<int>(node.seed % kVariants), snowy, broadleaf};
    const Baked& b = bake(key);
    if (!b.texture) return;
    const float factor = static_cast<float>(node.radius) / kBakeRadius * scale;
    SDL_FRect dst{screenX - b.originX * factor, screenY - b.originY * factor, b.width * factor,
                  b.height * factor};
    SDL_SetTextureAlphaModFloat(b.texture, alpha);
    SDL_RenderTexture(renderer_, b.texture, nullptr, &dst);
}

}  // namespace client
