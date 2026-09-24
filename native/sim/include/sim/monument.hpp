#pragma once

#include <cstdint>

#include "sim/deployable.hpp"
#include "sim/item.hpp"
#include "sim/npc.hpp"

namespace sim {

/** The looting places. Four are one to an island; two are scenery. */
enum class MonumentKind : std::uint8_t { Cabins, Lighthouse, Airfield, PowerPlant, Military, Town };

inline constexpr int kMonumentKindCount = 6;

/** One entry of a monument's loot table: how much of what, when it rolls. */
struct LootEntry {
    ItemId id;
    int low;
    int high;
};

struct MonumentDef {
    MonumentKind kind;
    const char* name;
    double radius;
    /** Radiation a second at the heart of it, and nought for the safe ones. */
    double rads;
    int scientists;
    /** The military base is held by soldiers rather than scientists. */
    bool soldiers;
    int crates;
    LootEntry loot[8];
    int lootCount;
    /** How many of it an island gets. */
    int count;
    /** A lighthouse stands on the shore; cabins are in the trees and the snow. */
    bool coast;
    bool forestOrSnow;
    /** The big three are never neighbours, whatever size the island is. */
    bool major;
};

const MonumentDef& monumentDef(MonumentKind kind);

/** One of them, standing somewhere. */
struct Monument {
    int id;
    MonumentKind kind;
    double x;
    double y;
    double radius;
};

/** A crate of loot at a monument, which fills again a while after it is emptied. */
struct LootCrate {
    int id;
    MonumentKind monument;
    double x;
    double y;
    Container container;
    bool looted;
    double respawn;
};

/**
 * How far past a monument's own radius you may not build.
 *
 * Their loot is what a run is for, and letting anyone wall one in would turn a
 * shared landmark into private property. The margin covers the approach as
 * well as the ground, so nobody rings one with walls and charges at the door.
 */
inline constexpr double kMonumentNoBuildMargin = 90;

/** How long an emptied crate takes to fill again. */
inline constexpr double kCrateRespawnSeconds = 300;

/**
 * How far apart the big places are kept: a third of the island's short side,
 * measured off the map rather than written out, so a bigger island spreads them
 * further rather than leaving them huddled in one corner.
 */
inline constexpr double kMonumentRelax = 0.85;
inline constexpr int kMonumentRounds = 12;

}  // namespace sim
