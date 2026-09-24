#pragma once

#include "paint.hpp"

namespace client {

/** How a thing is swung: a side blow with a tool, or a rock driven forward. */
enum class MeleeStyle { Chop, Smash };

/** One frame of a swing: where the hand is and how the tool sits in it. */
struct MeleePose {
    Point hand{12, -6};
    /** The tool's own angle, about the grip. */
    float angle = 1.5707963f;
    /** How long it looks along its length; below 1 is foreshortening. */
    float stretch = 0.55f;
    /** The whole upper body's turn into the blow. */
    float twist = 0;
    /** Whether what sits over the fist is the head rather than the grip. */
    bool overHand = true;
};

/**
 * How a melee weapon is carried and swung, frame by frame.
 *
 * Every blow has three parts: a wind-up it starts in, because the blow lands
 * the moment you click and the swing opens already drawn back, a fast strike
 * through what is in front of you, and a slower recovery to the carry.
 *
 * A chop is a side blow, the way an axe goes into the side of a trunk. A smash
 * is a rock drawn back by the ear and driven forward. `t` runs 0 to 1 over the
 * swing; below zero is the carry between blows.
 */
MeleePose meleeMotion(MeleeStyle style, float t, float walkPhase);

}  // namespace client
