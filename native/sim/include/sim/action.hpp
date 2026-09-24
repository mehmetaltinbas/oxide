#pragma once

#include "sim/inventory.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/** What a blow did, so the client can draw and sound it. */
struct SwingResult {
    /** Whether the blow was thrown at all: a swing on cooldown is nothing. */
    bool swung = false;
    bool landed = false;
    int nodeId = 0;
    NodeKind kind = NodeKind::Tree;
    double x = 0;
    double y = 0;
    bool broke = false;
    /** What went into the pack, for the little number that floats up. */
    ItemStack gained{};
    /** What would not fit, so the player can be told their pack is full. */
    bool packFull = false;
};

/**
 * One blow with whatever is in your hand.
 *
 * Nodes only for now: the animals, the other players and the buildings arrive
 * on top of this, in the order the old game took them.
 */
SwingResult swing(World& world, Player& player, Inventory& inventory);

/** What picking something up did. */
struct PickResult {
    bool picked = false;
    ItemStack stack{};
    double x = 0;
    double y = 0;
    bool packFull = false;
};

/**
 * Picking up: a stack off the ground first, then a nettle, which is gathered
 * by hand rather than swung at.
 */
PickResult pickUp(World& world, const Player& player, Inventory& inventory);

/** How far you can reach to pick something up. */
inline constexpr double kPickReach = 46;

}  // namespace sim
