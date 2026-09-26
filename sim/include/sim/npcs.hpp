#pragma once

#include <vector>

#include "sim/build.hpp"
#include "sim/npc.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/** Declared rather than included: the rounds in the air know about the npcs. */
class Projectiles;

/** What the animals did to you this tick, for the client to draw and sound. */
struct NpcEvents {
	/** Damage that landed on the player, and where it came from. */
	double playerDamage = 0;
	double fromX = 0;
	double fromY = 0;
	/** An animal that died this tick, for the sound and the popup. */
	bool killed = false;
	NpcKind killedKind = NpcKind::Elk;
	double killedX = 0;
	double killedY = 0;
};

/**
 * The wildlife.
 *
 * Each animal lives in its own country and stays near where it was born. The
 * peaceful ones bolt from anything that outranks them, the hunters run down
 * whatever is on their list, and everything gives up on anyone who swims away.
 * What they are worth is left on the ground where they fall. See
 * docs/systems/wildlife.md for the roster and the pecking order.
 */
class NpcSystem {
public:
	/** Fills the island: each animal only in the country it belongs to. */
	void populate(const World& world, std::uint32_t seed);

	/** Posts the monuments' guards: scientists, and soldiers at the base. */
	void garrison(const World& world, std::uint32_t seed);

	const std::vector<Npc>& list() const { return npcs_; }
	std::vector<Npc>& mutableList() { return npcs_; }

	/** Everything alive in a rectangle, appended to `out`. */
	void inRect(double x0, double y0, double x1, double y1, std::vector<const Npc*>& out) const;

	NpcEvents update(World& world, const BuildSystem& build, Projectiles& projectiles, double dt,
					 const Player& player);

	/**
	 * A blow or a bullet landing.
	 *
	 * It takes the world because the thing may die of it, and what it was
	 * worth falls where it stood at that moment rather than a tick later:
	 * whoever killed it should be able to pick it up straight away.
	 */
	void hurt(World& world, Npc& npc, double amount, double fromX, double fromY);

	/** The nearest one in front of a player, for a swing to land on. */
	Npc* nearest(double x, double y, double within);

private:
	std::vector<Npc> npcs_;
	/** What has been killed and is owed back to the island, and when. */
	struct Regrowth {
		NpcKind kind;
		bool guard;
		double homeX;
		double homeY;
		double leash;
		double seconds;
	};
	std::vector<Regrowth> regrowth_;
	int nextId_ = 1;
	std::uint32_t rolls_ = 1;

	void drop(World& world, const Npc& npc);
};

}  // namespace sim
