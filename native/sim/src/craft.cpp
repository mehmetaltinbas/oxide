#include "sim/craft.hpp"

#include <algorithm>

namespace sim {

namespace {

/**
 * The recipes, carried over from the TypeScript game.
 *
 * Only the tier-zero ones can be made at the moment, because a workbench is a
 * thing you build and building is not here yet. The rest are listed so the
 * costs live in one place when it is.
 */
const std::vector<Recipe>& table() {
    static const std::vector<Recipe> all = {
        {ItemId::Hatchet, 1, {{ItemId::Wood, 60}, {ItemId::Stone, 40}}, 2, 0, 4},
        {ItemId::Pickaxe, 1, {{ItemId::Wood, 60}, {ItemId::Stone, 60}}, 2, 0, 4},
        {ItemId::Hammer, 1, {{ItemId::Wood, 40}, {ItemId::Stone, 20}}, 2, 0, 3},
        // A hunting bow is the first weapon you make, as in Rust: no bench.
        {ItemId::Bow, 1, {{ItemId::Wood, 200}, {ItemId::Cloth, 50}}, 2, 0, 6},
        {ItemId::Spear, 1, {{ItemId::Wood, 60}, {ItemId::Cloth, 10}}, 2, 0, 4},
        {ItemId::LowGrade, 4, {{ItemId::AnimalFat, 3}, {ItemId::Cloth, 1}}, 2, 0, 2},

        {ItemId::Arrow, 8, {{ItemId::Wood, 40}, {ItemId::Stone, 15}}, 2, 1, 3},
        {ItemId::Gunpowder, 10, {{ItemId::Sulfur, 20}, {ItemId::Charcoal, 30}}, 2, 1, 3},
        {ItemId::Waterpipe, 1, {{ItemId::Wood, 150}, {ItemId::Metal, 75}}, 2, 1, 6},
        {ItemId::ShotgunShell, 4, {{ItemId::Gunpowder, 8}, {ItemId::Metal, 6}}, 2, 1, 3},

        {ItemId::Revolver, 1, {{ItemId::Metal, 150}, {ItemId::Scrap, 75}}, 2, 2, 8},
        {ItemId::PumpShotgun, 1, {{ItemId::Metal, 200}, {ItemId::Scrap, 120}}, 2, 2, 8},
        {ItemId::PistolAmmo, 12, {{ItemId::Gunpowder, 10}, {ItemId::Metal, 10}}, 2, 2, 3},

        {ItemId::Rifle, 1, {{ItemId::Metal, 450}, {ItemId::Scrap, 300}}, 2, 3, 12},
        {ItemId::Ak47, 1,
         {{ItemId::Metal, 600}, {ItemId::Scrap, 450}, {ItemId::Wood, 200}}, 3, 3, 15},
        {ItemId::RifleAmmo, 12, {{ItemId::Gunpowder, 20}, {ItemId::Metal, 15}}, 2, 3, 3},
    };
    return all;
}

}  // namespace

const std::vector<Recipe>& recipes() { return table(); }

bool canAfford(const Inventory& inventory, const Recipe& recipe) {
    for (int i = 0; i < recipe.costCount; ++i) {
        if (inventory.count(recipe.cost[i].id) < recipe.cost[i].count) return false;
    }
    return true;
}

bool Crafting::queue(Inventory& inventory, const Recipe& recipe) {
    if (static_cast<int>(jobs_.size()) >= kQueueMax) return false;
    if (!canAfford(inventory, recipe)) return false;
    for (int i = 0; i < recipe.costCount; ++i) {
        inventory.take(recipe.cost[i].id, recipe.cost[i].count);
    }
    jobs_.push_back(CraftJob{nextId_++, &recipe, recipe.seconds});
    return true;
}

void Crafting::cancel(Inventory& inventory, int jobId) {
    const auto it = std::find_if(jobs_.begin(), jobs_.end(),
                                 [jobId](const CraftJob& job) { return job.id == jobId; });
    if (it == jobs_.end()) return;
    for (int i = 0; i < it->recipe->costCount; ++i) {
        inventory.add(it->recipe->cost[i].id, it->recipe->cost[i].count);
    }
    jobs_.erase(it);
}

void Crafting::update(double dt, Inventory& inventory) {
    if (jobs_.empty()) return;
    CraftJob& job = jobs_.front();
    job.left -= dt;
    if (job.left > 0) return;
    // A pack with no room for it keeps the job waiting rather than losing it.
    const int left = inventory.add(job.recipe->out, job.recipe->amount);
    if (left > 0) {
        job.left = 0.25;
        return;
    }
    jobs_.erase(jobs_.begin());
}

}  // namespace sim
