#include "built.hpp"

#include <cmath>

#include "palette.hpp"

namespace client {

namespace {

/** What each tier is made of, as a comic flat and its shaded edge. */
Color tierColor(sim::BuildTier tier) {
    switch (tier) {
        case sim::BuildTier::Twig: return rgb(0x6b6a4a);
        case sim::BuildTier::Wood: return rgb(0x8a6034);
        case sim::BuildTier::Stone: return rgb(0x7e858c);
        case sim::BuildTier::Metal: return rgb(0x9aa8b4);
    }
    return rgb(0x6b6a4a);
}

Color tierEdge(sim::BuildTier tier) {
    switch (tier) {
        case sim::BuildTier::Twig: return rgb(0x4a4936);
        case sim::BuildTier::Wood: return rgb(0x5d4022);
        case sim::BuildTier::Stone: return rgb(0x4f555b);
        case sim::BuildTier::Metal: return rgb(0x5f6a74);
    }
    return rgb(0x4a4936);
}

/** A wall is drawn as a bar along its edge, this wide. */
constexpr float kWallHalf = 7;

struct View {
    double cameraX;
    double cameraY;
    double scale;
    int width;
    int height;

    Point at(double wx, double wy) const {
        return {static_cast<float>((wx - cameraX) * scale) + width * 0.5f,
                static_cast<float>((wy - cameraY) * scale) + height * 0.5f};
    }
};

/** The quad a piece of wall occupies, from one end of its edge to the other. */
void wallQuad(const View& view, double x0, double y0, double x1, double y1, double from, double to,
              std::vector<Point>& out) {
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double len = std::hypot(dx, dy);
    const double ux = dx / len;
    const double uy = dy / len;
    const double nx = -uy * kWallHalf;
    const double ny = ux * kWallHalf;
    const double ax = x0 + ux * len * from;
    const double ay = y0 + uy * len * from;
    const double bx = x0 + ux * len * to;
    const double by = y0 + uy * len * to;
    out = {view.at(ax + nx, ay + ny), view.at(bx + nx, by + ny), view.at(bx - nx, by - ny),
           view.at(ax - nx, ay - ny)};
}

void drawPiece(Paint& paint, const sim::Structure& piece, const View& view, Color fill, Color edge,
               bool ghost) {
    const float ink = ghost ? 0.0f : kInkWidth;
    if (piece.kind == sim::BuildKind::Foundation) {
        const double x = piece.gx * static_cast<double>(sim::kBuildCell);
        const double y = piece.gy * static_cast<double>(sim::kBuildCell);
        const double c = sim::kBuildCell;
        const std::vector<Point> square{view.at(x, y), view.at(x + c, y), view.at(x + c, y + c),
                                        view.at(x, y + c)};
        paint.fillPoly(square, fill);
        if (!ghost) {
            paint.outlinePoly(square, ink, kInk);
            // Planking, so a floor reads as boards rather than a coloured tile.
            for (int i = 1; i < 4; ++i) {
                const double t = i / 4.0;
                const Point a = view.at(x, y + c * t);
                const Point b = view.at(x + c, y + c * t);
                paint.line(a.x, a.y, b.x, b.y, kInkFine, edge);
            }
        }
        return;
    }

    double x0 = 0;
    double y0 = 0;
    double x1 = 0;
    double y1 = 0;
    sim::edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
    std::vector<Point> quad;
    if (piece.kind == sim::BuildKind::Wall) {
        wallQuad(view, x0, y0, x1, y1, 0, 1, quad);
        paint.fillPoly(quad, fill);
        if (!ghost) paint.outlinePoly(quad, ink, kInk);
        return;
    }

    // A doorway is two posts with a gap between them; a door fills the gap.
    wallQuad(view, x0, y0, x1, y1, 0, 0.28, quad);
    paint.fillPoly(quad, fill);
    if (!ghost) paint.outlinePoly(quad, ink, kInk);
    wallQuad(view, x0, y0, x1, y1, 0.72, 1, quad);
    paint.fillPoly(quad, fill);
    if (!ghost) paint.outlinePoly(quad, ink, kInk);

    if (piece.kind == sim::BuildKind::Door) {
        if (piece.open) {
            // Swung back against one post, out of the way.
            const double mx = x0 + (x1 - x0) * 0.28;
            const double my = y0 + (y1 - y0) * 0.28;
            const double ux = (x1 - x0) / std::hypot(x1 - x0, y1 - y0);
            const double uy = (y1 - y0) / std::hypot(x1 - x0, y1 - y0);
            const double leaf = sim::kBuildCell * 0.42;
            const std::vector<Point> swung{
                view.at(mx, my), view.at(mx - uy * leaf, my + ux * leaf),
                view.at(mx - uy * leaf + ux * 6, my + ux * leaf + uy * 6), view.at(mx + ux * 6, my + uy * 6)};
            paint.fillPoly(swung, edge);
            if (!ghost) paint.outlinePoly(swung, ink, kInk);
        } else {
            wallQuad(view, x0, y0, x1, y1, 0.28, 0.72, quad);
            paint.fillPoly(quad, edge);
            if (!ghost) paint.outlinePoly(quad, ink, kInk);
        }
    }
}

}  // namespace

BuildTarget targetAt(double worldX, double worldY, sim::BuildKind kind) {
    BuildTarget out;
    out.kind = kind;
    const double cell = sim::kBuildCell;
    const int gx = static_cast<int>(std::floor(worldX / cell));
    const int gy = static_cast<int>(std::floor(worldY / cell));
    out.gx = gx;
    out.gy = gy;
    if (kind == sim::BuildKind::Foundation) return out;

    // The nearest of the cell's four edges, named as the one cell that owns it.
    const double fx = worldX / cell - gx;
    const double fy = worldY / cell - gy;
    const double toNorth = fy;
    const double toSouth = 1 - fy;
    const double toWest = fx;
    const double toEast = 1 - fx;
    const double least = std::min(std::min(toNorth, toSouth), std::min(toWest, toEast));
    if (least == toNorth) {
        out.side = sim::EdgeSide::North;
    } else if (least == toSouth) {
        out.side = sim::EdgeSide::North;
        out.gy = gy + 1;
    } else if (least == toWest) {
        out.side = sim::EdgeSide::West;
    } else {
        out.side = sim::EdgeSide::West;
        out.gx = gx + 1;
    }
    return out;
}

void drawBuilt(Paint& paint, const sim::Structure& piece, double cameraX, double cameraY,
               double scale, int width, int height) {
    const View view{cameraX, cameraY, scale, width, height};
    const bool hurt = piece.flash > 0;
    drawPiece(paint, piece, view, hurt ? rgb(0xffdede) : tierColor(piece.tier),
              tierEdge(piece.tier), false);
}

void drawGhost(Paint& paint, const BuildTarget& target, sim::BuildTier tier, bool allowed,
               double cameraX, double cameraY, double scale, int width, int height) {
    const View view{cameraX, cameraY, scale, width, height};
    sim::Structure piece{};
    piece.kind = target.kind;
    piece.tier = tier;
    piece.gx = target.gx;
    piece.gy = target.gy;
    piece.side = target.side;
    const Color tint = allowed ? Color{124, 200, 255, 120} : Color{224, 80, 60, 120};
    drawPiece(paint, piece, view, tint, tint, true);
}

}  // namespace client
