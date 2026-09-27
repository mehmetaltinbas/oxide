#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <unordered_map>

#include "client/features/render/systems/paint.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/render/types/align.enum.hpp"
#include "client/features/render/types/face.enum.hpp"

namespace client {

/**
 * Text on the screen.
 *
 * Every line is rendered once into a texture and kept, because the same few
 * hundred strings are drawn every frame: a number that has not changed costs a
 * textured quad, not a pass over a font.
 */
class Text {
public:
	~Text();

	/** Opens the faces. Says whether it worked; without them nothing is drawn. */
	bool open(SDL_Renderer* renderer, const std::string& fontDir);

	/**
	 * Gives back the textures and the fonts.
	 *
	 * Called before SDL is shut down rather than left to the destructor: the
	 * destructor runs after SDL_Quit has pulled the renderer out from under
	 * it, and freeing a texture then is a crash on the way out of a game that
	 * had otherwise finished cleanly.
	 */
	void close();
	bool ready() const { return body_ != nullptr; }

	/**
	 * One line, at a size in points. Returns how wide it came out, so a caller
	 * can lay the next thing out after it.
	 */
	float draw(const std::string& line, float x, float y, float size, Color color,
			   Face face = Face::Body, Align align = Align::Left);

	/**
	 * One line with a black line round it, the way a word reads on a printed
	 * page: ink round paper, so it holds against grass, sand or water alike.
	 */
	float drawInked(const std::string& line, float x, float y, float size, Color color,
					Face face = Face::Body, Align align = Align::Left, int outline = 2);

	/** How wide a line would be, without drawing it. */
	float widthOf(const std::string& line, float size, Face face = Face::Body);

	/** Drops what has not been drawn for a while, so the cache cannot grow forever. */
	void endFrame();

private:
	struct Line {
		SDL_Texture* texture = nullptr;
		float w = 0;
		float h = 0;
		/** The frame it was last drawn in. */
		std::uint64_t seen = 0;
	};

	/** A face at one size: SDL_ttf sizes a font rather than scaling a glyph. */
	TTF_Font* fontFor(Face face, int size, int outline = 0);
	const Line* lineFor(const std::string& text, Face face, int size, Color color,
						int outline = 0);

	SDL_Renderer* renderer_ = nullptr;
	/** The raw font files, kept open so a new size can be opened from them. */
	TTF_Font* display_ = nullptr;
	TTF_Font* body_ = nullptr;
	TTF_Font* bodyBold_ = nullptr;
	std::unordered_map<std::uint64_t, TTF_Font*> sized_;
	std::unordered_map<std::string, Line> lines_;
	std::uint64_t frame_ = 0;
};

}  // namespace client
