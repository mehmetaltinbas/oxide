#pragma once

#include <cstdint>

#include "sim/biome.hpp"
#include "sim/item.hpp"

namespace sim {

/** What lives on the island, and who holds its monuments. */
enum class NpcKind : std::uint8_t { Chicken, Deer, Hyena, Wolf, Bear, Scientist, Soldier };

inline constexpr int kNpcKindCount = 7;

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
	/**
	 * Where it lives.
	 *
	 * An animal is only ever placed in one of these, and it wanders inside its
	 * leash from there, so a deer is a thing you find in the trees and the
	 * snow rather than a thing that happens to be anywhere.
	 */
	Biome homes[3];
	int homeCount;
	/**
	 * How formidable it is, which is the whole of the pecking order.
	 *
	 * Anything skittish runs from what outranks it. Nothing attacks anything
	 * that is not on its own list, so rank decides who flees and the list
	 * decides who hunts; the two together are the food chain.
	 */
	int rank;
	/** What it hunts, unprovoked, when it sees one. */
	NpcKind eats[3];
	int eatsCount;
	/** Whether it bolts from anything stronger, the player included. */
	bool skittish;
};

const NpcDef& npcDef(NpcKind kind);

/** Whether that country is one of its homes. */
bool livesIn(const NpcDef& def, Biome biome);
/** Whether it hunts that, unprovoked. */
bool hunts(const NpcDef& def, NpcKind prey);

/** What an animal is doing. */
enum class NpcState : std::uint8_t { Wander, Chase, Attack, Return, Flee };

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
	/**
	 * What it is chasing or running from, and for how much longer.
	 *
	 * Zero means it has nothing in mind but you and the grass. Held for a few
	 * seconds so a wolf does not lose interest in a deer the instant the deer
	 * steps behind a tree.
	 */
	int target;
	double mind;
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
