#pragma once

#include <SDL3/SDL.h>

#include "client/features/render/systems/paint.hpp"
#include "client/design/types/color.struct.hpp"

namespace client {

/**
 * The night, and the holes fires cut in it.
 *
 * A light is not paint. Adding a warm circle on top of the world washes out
 * everything under it: the grass, the ink on a tree and the player all go, and
 * what you get is a glowing smudge with nothing inside it. So the night is
 * built as a veil over the whole screen and each light **erases** part of that
 * veil, which leaves the world underneath at its own brightness. That is what
 * a fire does: it does not add orange to the ground, it stops the ground being
 * dark.
 *
 * The veil is built on its own texture so the erasing can touch its alpha
 * without touching the world, and blitted over in one pass at the end.
 */
class Night {
public:
	explicit Night(SDL_Renderer* renderer) : renderer_(renderer) {}
	~Night();

	/**
	 * Starts the veil, of this colour and this deep, and points drawing at it.
	 * Everything until `present` goes onto the veil, not onto the world.
	 */
	void begin(int width, int height, Color veil, float amount);

	/**
	 * Cuts a soft hole in it: `radius` screen pixels, fading to nothing at the
	 * edge so a light never ends in a visible circle. `strength` from nought to
	 * one is how completely the middle is cleared.
	 */
	void cut(float x, float y, float radius, float strength);

	/** Puts drawing back where it was and lays the veil over the world. */
	void present();

	/**
	 * A little warmth over a hole already cut, laid on after the veil.
	 *
	 * Erasing the dark gives you the ground back at its daylight colour, which
	 * is right for how much you can see and wrong for what it is lit by. This
	 * adds the fire's own colour back, gently: enough to read as firelight, not
	 * enough to wash the ground out again.
	 */
	void warm(float x, float y, float radius, Color tint, float strength);

private:
	SDL_Renderer* renderer_ = nullptr;
	SDL_Texture* veil_ = nullptr;
	SDL_Texture* light_ = nullptr;
	SDL_Texture* was_ = nullptr;
	int width_ = 0;
	int height_ = 0;
	bool open_ = false;

	SDL_Texture* lightTexture();
};

}  // namespace client
