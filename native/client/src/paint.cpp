#include "paint.hpp"

#include "palette.hpp"
#include "text.hpp"

#include <cmath>

namespace client {

namespace {

SDL_FColor toF(Color c) {
    return SDL_FColor{c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
}

/** Enough segments that a circle reads as round at the size it is drawn. */
int segmentsFor(float radius) {
    const int n = static_cast<int>(radius * 0.9f) + 8;
    return n > 64 ? 64 : n;
}

}  // namespace

std::vector<Point> circlePoints(float cx, float cy, float radius, int segments) {
    if (segments <= 0) segments = segmentsFor(radius);
    std::vector<Point> out;
    out.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        const float a = static_cast<float>(i) / segments * 6.28318530718f;
        out.push_back({cx + std::cos(a) * radius, cy + std::sin(a) * radius});
    }
    return out;
}

float Paint::write(const std::string& line, float x, float y, float size, Color color) {
    return text_ ? text_->draw(line, x, y, size, color) : 0;
}

void Paint::submit() {
    if (!indices_.empty()) {
        SDL_RenderGeometry(renderer_, nullptr, vertices_.data(), static_cast<int>(vertices_.size()),
                           indices_.data(), static_cast<int>(indices_.size()));
    }
    vertices_.clear();
    indices_.clear();
}

void Paint::fillPoly(const std::vector<Point>& points, Color color) {
    if (points.size() < 3) return;
    const SDL_FColor c = toF(color);
    vertices_.clear();
    indices_.clear();
    vertices_.reserve(points.size());
    for (const Point& p : points) {
        vertices_.push_back(SDL_Vertex{SDL_FPoint{p.x, p.y}, c, SDL_FPoint{0, 0}});
    }
    // A fan, which is right for the convex and near-convex shapes drawn here.
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        indices_.push_back(0);
        indices_.push_back(static_cast<int>(i));
        indices_.push_back(static_cast<int>(i + 1));
    }
    submit();
}

void Paint::fillCircle(float cx, float cy, float radius, Color color) {
    fillPoly(circlePoints(cx, cy, radius), color);
}

void Paint::fillRect(float x, float y, float w, float h, Color color) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    const SDL_FRect rect{x, y, w, h};
    SDL_RenderFillRect(renderer_, &rect);
}

void Paint::line(float x0, float y0, float x1, float y1, float width, Color color) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) {
        fillCircle(x0, y0, width * 0.5f, color);
        return;
    }
    const float nx = -dy / len * width * 0.5f;
    const float ny = dx / len * width * 0.5f;
    fillPoly({{x0 + nx, y0 + ny}, {x1 + nx, y1 + ny}, {x1 - nx, y1 - ny}, {x0 - nx, y0 - ny}}, color);
    // Round caps, so a chain of segments has no notches at its corners.
    fillCircle(x0, y0, width * 0.5f, color);
    fillCircle(x1, y1, width * 0.5f, color);
}

void Paint::outlinePoly(const std::vector<Point>& points, float width, Color color, bool closed) {
    if (points.size() < 2) return;
    const std::size_t last = closed ? points.size() : points.size() - 1;
    for (std::size_t i = 0; i < last; ++i) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % points.size()];
        line(a.x, a.y, b.x, b.y, width, color);
    }
}

void Paint::outlineCircle(float cx, float cy, float radius, float width, Color color) {
    outlinePoly(circlePoints(cx, cy, radius), width, color);
}

void Paint::inkedPoly(const std::vector<Point>& points, Color fill, float width) {
    fillPoly(points, fill);
    outlinePoly(points, width, kInk);
}

void Paint::inkedCircle(float cx, float cy, float radius, Color fill, float width) {
    const std::vector<Point> points = circlePoints(cx, cy, radius);
    fillPoly(points, fill);
    outlinePoly(points, width, kInk);
}

}  // namespace client
