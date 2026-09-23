#pragma once

#include "sim/world.hpp"

namespace sim {

/** The numbers a player moves by, shared with the server that checks them. */
struct PlayerRules {
    static constexpr double kRadius = 13;
    static constexpr double kSpeed = 132;
    static constexpr double kSprint = 1.4;
    /** Swimming: slow, and no sprinting out of it. */
    static constexpr double kSwim = 0.46;
};

/** Someone standing on the island. */
struct Player {
    double x = 0;
    double y = 0;
    /** Where they are looking, in radians. */
    double aim = 0;
    /** How far through a stride they are, for the legs and the arms. */
    double walkPhase = 0;
    bool swimming = false;
    bool sprinting = false;
};

/** What a player is asking to do this frame. */
struct PlayerInput {
    double moveX = 0;
    double moveY = 0;
    bool sprint = false;
    double aim = 0;
};

/**
 * One step of a player's movement, on the client and on the server alike.
 *
 * The client runs it to move at once rather than waiting for the network, and
 * the server runs the same code on the same input, so the two agree without
 * anyone keeping two copies of the rules in step by hand.
 */
void stepPlayer(const World& world, Player& player, const PlayerInput& input, double dt);

}  // namespace sim
