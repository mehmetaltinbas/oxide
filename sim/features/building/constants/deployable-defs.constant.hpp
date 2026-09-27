#pragma once

#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

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
/** Which item puts which thing down, and nothing for the rest. */
bool deployableOf(ItemId id, DeployKind& out);
/** What an item is called when it is standing rather than carried. */
ItemId itemOf(DeployKind kind);

}  // namespace sim
