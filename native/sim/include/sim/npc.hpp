#pragma once

#include <cstdint>

#include "sim/item.hpp"

namespace sim {

/** What lives on the island. The people of the monuments come later. */
enum class NpcKind : std::uint8_t { Boar, Wolf, Bear, Scientist, Soldier };

inline constexpr int kNpcKindCount = 5;

/** What one animal drops when it is killed, as a range. */
struct NpcLoot {
    ItemId id;
    int low;
    int high;
};

/** What one of them carries, and nothing for the ones that do not. */
struct NpcGun {
    double damage;
    double range;
    double cooldown;
    double speed;
    double spread;
};

struct NpcDef {
    NpcKind kind;
    const char* name;
    int hp;
    double speed;
    double radius;
    double damage;
    double attackRange;
    double attackCooldown;
    /** Whether it comes for you unprovoked; the rest only fight back. */
    bool hostile;
    /** Whether it walks on two legs, which is most of how it is drawn. */
    bool human;
    NpcGun gun;
    NpcLoot loot[5];
    int lootCount;
};

const NpcDef& npcDef(NpcKind kind);

/** What an animal is doing. */
enum class NpcState : std::uint8_t { Wander, Chase, Attack, Return };

/** One animal, somewhere on the island. */
struct Npc {
    int id;
    NpcKind kind;
    double x;
    double y;
    double vx;
    double vy;
    double facing;
    int hp;
    NpcState state;
    double stateTime;
    double attackTimer;
    /** How far through its gait it is, for the legs. */
    double animPhase;
    /** Seconds left of the white flash of being hit. */
    double flash;
    double homeX;
    double homeY;
    double leash;
    /** How long it has stood at the water's edge waiting for you to come out. */
    double shoreWait;
    /** Seconds until it may fire again. */
    double gunTimer;
    /** What a blow knocked into it, shed over the next moment. */
    double knockX;
    double knockY;
    /** Where it was posted, for the ones that hold a monument. */
    bool guard;
    std::uint32_t seed;
};

/** How far an animal will wander from where it was placed. */
inline constexpr double kNpcLeash = 520;
/**
 * Anything further than this from a player stops thinking, which keeps the
 * simulation flat however big the map gets.
 */
inline constexpr double kNpcActiveRadius = 1900;
/** How long an animal waits at the shore before giving up on a swimmer. */
inline constexpr double kShoreGiveUp = 3.0;

}  // namespace sim
