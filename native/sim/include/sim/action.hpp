#pragma once

#include "sim/inventory.hpp"
#include "sim/npcs.hpp"
#include "sim/projectile.hpp"
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
    /** An animal took the blow instead, and how much it felt. */
    bool hitNpc = false;
    NpcKind npcKind = NpcKind::Boar;
    /** Whether that blow was the one that finished it. */
    bool killed = false;
    double damage = 0;
};

/**
 * One blow with whatever is in your hand.
 *
 * Living things first, then what stands on the ground. Other players and
 * buildings arrive on top of this, in the order the old game took them.
 */
SwingResult swing(World& world, NpcSystem& npcs, Player& player, Inventory& inventory);

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

/** What pulling the trigger did. */
struct FireResult {
    bool fired = false;
    /** Nothing in the magazine: the click that tells you to reload. */
    bool empty = false;
    /** How many rounds went out, which is more than one for a shotgun. */
    int rounds = 0;
    double x = 0;
    double y = 0;
    double angle = 0;
    ItemId gun = ItemId::None;
};

/**
 * Pulling the trigger.
 *
 * A gun fires what is loaded and no more; a bow needs a full draw and takes
 * its arrow straight out of the pack. `drawing` is whether the trigger is
 * being held, which is the whole of how a bow works.
 */
FireResult fire(Player& player, Inventory& inventory, Projectiles& projectiles, bool drawing,
                double dt);

/** Starting a reload, and the seconds of it passing. */
void reload(Player& player, const Inventory& inventory);
void tickReload(Player& player, Inventory& inventory, double dt);

/** How many rounds are to hand for whatever is held, loaded and carried. */
int roundsCarried(const Player& player, const Inventory& inventory);

/** How far you can reach to pick something up. */
inline constexpr double kPickReach = 46;

}  // namespace sim
