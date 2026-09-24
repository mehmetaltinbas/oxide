#pragma once

#include <cmath>

#include "paint.hpp"

namespace client {

/**
 * A person's own frame: origin at the chest, facing up the screen.
 *
 * The body, the arms and whatever is in the hand are all laid out in these
 * numbers and placed by this, so the drawing reads as a drawing and not as
 * trigonometry, and so the hand and the tool in it cannot drift apart.
 */
struct BodyFrame {
    float ox = 0;
    float oy = 0;
    float cos = 1;
    float sin = 0;
    float scale = 1;

    Point at(float x, float y) const {
        const float sx = x * scale;
        const float sy = y * scale;
        return {ox + sx * cos - sy * sin, oy + sx * sin + sy * cos};
    }

    /** The same frame turned about the chest, for a body twisting into a blow. */
    BodyFrame turned(float radians) const {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        return BodyFrame{ox, oy, cos * c - sin * s, sin * c + cos * s, scale};
    }
};

}  // namespace client
