#pragma once

#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/wildlife/types/npc-gun.struct.hpp"
#include "sim/features/wildlife/types/npc-kind.enum.hpp"
#include "sim/features/wildlife/types/npc-loot.struct.hpp"

namespace sim {

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
	/**
	 * Whether it bolts from anything stronger than it.
	 *
	 * About other animals only. A wolf is skittish because it runs from a
	 * bear, and that must not be read as running from you: see `fleesPlayer`.
	 */
	bool skittish;
	/**
	 * How far off it notices you and starts something.
	 *
	 * A wolf commits from across a field; a snake will not leave the yard of
	 * sand it is lying on. Zero takes the default, which is a field.
	 */
	double sight;
};

}  // namespace sim
