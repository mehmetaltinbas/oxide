#include <algorithm>
#include <cmath>

#include "sim/world.hpp"

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
constexpr double kRegrowthSeconds = 3600;
constexpr double kRegrowthSpread = 0.08;

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

ResourceNode* World::nodeById(int id) {
    if (id <= 0 || id > static_cast<int>(nodes_.size())) return nullptr;
    ResourceNode& node = nodes_[id - 1];
    return node.id == id ? &node : nullptr;
}

bool World::hurtNode(ResourceNode& node, double damage) {
    if (node.hp <= 0) return false;
    node.hp -= static_cast<int>(damage);
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
    drops_.push_back(Dropped{nextDropId_++, stack, x, y, 0});
}

void World::removeDrop(int id) {
    drops_.erase(std::remove_if(drops_.begin(), drops_.end(),
                                [id](const Dropped& d) { return d.id == id; }),
                 drops_.end());
}

void World::update(double dt) {
    for (ResourceNode& node : nodes_) {
        if (node.respawn <= 0) continue;
        node.respawn -= dt;
        if (node.respawn > 0) continue;
        node.respawn = 0;
        node.hp = node.maxHp;
    }
    for (Dropped& drop : drops_) drop.age += dt;
    drops_.erase(std::remove_if(drops_.begin(), drops_.end(),
                                [](const Dropped& d) { return d.age > kDropLifetime; }),
                 drops_.end());
}

}  // namespace sim
