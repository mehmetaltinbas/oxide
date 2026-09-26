#include "text.hpp"

#include <vector>

namespace client {

namespace {

/** The sizes are rounded, because a font is opened per size and kept. */
int roundedSize(float size) {
    const int at = static_cast<int>(size + 0.5f);
    return at < 8 ? 8 : at;
}

}  // namespace

Text::~Text() { close(); }

void Text::close() {
    for (auto& [key, line] : lines_) {
        if (line.texture) SDL_DestroyTexture(line.texture);
    }
    lines_.clear();
    for (auto& [key, font] : sized_) TTF_CloseFont(font);
    sized_.clear();
    if (display_) TTF_CloseFont(display_);
    if (body_) TTF_CloseFont(body_);
    if (bodyBold_) TTF_CloseFont(bodyBold_);
    display_ = nullptr;
    body_ = nullptr;
    bodyBold_ = nullptr;
    renderer_ = nullptr;
    if (TTF_WasInit()) TTF_Quit();
}

bool Text::open(SDL_Renderer* renderer, const std::string& fontDir) {
    renderer_ = renderer;
    if (!TTF_Init()) return false;
    // Opened once at a nominal size; every size drawn opens its own copy from
    // the same file, which is how SDL_ttf wants to be used.
    display_ = TTF_OpenFont((fontDir + "/bangers.ttf").c_str(), 24);
    body_ = TTF_OpenFont((fontDir + "/comic-neue.ttf").c_str(), 16);
    bodyBold_ = TTF_OpenFont((fontDir + "/comic-neue-bold.ttf").c_str(), 16);
    return body_ != nullptr;
}

TTF_Font* Text::fontFor(Face face, int size) {
    const std::uint64_t key = (static_cast<std::uint64_t>(face) << 32) |
                              static_cast<std::uint32_t>(size);
    const auto it = sized_.find(key);
    if (it != sized_.end()) return it->second;
    const char* file = face == Face::Display     ? "bangers.ttf"
                       : face == Face::BodyBold  ? "comic-neue-bold.ttf"
                                                 : "comic-neue.ttf";
    // The directory is remembered by way of the fonts opened in `open`.
    TTF_Font* from = face == Face::Display     ? display_
                     : face == Face::BodyBold  ? bodyBold_
                                               : body_;
    if (!from) return nullptr;
    TTF_Font* copy = TTF_CopyFont(from);
    if (!copy) return nullptr;
    TTF_SetFontSize(copy, static_cast<float>(size));
    (void)file;
    sized_[key] = copy;
    return copy;
}

const Text::Line* Text::lineFor(const std::string& text, Face face, int size, Color color) {
    // Keyed by everything that changes how it looks, so two colours of the
    // same word are two textures rather than one that flickers between them.
    char head[64];
    SDL_snprintf(head, sizeof(head), "%d|%d|%02x%02x%02x|", static_cast<int>(face), size, color.r,
                 color.g, color.b);
    const std::string key = std::string(head) + text;
    const auto it = lines_.find(key);
    if (it != lines_.end()) {
        it->second.seen = frame_;
        return &it->second;
    }
    TTF_Font* font = fontFor(face, size);
    if (!font) return nullptr;
    const SDL_Color sdl{color.r, color.g, color.b, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), sdl);
    if (!surface) return nullptr;
    Line line;
    line.texture = SDL_CreateTextureFromSurface(renderer_, surface);
    line.w = static_cast<float>(surface->w);
    line.h = static_cast<float>(surface->h);
    line.seen = frame_;
    SDL_DestroySurface(surface);
    if (!line.texture) return nullptr;
    SDL_SetTextureBlendMode(line.texture, SDL_BLENDMODE_BLEND);
    lines_[key] = line;
    return &lines_[key];
}

float Text::draw(const std::string& text, float x, float y, float size, Color color, Face face,
                 Align align) {
    if (text.empty() || !renderer_) return 0;
    const Line* line = lineFor(text, face, roundedSize(size), color);
    if (!line) return 0;
    const float left = align == Align::Centre  ? x - line->w * 0.5f
                       : align == Align::Right ? x - line->w
                                               : x;
    SDL_SetTextureAlphaMod(line->texture, color.a);
    const SDL_FRect dst{left, y, line->w, line->h};
    SDL_RenderTexture(renderer_, line->texture, nullptr, &dst);
    return line->w;
}

float Text::widthOf(const std::string& text, float size, Face face) {
    if (text.empty()) return 0;
    const Line* line = lineFor(text, face, roundedSize(size), rgb(0xffffff));
    return line ? line->w : 0;
}

void Text::endFrame() {
    ++frame_;
    // Anything not drawn for a few hundred frames is let go: the cache is for
    // the lines the game keeps asking for, not a record of every one it ever
    // drew.
    if (frame_ % 600 != 0) return;
    std::vector<std::string> stale;
    for (const auto& [key, line] : lines_) {
        if (frame_ - line.seen > 600) stale.push_back(key);
    }
    for (const std::string& key : stale) {
        if (lines_[key].texture) SDL_DestroyTexture(lines_[key].texture);
        lines_.erase(key);
    }
}

}  // namespace client
