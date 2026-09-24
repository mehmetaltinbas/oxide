#include "map_screen.hpp"

#include <algorithm>
#include <cmath>

#include "palette.hpp"

namespace client {

namespace {

/**
 * How many squares fit along the map's longest side. The short side gets
 * however many fit at the same size, so a square is always square whatever
 * shape the island is.
 */
constexpr int kCellsAcross = 15;
constexpr double kGridSize =
    (sim::kWorldWidth > sim::kWorldHeight ? sim::kWorldWidth : sim::kWorldHeight) / kCellsAcross;

void text(SDL_Renderer* renderer, float x, float y, float scale, Color color, const char* line) {
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDebugText(renderer, x / scale, y / scale, line);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

}  // namespace

MapScreen::~MapScreen() {
    if (island_) SDL_DestroyTexture(island_);
}

void MapScreen::squareOf(double x, double y, char* out, int size) {
    const int col = std::clamp(static_cast<int>(x / kGridSize), 0, kCellsAcross - 1);
    const int row = std::clamp(static_cast<int>(y / kGridSize), 0, kCellsAcross - 1);
    SDL_snprintf(out, size, "%c%d", static_cast<char>('A' + col), row);
}

SDL_Texture* MapScreen::island(const sim::World& world) {
    if (island_) return island_;
    // One pixel a biome tile: the island as the generator sees it.
    island_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                sim::kBiomeCols, sim::kBiomeRows);
    if (!island_) return nullptr;
    SDL_SetTextureBlendMode(island_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(island_, SDL_SCALEMODE_NEAREST);
    SDL_Texture* was = SDL_GetRenderTarget(renderer_);
    SDL_SetRenderTarget(renderer_, island_);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    for (int row = 0; row < sim::kBiomeRows; ++row) {
        for (int col = 0; col < sim::kBiomeCols; ++col) {
            const Color c = biomeColor(world.tile(col, row));
            SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, 255);
            SDL_RenderPoint(renderer_, static_cast<float>(col), static_cast<float>(row));
        }
    }
    SDL_SetRenderTarget(renderer_, was);
    return island_;
}

void MapScreen::draw(Paint& paint, const sim::World& world, const sim::BuildSystem& build,
                     const sim::Player& player, int width, int height, float uiScale) {
    if (!open_) return;
    SDL_Renderer* renderer = paint.renderer();

    // The island at its own shape, as big as the window allows.
    const float longest = std::max(200.0f, std::min(width, height) - 190 * uiScale);
    const float mapW = sim::kWorldWidth >= sim::kWorldHeight
                           ? longest
                           : longest * sim::kWorldWidth / sim::kWorldHeight;
    const float mapH = sim::kWorldHeight >= sim::kWorldWidth
                           ? longest
                           : longest * sim::kWorldHeight / sim::kWorldWidth;
    const float x = (width - mapW) * 0.5f;
    const float y = (height - mapH) * 0.5f;
    const float scale = mapW / sim::kWorldWidth;

    paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
                   Color{0, 0, 0, 150});
    paint.fillRect(x - 6 * uiScale, y - 6 * uiScale, mapW + 12 * uiScale, mapH + 12 * uiScale,
                   kInk);
    if (SDL_Texture* baked = island(world)) {
        const SDL_FRect dst{x, y, mapW, mapH};
        SDL_RenderTexture(renderer, baked, nullptr, &dst);
    }

    // Rust's grid over the top, each square named in its corner.
    const float cell = static_cast<float>(kGridSize) * scale;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 17, 13, 110);
    for (int i = 1; i < kCellsAcross; ++i) {
        SDL_RenderLine(renderer, x + i * cell, y, x + i * cell, y + mapH);
        SDL_RenderLine(renderer, x, y + i * cell, x + mapW, y + i * cell);
    }
    // Exactly as many squares as the island has, not one more: the rounding
    // used to leave a column of labels hanging off the edge.
    const int cols = static_cast<int>(std::ceil(sim::kWorldWidth / kGridSize));
    const int rows = static_cast<int>(std::ceil(sim::kWorldHeight / kGridSize));
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            char label[8];
            SDL_snprintf(label, sizeof(label), "%c%d", static_cast<char>('A' + col), row);
            text(renderer, x + col * cell + 3 * uiScale, y + row * cell + 3 * uiScale, uiScale,
                 Color{240, 235, 221, 150}, label);
        }
    }

    // What you have put down, in lamplight yellow.
    for (const sim::Deployable& thing : build.deployables()) {
        paint.fillRect(x + static_cast<float>(thing.x) * scale - 2 * uiScale,
                       y + static_cast<float>(thing.y) * scale - 2 * uiScale, 4 * uiScale,
                       4 * uiScale, rgb(0xe8c87a));
    }

    // The monuments, named: a dot you have to guess at is no use for deciding
    // where to go next, and the hot ones are written in the hazard green.
    for (const sim::Monument& monument : world.monuments()) {
        const sim::MonumentDef& def = sim::monumentDef(monument.kind);
        const bool hot = def.rads > 0;
        const float mx = x + static_cast<float>(monument.x) * scale;
        const float my = y + static_cast<float>(monument.y) * scale;
        paint.inkedCircle(mx, my, 4 * uiScale, hot ? rgb(0xb4e65a) : rgb(0xefeadd), kInkFine);
        const float size = 1.1f * uiScale;
        const float textW = static_cast<float>(SDL_strlen(def.name)) * 8 * size;
        text(renderer, mx - textW / 2, my + 7 * uiScale, size,
             hot ? rgb(0xb4e65a) : rgb(0xefeadd), def.name);
    }

    // You, and which way you are facing.
    const float px = x + static_cast<float>(player.x) * scale;
    const float py = y + static_cast<float>(player.y) * scale;
    paint.inkedCircle(px, py, 5 * uiScale, rgb(0x7cc8ff), kInkFine);
    paint.line(px, py, px + std::cos(static_cast<float>(player.aim)) * 12 * uiScale,
               py + std::sin(static_cast<float>(player.aim)) * 12 * uiScale, 2 * uiScale,
               rgb(0x7cc8ff));

    char square[8];
    squareOf(player.x, player.y, square, sizeof(square));
    char line[64];
    SDL_snprintf(line, sizeof(line), "MAP    you are in %s    M to close", square);
    text(renderer, x, y - 26 * uiScale, 1.6f * uiScale, rgb(0xefeadd), line);
}

}  // namespace client
