#pragma once

#include <SDL3/SDL.h>

#include <array>

#include "client/features/render/systems/paint.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/world/systems/world.hpp"

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
	/**
	 * What cannot be baked into a tile, drawn over the top of them.
	 *
	 * A tile is a texture made once and stamped everywhere it belongs, which
	 * is why the ground is cheap to draw. Two things do not fit in one:
	 *
	 * The swell, because it moves. A lake is not the sea: sheltered water has
	 * a small steady ripple, the open sea a long heavy swell that travels, and
	 * `freshAt` is what tells them apart.
	 *
	 * The ink along a road, because it depends on the tile's neighbours rather
	 * than on the tile, and a texture knows nothing about what it is next to.
	 */
	void drawOver(Paint& paint, const sim::World& world, double cameraX, double cameraY,
				  double zoom, int screenW, int screenH, double clock);

	void draw(const sim::World& world, double cameraX, double cameraY, double zoom, int screenW,
			  int screenH);

	/**
	 * The dot screen over the ground.
	 *
	 * Light enough to read as paper rather than as dirt, and the one thing
	 * that stops a flat biome colour looking like a vector drawing. Laid over
	 * the terrain and under everything that stands on it.
	 */
	void drawScreen(double cameraX, double cameraY, double zoom, int screenW, int screenH);

private:
	static constexpr int kVariants = 4;
	/** Painted at this many pixels a tile, then scaled to the zoom. */
	static constexpr int kTilePixels = 96;

	SDL_Renderer* renderer_ = nullptr;
	std::array<SDL_Texture*, sim::kBiomeCount * kVariants> tiles_{};
	SDL_Texture* screen_ = nullptr;

	SDL_Texture* tile(sim::Biome biome, int variant);
};

}  // namespace client
