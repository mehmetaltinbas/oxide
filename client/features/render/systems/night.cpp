#include "client/features/render/systems/night.hpp"
#include "client/design/types/color.struct.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace client {

namespace {

/**
 * How big the baked light is, in pixels across.
 *
 * It is always stretched up, never down, so this only has to be big enough
 * that the stretch does not show: at 192 the falloff is smooth past three
 * hundred pixels of light, which is more than anything in the game casts.
 */
constexpr int kSize = 192;

/**
 * The blend that erases.
 *
 * Colour is left exactly as it is and the destination's alpha is multiplied by
 * one minus the source's, so drawing the light into the veil takes the veil
 * away where the light is bright and leaves it where the light has faded.
 */
SDL_BlendMode eraser() {
	static SDL_BlendMode mode = SDL_ComposeCustomBlendMode(
		SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
		SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
	return mode;
}

}  // namespace

Night::~Night() {
	if (veil_) SDL_DestroyTexture(veil_);
	if (light_) SDL_DestroyTexture(light_);
}

SDL_Texture* Night::lightTexture() {
	if (light_) return light_;
	std::vector<std::uint32_t> pixels(static_cast<std::size_t>(kSize) * kSize);
	const float mid = (kSize - 1) * 0.5f;
	for (int y = 0; y < kSize; ++y) {
		for (int x = 0; x < kSize; ++x) {
			const float dx = (x - mid) / mid;
			const float dy = (y - mid) / mid;
			const float d = std::sqrt(dx * dx + dy * dy);
			// Flat and full in the middle, then falling away: a linear ramp
			// reads as a disc with a hard edge, and this reads as a lamp.
			float a = 1 - d;
			a = a <= 0 ? 0 : a * a * (0.4f + 0.6f * a);
			const auto alpha = static_cast<std::uint32_t>(a * 255.0f + 0.5f);
			pixels[static_cast<std::size_t>(y) * kSize + x] = (alpha << 24) | 0x00ffffffu;
		}
	}
	light_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
							   kSize, kSize);
	if (!light_) return nullptr;
	SDL_UpdateTexture(light_, nullptr, pixels.data(), kSize * 4);
	SDL_SetTextureScaleMode(light_, SDL_SCALEMODE_LINEAR);
	return light_;
}

void Night::begin(int width, int height, Color veil, float amount) {
	if (width <= 0 || height <= 0) return;
	if (!veil_ || width != width_ || height != height_) {
		if (veil_) SDL_DestroyTexture(veil_);
		veil_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
								  width, height);
		width_ = width;
		height_ = height;
	}
	if (!veil_) return;
	SDL_SetTextureBlendMode(veil_, SDL_BLENDMODE_BLEND);
	was_ = SDL_GetRenderTarget(renderer_);
	SDL_SetRenderTarget(renderer_, veil_);
	SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer_, veil.r, veil.g, veil.b,
						   static_cast<std::uint8_t>(std::min(1.0f, amount) * 255.0f));
	SDL_RenderClear(renderer_);
	open_ = true;
}

void Night::cut(float x, float y, float radius, float strength) {
	if (!open_ || radius <= 0 || strength <= 0) return;
	SDL_Texture* light = lightTexture();
	if (!light) return;
	SDL_SetTextureBlendMode(light, eraser());
	SDL_SetTextureAlphaModFloat(light, strength > 1 ? 1.0f : strength);
	const SDL_FRect dst{x - radius, y - radius, radius * 2, radius * 2};
	SDL_RenderTexture(renderer_, light, nullptr, &dst);
}

void Night::warm(float x, float y, float radius, Color tint, float strength) {
	SDL_Texture* light = lightTexture();
	if (!light || radius <= 0 || strength <= 0) return;
	SDL_SetTextureBlendMode(light, SDL_BLENDMODE_ADD);
	SDL_SetTextureColorMod(light, tint.r, tint.g, tint.b);
	SDL_SetTextureAlphaModFloat(light, strength > 1 ? 1.0f : strength);
	const SDL_FRect dst{x - radius, y - radius, radius * 2, radius * 2};
	SDL_RenderTexture(renderer_, light, nullptr, &dst);
	SDL_SetTextureColorMod(light, 255, 255, 255);
}

void Night::present() {
	if (!open_) return;
	SDL_SetRenderTarget(renderer_, was_);
	open_ = false;
	if (!veil_) return;
	SDL_RenderTexture(renderer_, veil_, nullptr, nullptr);
}

}  // namespace client
