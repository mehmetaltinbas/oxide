#pragma once

#include "paint.hpp"

namespace client {

/** How one person is dressed, and what their body is doing this frame. */
struct HumanLook {
    float radius = 13;
    /** Where they are, on screen, and which way they face. */
    float x = 0;
    float y = 0;
    float facing = 0;
    /** How far through a stride they are. */
    float phase = 0;
    float stride = 1;
    bool swimming = false;
    Color skin{0xc0, 0x8a, 0x5e, 255};
    Color hair{0x3a, 0x2c, 0x22, 255};
    Color shirt{0x6a, 0x7f, 0x9a, 255};
    Color legs{0x4a, 0x3a, 0x2c, 255};
};

/**
 * One person, seen from above.
 *
 * Everybody human goes through this one routine, so a player, a scientist and
 * a soldier cannot drift into three different drawings of a person: feet
 * stepping under the body, shoulders wider than they are deep, arms that swing
 * against the stride, and a head that is mostly hair from up here.
 */
void drawHuman(Paint& paint, const HumanLook& look);

}  // namespace client
