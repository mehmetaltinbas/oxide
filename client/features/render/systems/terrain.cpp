#include "client/features/render/systems/terrain.hpp"

#include <cmath>

#include "client/features/render/systems/paint.hpp"
#include "client/design/tokens/world.tokens.hpp"
#include "sim/shared/utils/rng.util.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "client/design/types/color.struct.hpp"

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
			// Tufts of three blades, one per forty-six units of ground, each
			// leaning the way the blade beside it does not: the same tuft the
			// TypeScript game grows, which is what makes the grass read as
			// drawn rather than as a flat green square with specks on it.
			const int across = static_cast<int>(size / 46.0f) + 1;
			for (int gy = 0; gy < across; ++gy) {
				for (int gx = 0; gx < across; ++gx) {
					const int slot = (gy * across + gx) * 3;
					const float r = roll(slot);
					const float r2 = roll(slot + 1);
					if (r > 0.62f) continue;
					const float x = (gx + r) * 46.0f;
					const float y = (gy + r2) * 46.0f;
					const float h = 11.0f * (0.7f + r2 * 0.6f);
					for (int i = 0; i < 3; ++i) {
						const float lean = (i - 1) * 3.5f + (r - 0.5f) * 3;
						paint.line(x + (i - 1) * 3, y, x + (i - 1) * 3 + lean, y - h, 1.3f, kInk);
					}
				}
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
	if (screen_) SDL_DestroyTexture(screen_);
}

