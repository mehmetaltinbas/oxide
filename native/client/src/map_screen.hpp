#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "paint.hpp"
#include "sim/build.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace client {

/**
 * The island on one screen: the whole of it at its own shape, Rust's lettered
 * grid over the top, every monument named, what you have put down, and you.
 *
 * Baked once into a texture from the biome grid, because the map is the same
 * island every time it is opened.
 */
class MapScreen {
public:
    explicit MapScreen(SDL_Renderer* renderer) : renderer_(renderer) {}
    ~MapScreen();

    MapScreen(const MapScreen&) = delete;
    MapScreen& operator=(const MapScreen&) = delete;

    bool open() const { return open_; }
    void toggle() { open_ = !open_; }
    void close() { open_ = false; }

    void draw(Paint& paint, const sim::World& world, const sim::BuildSystem& build,
              const sim::Player& player, int width, int height, float uiScale);

    /** A mate to mark, called once for each before the map is drawn. */
    void addMate(double x, double y) { mates_.push_back({x, y}); }

    /** The square a point is in, as "F7". */
    static void squareOf(double x, double y, char* out, int size);

private:
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* island_ = nullptr;
    bool open_ = false;

    SDL_Texture* island(const sim::World& world);

    struct Mate {
        double x;
        double y;
    };
    std::vector<Mate> mates_;
};

}  // namespace client
