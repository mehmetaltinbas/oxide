#include "client/features/ui/systems/map-screen.hpp"

#include <algorithm>
#include <cmath>

#include "client/design/tokens/world.tokens.hpp"
#include "client/features/render/systems/text.hpp"
#include "sim/features/building/types/deployable.struct.hpp"
#include "sim/features/monuments/constants/monument-defs.constant.hpp"
#include "sim/features/monuments/types/monument-def.struct.hpp"
#include "sim/features/monuments/types/monument.struct.hpp"
#include "sim/features/survival/types/player.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/align.enum.hpp"
#include "client/features/render/types/face.enum.hpp"

namespace client {

namespace {

/**
 * How many squares fit along the map's longest side. The short side gets
 * however many fit at the same size, so a square is always square whatever
 * shape the island is.
 */
/**
 * How many lettered squares the island is divided into, each way.
 *
 * Twenty. At fifteen a square was twenty-one foundations across, which is a
 * whole base and its garden: "he is in H7" told you almost nothing. At twenty
 * it is sixteen, which is a place rather than a district.
 */
constexpr int kCellsAcross = 20;
constexpr double kGridSize =
	(sim::kWorldWidth > sim::kWorldHeight ? sim::kWorldWidth : sim::kWorldHeight) / kCellsAcross;

/** One line of the map's own lettering, at a size in pixels. */
void text(Paint& paint, float x, float y, float size, Color color, const char* line,
		  Align align = Align::Left, Face face = Face::Body) {
	if (Text* lettering = paint.text()) lettering->draw(line, x, y, size, color, face, align);
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
			text(paint, x + col * cell + 3 * uiScale, y + row * cell + 2 * uiScale,
				 9 * uiScale, Color{240, 235, 221, 170}, label);
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
		text(paint, mx, my + 6 * uiScale, 11 * uiScale, hot ? rgb(0xb4e65a) : rgb(0xefeadd),
			 def.name, Align::Centre);
	}

	// Where you last died, under everything else: a cross, in the red the
	// health bar is, with your own name for the square beside it. What you
	// were carrying is lying there, and it rots.
	if (died_) {
		const float dx = x + static_cast<float>(deathX_) * scale;
		const float dy = y + static_cast<float>(deathY_) * scale;
		const float arm = 7 * uiScale;
		const Color bone = rgb(0xd8483a);
		for (int i = -1; i <= 1; i += 2) {
			const float s = static_cast<float>(i);
			paint.line(dx - arm, dy - arm * s, dx + arm, dy + arm * s, 4.5f * uiScale, kInk);
			paint.line(dx - arm, dy - arm * s, dx + arm, dy + arm * s, 2.5f * uiScale, bone);
		}
	}

	// You: an arrowhead with its point where you are looking, rather than a
	// dot with a whisker off it. A shape that has a front tells you which way
	// you face at a glance, which is the whole of what the marker is for.
	const float px = x + static_cast<float>(player.x) * scale;
	const float py = y + static_cast<float>(player.y) * scale;
	{
		const float a = static_cast<float>(player.aim);
		const float ca = std::cos(a);
		const float sa = std::sin(a);
		// Along the way you look, and across it.
		const auto at = [&](float along, float across) {
			return Point{px + ca * along - sa * across, py + sa * along + ca * across};
		};
		const float len = 11 * uiScale;
		const float half = 6.5f * uiScale;
		// Notched at the back, so the head reads as an arrow and not a wedge.
		paint.inkedPoly({at(len, 0), at(-len * 0.7f, half), at(-len * 0.35f, 0),
						 at(-len * 0.7f, -half)},
						rgb(0x7cc8ff), kInkFine * 2);
	}

	// Your team, and nobody else: finding the rest is still the game.
	for (const Mate& mate : mates_) {
		paint.inkedCircle(x + static_cast<float>(mate.x) * scale,
						  y + static_cast<float>(mate.y) * scale, 4 * uiScale, rgb(0x5fb85f),
						  kInkFine);
	}
	mates_.clear();

	char square[8];
	squareOf(player.x, player.y, square, sizeof(square));
	char line[64];
	SDL_snprintf(line, sizeof(line), "Map    you are in %s    M to close", square);
	text(paint, x, y - 30 * uiScale, 18 * uiScale, rgb(0xefeadd), line, Align::Left,
		 Face::Display);
}

}  // namespace client