void Terrain::drawScreen(double cameraX, double cameraY, double zoom, int screenW, int screenH) {
	// Two dots half a tile apart, so the grid reads as a screen rather than as
	// rows and columns. Nine world units between them, as the press had it.
	constexpr int kSpacing = 9;
	constexpr int kOversample = 4;
	if (!screen_) {
		const int size = kSpacing * kOversample;
		screen_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
									size, size);
		if (!screen_) return;
		SDL_SetTextureBlendMode(screen_, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(screen_, SDL_SCALEMODE_LINEAR);
		SDL_Texture* was = SDL_GetRenderTarget(renderer_);
		SDL_SetRenderTarget(renderer_, screen_);
		SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
		SDL_RenderClear(renderer_);
		SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
		Paint paint(renderer_);
		const float dot = 0.55f * kOversample;
		paint.fillCircle(size * 0.25f, size * 0.25f, dot, Color{20, 17, 13, 190});
		paint.fillCircle(size * 0.75f, size * 0.75f, dot, Color{20, 17, 13, 190});
		SDL_SetRenderTarget(renderer_, was);
	}
	// Pinned to the world rather than the screen, so the dots do not swim
	// about as you walk.
	const float tile = static_cast<float>(kSpacing * zoom);
	const float offX = static_cast<float>(std::fmod(cameraX * zoom, tile));
	const float offY = static_cast<float>(std::fmod(cameraY * zoom, tile));
	const SDL_FRect dst{-offX - tile, -offY - tile, screenW + tile * 2, screenH + tile * 2};
	// The tile is baked four times larger than it is drawn, so its dots stay
	// round when the view is zoomed in.
	SDL_RenderTextureTiled(renderer_, screen_, nullptr,
						   static_cast<float>(zoom) / kOversample, &dst);
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

void Terrain::drawOver(Paint& paint, const sim::World& world, double cameraX, double cameraY,
					   double zoom, int screenW, int screenH, double clock) {
	const double halfW = screenW / (2 * zoom);
	const double halfH = screenH / (2 * zoom);
	const int col0 = static_cast<int>(std::floor((cameraX - halfW) / sim::kBiomeTile)) - 1;
	const int col1 = static_cast<int>(std::floor((cameraX + halfW) / sim::kBiomeTile)) + 1;
	const int row0 = static_cast<int>(std::floor((cameraY - halfH) / sim::kBiomeTile)) - 1;
	const int row1 = static_cast<int>(std::floor((cameraY + halfH) / sim::kBiomeTile)) + 1;

	for (int row = row0; row <= row1; ++row) {
		for (int col = col0; col <= col1; ++col) {
			const bool inside =
				col >= 0 && row >= 0 && col < sim::kBiomeCols && row < sim::kBiomeRows;
			const sim::Biome biome = inside ? world.tile(col, row) : sim::Biome::Water;
			// A road is inked where it meets anything else, the way a panel
			// separates one thing from another. Only on the road's own side:
			// drawn from both sides every seam came out twice as heavy.
			if (biome == sim::Biome::Road) {
				const float x0 = static_cast<float>((col * sim::kBiomeTile - cameraX) * zoom) +
								 screenW * 0.5f;
				const float y0 = static_cast<float>((row * sim::kBiomeTile - cameraY) * zoom) +
								 screenH * 0.5f;
				const float w = static_cast<float>(sim::kBiomeTile * zoom);
				// Heavier than a node's outline: this is a panel border between
				// two grounds, not a line round an object, and at a node's
				// weight it came out under a pixel and could not be seen.
				const float ink = kInkWidth * 3.0f * static_cast<float>(zoom);
				const auto other = [&](int dc, int dr) {
					const int c = col + dc;
					const int r = row + dr;
					if (c < 0 || r < 0 || c >= sim::kBiomeCols || r >= sim::kBiomeRows) return true;
					return world.tile(c, r) != sim::Biome::Road;
				};
				if (other(0, -1)) paint.line(x0, y0, x0 + w, y0, ink, kInk);
				if (other(0, 1)) paint.line(x0, y0 + w, x0 + w, y0 + w, ink, kInk);
				if (other(-1, 0)) paint.line(x0, y0, x0, y0 + w, ink, kInk);
				if (other(1, 0)) paint.line(x0 + w, y0, x0 + w, y0 + w, ink, kInk);
				continue;
			}
			if (biome != sim::Biome::Water) continue;
			const double wx = (col + 0.5) * sim::kBiomeTile;
			const double wy = (row + 0.5) * sim::kBiomeTile;
			// A lake is sheltered: a small ripple that barely travels. The open
			// sea is not: a long swell that rolls across the tile and back.
			const bool lake = inside && world.freshAt(wx, wy);
			const int lines = lake ? 2 : 3;
			const float amp = lake ? 1.6f : 5.5f;
			const double speed = lake ? 0.35 : 1.1;
			const float length = lake ? 0.34f : 0.62f;
			const std::uint8_t ink = lake ? 54 : 96;
			for (int i = 0; i < lines; ++i) {
				// Its own offset down the tile and its own phase, so a field of
				// tiles does not beat in unison.
				const double seed = sim::seeded(static_cast<std::uint64_t>(col * 73 + row * 149),
												i);
				const double lane = (0.18 + 0.3 * i + seed * 0.12);
				const double phase = clock * speed + seed * 6.28318530718 + col * 0.7 + row * 0.4;
				const double ox = wx - sim::kBiomeTile * 0.5 +
								  sim::kBiomeTile * (0.2 + 0.3 * std::sin(phase));
				const double oy = wy - sim::kBiomeTile * 0.5 + sim::kBiomeTile * lane +
								  amp * std::sin(phase * 1.3);
				const float x0 = static_cast<float>((ox - cameraX) * zoom) + screenW * 0.5f;
				const float y0 = static_cast<float>((oy - cameraY) * zoom) + screenH * 0.5f;
				const float run = static_cast<float>(sim::kBiomeTile * length * zoom);
				const float lift = static_cast<float>(amp * 0.5 * zoom);
				// A crest, not a straight line: out, up over the top, and down.
				const std::vector<Point> crest{{x0, y0},
											   {x0 + run * 0.35f, y0 - lift},
											   {x0 + run * 0.7f, y0 - lift},
											   {x0 + run, y0}};
				paint.outlinePoly(crest, 2.0f, Color{255, 255, 255, ink}, false);
			}
		}
	}
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
