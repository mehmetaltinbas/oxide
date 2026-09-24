#include <algorithm>

#include "sim/world.hpp"

namespace sim {

namespace {

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
