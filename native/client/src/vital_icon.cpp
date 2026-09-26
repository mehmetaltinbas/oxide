#include "vital_icon.hpp"

#include <cmath>
#include <vector>

#include "palette.hpp"

namespace client {

namespace {

/** A point of a shape given in the unit square, placed and scaled. */
struct Unit {
    float cx;
    float cy;
    float size;

    Point at(float x, float y) const { return {cx + x * size, cy + y * size}; }
};

/**
 * A cooked leg: a round meaty end tapering into the bone, and the bone ending
 * in the double knob that makes it read as a drumstick rather than a blob.
 */
void drawDrumstick(Paint& paint, const Unit& u, Color color) {
    const Color bone = rgb(0xf2ead8);
    const float pen = kInkWidth * 0.5f;

    // The shaft, as a thick line with the pen under it.
    const Point from = u.at(0.02f, 0.04f);
    const Point to = u.at(0.27f, 0.29f);
    paint.line(from.x, from.y, to.x, to.y, 0.13f * u.size + pen * 2, kInk);
    paint.line(from.x, from.y, to.x, to.y, 0.13f * u.size, bone);

    // The knuckle: two knobs side by side across the end of the shaft.
    for (const auto& knob : {std::pair<float, float>{0.24f, 0.38f},
                             std::pair<float, float>{0.37f, 0.25f}}) {
        const Point at = u.at(knob.first, knob.second);
        paint.inkedCircle(at.x, at.y, 0.085f * u.size, bone, pen);
    }
    // The shaft again over the inner edges, so the knobs join it.
    const Point innerA = u.at(0.2f, 0.22f);
    const Point innerB = u.at(0.29f, 0.31f);
    paint.line(innerA.x, innerA.y, innerB.x, innerB.y, 0.13f * u.size, bone);

    // The meat: a round end at the top left, tapering down onto the bone.
    std::vector<Point> meat;
    for (int i = 0; i <= 16; ++i) {
        const float a = static_cast<float>(i) / 16 * 6.28318530718f;
        meat.push_back(u.at(-0.16f + std::cos(a) * 0.27f, -0.14f + std::sin(a) * 0.24f));
    }
    paint.inkedPoly(meat, color, pen);

    // Two short marks across it: the grain, in the comic way.
    const Point grainA = u.at(-0.26f, -0.22f);
    const Point grainB = u.at(-0.1f, -0.24f);
    const Point grainC = u.at(-0.3f, -0.04f);
    const Point grainD = u.at(-0.1f, -0.06f);
    paint.line(grainA.x, grainA.y, grainB.x, grainB.y, pen * 0.8f, kInk);
    paint.line(grainC.x, grainC.y, grainD.x, grainD.y, pen * 0.8f, kInk);
}

}  // namespace

void drawVitalIcon(Paint& paint, Vital vital, float cx, float cy, float size, Color color) {
    const Unit u{cx, cy, size};
    const float pen = kInkWidth * 0.5f;

    if (vital == Vital::Food) {
        drawDrumstick(paint, u, color);
        return;
    }

    std::vector<Point> shape;
    if (vital == Vital::Health) {
        // Two lobes and a point.
        for (int i = 0; i <= 24; ++i) {
            const float t = static_cast<float>(i) / 24 * 6.28318530718f;
            // The classic heart curve, squashed into the unit square.
            const float x = 0.32f * std::pow(std::sin(t), 3.0f) * 1.6f;
            const float y = -(0.26f * std::cos(t) - 0.1f * std::cos(2 * t) -
                              0.04f * std::cos(3 * t) - 0.02f * std::cos(4 * t)) *
                            1.25f;
            shape.push_back(u.at(x, y));
        }
    } else {
        // A falling drop: a point at the top over a round belly, drawn as the
        // apex and then the long way round the belly.
        shape.push_back(u.at(0, -0.44f));
        const float belly = 0.3f;
        for (int i = 0; i <= 20; ++i) {
            // From up the right side, round the bottom, to up the left.
            const float a = -1.0f + static_cast<float>(i) / 20 * 5.28318530718f;
            shape.push_back(u.at(std::cos(a) * belly, 0.14f + std::sin(a) * belly));
        }
    }
    paint.inkedPoly(shape, color, pen);
}

}  // namespace client
