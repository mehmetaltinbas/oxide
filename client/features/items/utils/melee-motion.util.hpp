#pragma once

#include "client/features/render/systems/paint.hpp"
#include "client/features/items/types/melee-pose.struct.hpp"
#include "client/features/items/types/melee-style.enum.hpp"

namespace client {

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
