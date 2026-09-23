#include "terrain.hpp"

#include <cmath>

#include "paint.hpp"
#include "palette.hpp"
#include "sim/rng.hpp"

namespace client {

namespace {

/** The marks that make each ground read as itself, drawn inside one tile. */
void markGround(Paint& paint, sim::Biome biome, int variant, float size) {
    const auto roll = [&](int slot) {
        return static_cast<float>(sim::seeded(static_cast<std::uint64_t>(variant) * 131 + 7, slot));
    };
    switch (biome) {
        case sim::Biome::Water: {
            // Swell lines, which is all open water is in a printed panel.
            for (int i = 0; i < 3; ++i) {
                const float y = (roll(i * 3) * 0.8f + 0.1f) * size;
                const float x = roll(i * 3 + 1) * size * 0.6f;
                paint.line(x, y, x + size * 0.3f, y - size * 0.03f, 2.0f, Color{255, 255, 255, 70});
            }
            break;
        }
        case sim::Biome::Grass:
        case sim::Biome::Forest: {
            // Tufts, leaning the way the tuft beside them does not.
            const int count = biome == sim::Biome::Forest ? 9 : 6;
            for (int i = 0; i < count; ++i) {
                const float x = roll(i * 2) * size;
                const float y = roll(i * 2 + 1) * size;
                const float lean = (roll(i * 2 + 40) - 0.5f) * size * 0.05f;
                paint.line(x, y, x + lean, y - size * 0.07f, 1.8f, Color{20, 17, 13, 90});
            }
            break;
        }
        case sim::Biome::Beach:
        case sim::Biome::SnowBeach:
        case sim::Biome::Desert: {
            // Grains of sand, as a scattering of dots.
            for (int i = 0; i < 14; ++i) {
                const float x = roll(i * 2) * size;
                const float y = roll(i * 2 + 1) * size;
                paint.fillCircle(x, y, 1.4f, Color{20, 17, 13, 55});
            }
            break;
        }
        case sim::Biome::Snow: {
            // Drifts: a few pale strokes, and nothing dark, on snow.
            for (int i = 0; i < 4; ++i) {
                const float x = roll(i * 2) * size;
                const float y = roll(i * 2 + 1) * size;
                paint.line(x, y, x + size * 0.18f, y, 2.2f, Color{255, 255, 255, 150});
            }
            break;
        }
        case sim::Biome::Road: {
            // Gravel.
            for (int i = 0; i < 18; ++i) {
                const float x = roll(i * 2) * size;
                const float y = roll(i * 2 + 1) * size;
                paint.fillCircle(x, y, 1.6f, Color{20, 17, 13, 70});
            }
            break;
        }
    }
}

}  // namespace

Terrain::~Terrain() {
    for (SDL_Texture* t : tiles_) {
        if (t) SDL_DestroyTexture(t);
    }
}

SDL_Texture* Terrain::tile(sim::Biome biome, int variant) {
    const int index = static_cast<int>(biome) * kVariants + variant;
    if (tiles_[index]) return tiles_[index];

    SDL_Texture* texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_TARGET, kTilePixels, kTilePixels);
    if (!texture) return nullptr;
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    // Nearest, because a tile is stamped edge to edge: sampling past its edge
    // pulls in the transparent border and leaves a seam between every square.
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    SDL_Texture* was = SDL_GetRenderTarget(renderer_);
    SDL_SetRenderTarget(renderer_, texture);
    const Color flat = biomeColor(biome);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer_, flat.r, flat.g, flat.b, 255);
    SDL_RenderClear(renderer_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    Paint paint(renderer_);
    markGround(paint, biome, variant, static_cast<float>(kTilePixels));
    SDL_SetRenderTarget(renderer_, was);

    tiles_[index] = texture;
    return texture;
}

void Terrain::draw(const sim::World& world, double cameraX, double cameraY, double zoom,
                   int screenW, int screenH) {
    const double halfW = screenW / (2 * zoom);
    const double halfH = screenH / (2 * zoom);
    const int col0 = static_cast<int>(std::floor((cameraX - halfW) / sim::kBiomeTile)) - 1;
    const int col1 = static_cast<int>(std::floor((cameraX + halfW) / sim::kBiomeTile)) + 1;
    const int row0 = static_cast<int>(std::floor((cameraY - halfH) / sim::kBiomeTile)) - 1;
    const int row1 = static_cast<int>(std::floor((cameraY + halfH) / sim::kBiomeTile)) + 1;
    const float step = static_cast<float>(sim::kBiomeTile * zoom);

    for (int row = row0; row <= row1; ++row) {
        for (int col = col0; col <= col1; ++col) {
            // Off the map is open sea, so the island has a horizon rather than
            // an edge with the void behind it.
            const bool inside = col >= 0 && row >= 0 && col < sim::kBiomeCols && row < sim::kBiomeRows;
            const sim::Biome biome = inside ? world.tile(col, row) : sim::Biome::Water;
            const int variant = static_cast<int>((col * 7 + row * 13) % kVariants);
            SDL_Texture* texture = tile(biome, variant);
            if (!texture) continue;
            const float x = static_cast<float>((col * sim::kBiomeTile - cameraX) * zoom) + screenW * 0.5f;
            const float y = static_cast<float>((row * sim::kBiomeTile - cameraY) * zoom) + screenH * 0.5f;
            // Grown by a pixel: neighbouring tiles land on fractional pixels and
            // a hairline of the frame behind shows through between them.
            const SDL_FRect dst{x, y, step + 1.0f, step + 1.0f};
            SDL_RenderTexture(renderer_, texture, nullptr, &dst);
        }
    }
}

}  // namespace client
