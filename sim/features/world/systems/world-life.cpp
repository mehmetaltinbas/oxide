#include <algorithm>
#include <cmath>

#include "sim/features/world/systems/world.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"
#include "sim/features/monuments/constants/monument-defs.constant.hpp"
#include "sim/features/monuments/types/loot-crate.struct.hpp"
#include "sim/features/monuments/types/monument-def.struct.hpp"
#include "sim/features/monuments/types/monument.struct.hpp"
#include "sim/features/world/constants/drop-lifetime.constant.hpp"
#include "sim/features/world/constants/node-defs.constant.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/world/types/dropped.struct.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"
#include "sim/shared/utils/health.util.hpp"

namespace sim {

namespace {

/** What is in a barrel, and how often. */
struct Loot {
	ItemId id;
	int low;
	int high;
	double chance;
};

constexpr Loot kBarrelLoot[4] = {
	{ItemId::Scrap, 3, 6, 1.0},
	{ItemId::Metal, 10, 25, 0.3},
	{ItemId::LowGrade, 5, 12, 0.25},
	{ItemId::PistolAmmo, 4, 8, 0.12},
};

/**
 * How long anything harvested takes to come back: one full in-game day.
 *
 * One rule for every resource. It is what makes a stretch of coast you worked
 * yesterday worth walking past today, and what stops an ore field being farmed
 * on a five-minute loop.
 */
/**
 * How long a tree, a rock or an ore takes to come back.
 *
 * Ninety minutes: an hour and a half, which is a day and a half of this
 * island's own clock. An hour was a day exactly, so a patch you cleared at
 * noon was back by noon and clearing it meant nothing; two hours and the
 * ground round a base is bare for a whole evening. Long enough that where you
 * build matters, short enough that it recovers while you are off doing
 * something else.
 *
 * Nothing comes back under a floor. The timer runs, finds the ground taken,
 * and tries again in a minute and a half, so a node returns the moment the
 * base over it is gone rather than being deleted with it.
 */
constexpr double kRegrowthSeconds = 5400;
constexpr double kRegrowthSpread = 0.08;
/** How long it waits before asking again when something is standing on it. */
constexpr double kRegrowthRetry = 90;

}  // namespace

void World::beachSpawn(std::uint32_t roll, double& x, double& y) const {
	Rng rng(roll * 2654435761u + 1u);
	// Sand, and not the frozen sort: you start somewhere you can stand about.
	for (int tries = 0; tries < 4000; ++tries) {
		const int col = static_cast<int>(rng.unit() * kBiomeCols);
		const int row = static_cast<int>(rng.unit() * kBiomeRows);
		if (biomes_[row * kBiomeCols + col] != Biome::Beach) continue;
		x = (col + 0.5) * kBiomeTile;
		y = (row + 0.5) * kBiomeTile;
		return;
	}
	// An island with no sand at all: the middle will do.
	x = kWorldWidth * 0.5;
	y = kWorldHeight * 0.5;
}

double World::radiationAt(double x, double y) const {
	double depth = 0;
	const Monument* m = monumentAt(x, y, depth);
	if (!m) return 0;
	const MonumentDef& def = monumentDef(m->kind);
	if (def.rads <= 0) return 0;
	// Hotter the further in you go, and at its worst well before the middle.
	return def.rads * std::min(1.0, depth * 1.6);
}

ResourceNode* World::nodeById(int id) {
	if (id <= 0 || id > static_cast<int>(nodes_.size())) return nullptr;
	ResourceNode& node = nodes_[id - 1];
	return node.id == id ? &node : nullptr;
}

bool World::hurtNode(ResourceNode& node, double damage) {
	if (node.hp <= 0) return false;
	node.hp -= static_cast<int>(damage);
	tookDamage(node);
	node.shake = 0.16;
	if (node.hp > 0) return false;
	node.hp = 0;
	Rng rng(respawnSeed_ += 0x9e3779b9u);
	const double slack = kRegrowthSeconds * kRegrowthSpread;
	node.respawn = kRegrowthSeconds + rng.range(-slack, slack);

	// What was inside spills where it stood, scattered a little so the stacks
	// do not sit in one pile, for whoever gets there.
	if (nodeDef(node.kind).loot) {
		Rng loot(node.seed * 2654435761u + 17u);
		for (const Loot& entry : kBarrelLoot) {
			if (loot.unit() > entry.chance) continue;
			const int amount = static_cast<int>(std::lround(loot.range(entry.low, entry.high)));
			if (amount <= 0) continue;
			const double a = loot.unit() * 6.28318530717959;
			const double d = loot.range(4, 16);
			dropStack(ItemStack{entry.id, amount}, node.x + std::cos(a) * d,
					  node.y + std::sin(a) * d);
		}
	}
	return true;
}

void World::dropStack(ItemStack stack, double x, double y) {
	if (stack.id == ItemId::None || stack.count <= 0) return;
	// Every drop is pushed clear of anything solid, here rather than at each
	// of the dozen places that drop something: a stack inside a wall or under
	// a boulder is a stack nobody can pick up, and the callers that get it
	// right are not the ones that matter. See docs/systems/dropped-items.md.
	if (settle_) settle_(settleOwner_, *this, x, y, kDropClearance);
	drops_.push_back(Dropped{nextDropId_++, stack, x, y, 0});
}

void World::setDropSettler(const void* owner,
						   void (*settle)(const void*, const World&, double&, double&, double)) {
	settleOwner_ = owner;
	settle_ = settle;
}

void World::removeDrop(int id) {
	drops_.erase(std::remove_if(drops_.begin(), drops_.end(),
								[id](const Dropped& d) { return d.id == id; }),
				 drops_.end());
}

void World::setRegrowthBlocked(const void* owner, bool (*blocked)(const void*, double, double)) {
	blockedOwner_ = owner;
	blocked_ = blocked;
}

int World::clearNaturalIn(double x0, double y0, double x1, double y1) {
	static thread_local std::vector<const ResourceNode*> found;
	nodesInRect(x0, y0, x1, y1, found);
	int cleared = 0;
	for (const ResourceNode* node : found) {
		if (node->hp <= 0) continue;
		ResourceNode* live = nodeById(node->id);
		if (!live) continue;
		// The ordinary timer, not a deletion: it comes back if the building
		// that took its place ever comes down.
		live->hp = 0;
		Rng rng(respawnSeed_ += 0x9e3779b9u);
		const double slack = kRegrowthSeconds * kRegrowthSpread;
		live->respawn = kRegrowthSeconds + rng.range(-slack, slack);
		++cleared;
	}
	return cleared;
}

void World::update(double dt) {
	// An emptied crate fills again a while later, so a monument is worth
	// walking back to rather than being used up.
	for (LootCrate& crate : crates_) {
		if (!crate.looted) continue;
		crate.respawn -= dt;
		if (crate.respawn > 0) continue;
		fillCrate(crate, static_cast<std::uint32_t>(crate.id * 7919 + respawnSeed_++));
	}
	for (ResourceNode& node : nodes_) {
		ageWound(node, dt);
		if (node.shake > 0) node.shake -= dt;
		if (node.respawn <= 0) continue;
		node.respawn -= dt;
		if (node.respawn > 0) continue;
		if (blocked_ && blocked_(blockedOwner_, node.x, node.y)) {
			// Something is standing on it. Not its timer, a check to run again
			// once whatever is there might have come down.
			node.respawn = kRegrowthRetry;
			continue;
		}
		node.respawn = 0;
		node.hp = node.maxHp;
	}
	for (Dropped& drop : drops_) drop.age += dt;
	drops_.erase(std::remove_if(drops_.begin(), drops_.end(),
								[](const Dropped& d) { return d.age > kDropLifetime; }),
				 drops_.end());
}

}  // namespace sim
