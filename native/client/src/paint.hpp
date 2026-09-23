#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

namespace client {

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

/** A colour written the way the TypeScript game writes it: 0xRRGGBB. */
constexpr Color rgb(std::uint32_t hex, std::uint8_t alpha = 255) {
    return Color{static_cast<std::uint8_t>((hex >> 16) & 0xff), static_cast<std::uint8_t>((hex >> 8) & 0xff),
                 static_cast<std::uint8_t>(hex & 0xff), alpha};
}

struct Point {
    float x = 0;
    float y = 0;
};

/**
 * The pen and the brush.
 *
 * SDL draws rectangles, lines and triangles and nothing else, so the shapes the
 * comic look is built from - a filled blob with a black line round it - are
 * assembled here once instead of in every tree and barrel. Everything a shape
 * needs is a polygon and a circle, filled and then inked.
 */
class Paint {
public:
    explicit Paint(SDL_Renderer* renderer) : renderer_(renderer) {}

    SDL_Renderer* renderer() const { return renderer_; }

    void fillPoly(const std::vector<Point>& points, Color color);
    void fillCircle(float cx, float cy, float radius, Color color);
    void fillRect(float x, float y, float w, float h, Color color);

    /** A line with width, drawn as a quad so it can be as heavy as the ink is. */
    void line(float x0, float y0, float x1, float y1, float width, Color color);
    /** The line round a shape: the same polygon walked edge by edge. */
    void outlinePoly(const std::vector<Point>& points, float width, Color color, bool closed = true);
    void outlineCircle(float cx, float cy, float radius, float width, Color color);

    /** Fill and ink in one call, which is what nearly every shape here wants. */
    void inkedPoly(const std::vector<Point>& points, Color fill, float width);
    void inkedCircle(float cx, float cy, float radius, Color fill, float width);

private:
    SDL_Renderer* renderer_ = nullptr;
    std::vector<SDL_Vertex> vertices_;
    std::vector<int> indices_;

    void submit();
};

/** The points of a circle, for when a shape needs them rather than the fill. */
std::vector<Point> circlePoints(float cx, float cy, float radius, int segments = 0);

}  // namespace client
