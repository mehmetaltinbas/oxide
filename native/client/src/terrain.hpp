#pragma once

#include <SDL3/SDL.h>

#include <array>

#include "sim/biome.hpp"
#include "sim/world.hpp"

namespace client {

/**
 * The ground.
 *
 * Every biome's tile is painted once into a few variants - the flat colour plus
 * the dots and scratches that make it a printed page rather than a coloured
 * square - and the visible part of the island is then stamped out of them. A
 * screen is a hundred or so of these, so the ground costs nothing worth naming.
 */
class Terrain {
public:
    explicit Terrain(SDL_Renderer* renderer) : renderer_(renderer) {}
    ~Terrain();

    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    /** Draw the island under a view: world point at the centre, and a zoom. */
    void draw(const sim::World& world, double cameraX, double cameraY, double zoom, int screenW,
              int screenH);

private:
    static constexpr int kVariants = 4;
    /** Painted at this many pixels a tile, then scaled to the zoom. */
    static constexpr int kTilePixels = 96;

    SDL_Renderer* renderer_ = nullptr;
    std::array<SDL_Texture*, sim::kBiomeCount * kVariants> tiles_{};

    SDL_Texture* tile(sim::Biome biome, int variant);
};

}  // namespace client
