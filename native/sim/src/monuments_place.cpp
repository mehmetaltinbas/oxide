#include <algorithm>
#include <cmath>

#include "sim/world.hpp"

namespace sim {

namespace {

constexpr double kTau = 6.28318530717959;

/** Where the big places start out from each other: a third of the short side. */
double apartStart() { return std::min(kWorldWidth, kWorldHeight) / 3.0; }

}  // namespace

bool World::onCoast(double x, double y) const {
    // Open sea within a couple of hundred, which is what makes a shore a shore.
    for (int i = 0; i < 16; ++i) {
        const double a = i / 16.0 * kTau;
        for (double r = 80; r <= 260; r += 60) {
            const double px = x + std::cos(a) * r;
            const double py = y + std::sin(a) * r;
            if (biomeAt(px, py) == Biome::Water && !freshAt(px, py)) return true;
        }
    }
    return false;
}

void World::spawnCrates(const MonumentDef& def, MonumentKind kind, double mx, double my, Rng& rng) {
    for (int i = 0; i < def.crates; ++i) {
        const double a = rng.unit() * kTau;
        const double r = rng.range(def.radius * 0.2, def.radius * 0.78);
        LootCrate crate{};
        crate.id = static_cast<int>(crates_.size()) + 1;
        crate.monument = kind;
        crate.x = mx + std::cos(a) * r;
        crate.y = my + std::sin(a) * r;
        crate.container.slots.assign(6, ItemStack{});
        fillCrate(crate, static_cast<std::uint32_t>(rng.unit() * 1e6));
        crates_.push_back(crate);
    }
}

void World::fillCrate(LootCrate& crate, std::uint32_t roll) {
    const MonumentDef& def = monumentDef(crate.monument);
    crate.container.slots.assign(6, ItemStack{});
    Rng rng(roll * 2654435761u + 11u);
    int slot = 0;
    for (int i = 0; i < def.lootCount && slot < 6; ++i) {
        // Not everything every time: a crate is a handful of its table.
        if (rng.unit() < 0.45) continue;
        const int amount = static_cast<int>(std::lround(rng.range(def.loot[i].low, def.loot[i].high)));
        if (amount <= 0) continue;
        crate.container.slots[slot++] =
            ItemStack{def.loot[i].id, std::min(amount, itemDef(def.loot[i].id).stack)};
    }
    if (slot == 0 && def.lootCount > 0) {
        // Never empty: a crate you walked across the island for owes you one.
        const int i = static_cast<int>(rng.unit() * def.lootCount);
        const int amount = std::max(
            1, static_cast<int>(std::lround(rng.range(def.loot[i].low, def.loot[i].high))));
        crate.container.slots[0] = ItemStack{def.loot[i].id, amount};
    }
    crate.looted = false;
}

void World::placeMonuments(Rng& rng) {
    monuments_.clear();
    crates_.clear();
    // The landmarks go down first and far apart, then everything else fills in
    // round them. The other way about, the small stuff takes the room the big
    // places need and they end up in each other's pockets.
    MonumentKind order[kMonumentKindCount];
    int n = 0;
    for (int i = 0; i < kMonumentKindCount; ++i) {
        if (monumentDef(static_cast<MonumentKind>(i)).major) order[n++] = static_cast<MonumentKind>(i);
    }
    for (int i = 0; i < kMonumentKindCount; ++i) {
        if (!monumentDef(static_cast<MonumentKind>(i)).major) {
            order[n++] = static_cast<MonumentKind>(i);
        }
    }

    for (int o = 0; o < kMonumentKindCount; ++o) {
        const MonumentKind kind = order[o];
        const MonumentDef& def = monumentDef(kind);
        for (int copy = 0; copy < def.count; ++copy) {
            double apart = apartStart();
            bool placed = false;
            for (int round = 0; round < kMonumentRounds && !placed; ++round) {
                for (int attempt = 0; attempt < 400; ++attempt) {
                    const double x = rng.range(def.radius + 160, kWorldWidth - def.radius - 160);
                    const double y = rng.range(def.radius + 160, kWorldHeight - def.radius - 160);
                    const Biome biome = biomeAt(x, y);
                    if (biome == Biome::Water) continue;
                    // A base or an airfield on sand is somebody's beach hut.
                    if (def.major && (biome == Biome::Beach || biome == Biome::SnowBeach)) continue;
                    if (def.coast && !onCoast(x, y)) continue;
                    if (def.forestOrSnow && biome != Biome::Forest && biome != Biome::Snow) continue;
                    bool clash = false;
                    for (const Monument& other : monuments_) {
                        const MonumentDef& otherDef = monumentDef(other.kind);
                        const double need = def.major && otherDef.major
                                                ? apart
                                                : other.radius + def.radius + 900;
                        if (std::hypot(x - other.x, y - other.y) < need) {
                            clash = true;
                            break;
                        }
                    }
                    if (clash) continue;
                    monuments_.push_back(
                        Monument{static_cast<int>(monuments_.size()) + 1, kind, x, y, def.radius});
                    spawnCrates(def, kind, x, y, rng);
                    placed = true;
                    break;
                }
                // Nowhere that far from the others on this island: ask for a
                // little less and go round again, rather than dropping it.
                apart *= kMonumentRelax;
            }
        }
    }
}

const Monument* World::monumentAt(double x, double y, double& depth) const {
    for (const Monument& m : monuments_) {
        const double d = std::hypot(x - m.x, y - m.y);
        if (d > m.radius) continue;
        depth = 1 - d / m.radius;
        return &m;
    }
    return nullptr;
}

}  // namespace sim
