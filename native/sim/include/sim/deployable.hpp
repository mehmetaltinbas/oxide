#pragma once

#include <vector>

#include "sim/item.hpp"

namespace sim {

/** The things you put down rather than build: they sit on a cell of their own. */
enum class DeployKind : std::uint8_t {
    Campfire,
    Furnace,
    ToolCupboard,
    WoodenBox,
    SleepingBag,
    Workbench1,
    Workbench2,
    Workbench3,
};

/** How near you must stand to a bench for it to count. */
inline constexpr double kBenchReach = 160;

/** Which tier a thing is as a workbench, and nought for everything else. */
int benchTier(DeployKind kind);

/** How much room each one has inside it, and nought for the ones with none. */
int containerSlots(DeployKind kind);

/** What a deployable is worth when it is put down. */
inline constexpr int kDeployableHp = 300;
/** How much of a cell one takes up, for walking into it. */
inline constexpr double kDeployHalf = 22;
/** How far a tool cupboard's claim reaches. */
inline constexpr double kToolCupboardRadius = 420;
/**
 * How far apart two sleeping bags must be.
 *
 * Rust's rule: as many bags as you like, but not two in the same spot, so a bag
 * is a claim on a place rather than a stack of extra lives in one room.
 */
inline constexpr double kSleepingBagSpacing = 320;

/** A furnace's rates, and how long a campfire takes over a piece of meat. */
inline constexpr double kSmeltSeconds = 2.5;
inline constexpr double kCharcoalChance = 0.5;
inline constexpr double kCookSeconds = 4;
/** How long one piece of wood burns for. */
inline constexpr double kWoodBurnSeconds = 4;

/** Somewhere to put things: a box, a fire's grate, a furnace's belly. */
struct Container {
    std::vector<ItemStack> slots;

    /** Takes what it can; returns what would not fit. */
    int add(ItemId id, int count);
    int count(ItemId id) const;
    int take(ItemId id, int count);
};

struct Deployable {
    int id;
    DeployKind kind;
    double x;
    double y;
    int hp;
    int maxHp;
    int owner;
    bool lit;
    /** Seconds of fuel left, and how far through the current piece of work. */
    double fuel;
    double progress;
    double flash;
    Container container;
};

/** Which item puts which thing down, and nothing for the rest. */
bool deployableOf(ItemId id, DeployKind& out);
/** What an item is called when it is standing rather than carried. */
ItemId itemOf(DeployKind kind);

}  // namespace sim
