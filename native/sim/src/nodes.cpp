#include <algorithm>
#include <cmath>

#include "sim/world.hpp"

namespace sim {

namespace {

/** How thick the trees stand, and how far apart they keep. */
constexpr double kTreeSpacing = 58;
constexpr double kForestPerTile = 1.09;
constexpr double kGrassShare = 0.25;
constexpr double kSnowChance = 0.35;

/** Ore: a fixed count per region on a map of this area, scaled with the map. */
constexpr double kOreSpacing = 76;
constexpr double kOreReferenceArea = 20736.0 * 20736.0;

/** Nettle, per tile of grass or forest. */
constexpr double kNettlePerTile = 0.1;
constexpr double kNettleSpacing = 52;

/** Barrels along the verges. */
constexpr double kBarrelChancePerTile = 0.33;
constexpr double kBarrelSpacing = 220;
constexpr double kBarrelClearance = 40;
constexpr int kBarrelMaxCluster = 3;
constexpr double kBarrelGap = 29;
constexpr double kBarrelVerge = 16;

/** One region's share of the ore, and which three it leans to. */
struct OreRegion {
    Biome biomes[2];
    int biomeCount;
    int count;
    NodeKind mix[3];
    double shares[3];
};

// Stone near where you start, metal up in the cold, sulfur down in the heat.
constexpr OreRegion kOreRegions[3] = {
    {{Biome::Grass, Biome::Forest}, 2, 500,
     {NodeKind::Stone, NodeKind::Metal, NodeKind::Sulfur}, {0.45, 0.30, 0.25}},
    {{Biome::Snow, Biome::Snow}, 1, 750,
     {NodeKind::Metal, NodeKind::Stone, NodeKind::Sulfur}, {0.45, 0.30, 0.25}},
    {{Biome::Desert, Biome::Desert}, 1, 750,
     {NodeKind::Sulfur, NodeKind::Stone, NodeKind::Metal}, {0.45, 0.30, 0.25}},
};

/** The bucket grid for looking things up by place. */
constexpr double kBucket = 128;
constexpr int kBucketCols = static_cast<int>(kWorldWidth / kBucket) + 1;
constexpr int kBucketRows = static_cast<int>(kWorldHeight / kBucket) + 1;

double dist2(double ax, double ay, double bx, double by) {
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx * dx + dy * dy;
}

}  // namespace

void World::indexNodes() {
    buckets_.assign(kBucketCols * kBucketRows, {});
    for (int i = 0; i < static_cast<int>(nodes_.size()); ++i) {
        const ResourceNode& n = nodes_[i];
        const int bx = std::clamp(static_cast<int>(n.x / kBucket), 0, kBucketCols - 1);
        const int by = std::clamp(static_cast<int>(n.y / kBucket), 0, kBucketRows - 1);
        buckets_[by * kBucketCols + bx].push_back(i);
    }
}

void World::nodesInRect(double x0, double y0, double x1, double y1,
                        std::vector<const ResourceNode*>& out) const {
    out.clear();
    const int bx0 = std::clamp(static_cast<int>(x0 / kBucket), 0, kBucketCols - 1);
    const int bx1 = std::clamp(static_cast<int>(x1 / kBucket), 0, kBucketCols - 1);
    const int by0 = std::clamp(static_cast<int>(y0 / kBucket), 0, kBucketRows - 1);
    const int by1 = std::clamp(static_cast<int>(y1 / kBucket), 0, kBucketRows - 1);
    for (int by = by0; by <= by1; ++by) {
        for (int bx = bx0; bx <= bx1; ++bx) {
            for (const int i : buckets_[by * kBucketCols + bx]) {
                const ResourceNode& n = nodes_[i];
                if (n.x < x0 || n.x > x1 || n.y < y0 || n.y > y1) continue;
                out.push_back(&n);
            }
        }
    }
}

void World::scatterNodes(Rng& rng) {
    nodes_.clear();
    // Placement checks what is already down, so the buckets are filled as we go.
    std::vector<std::vector<int>> placing(kBucketCols * kBucketRows);

    const auto biomeIsOneOf = [](Biome b, const Biome* list, int count) {
        for (int i = 0; i < count; ++i) {
            if (list[i] == b) return true;
        }
        return false;
    };

    const auto free = [&](double x, double y, double min) {
        if (biomeAt(x, y) == Biome::Water) return false;
        // Nothing grows or sits on a road or a beach, not even overhanging one.
        const double edge = min * 0.5;
        const double probes[5][2] = {{0, 0}, {-edge, 0}, {edge, 0}, {0, -edge}, {0, edge}};
        for (const auto& p : probes) {
            const Biome b = biomeAt(x + p[0], y + p[1]);
            if (b == Biome::Road || b == Biome::Beach || b == Biome::SnowBeach) return false;
        }
        // Monuments keep their ground clear, as they did in the other game.
        for (const Monument& m : monuments_) {
            if (std::hypot(x - m.x, y - m.y) < m.radius + 40) return false;
        }
        const double m2 = min * min;
        const int reach = static_cast<int>(min / kBucket) + 1;
        const int bx = std::clamp(static_cast<int>(x / kBucket), 0, kBucketCols - 1);
        const int by = std::clamp(static_cast<int>(y / kBucket), 0, kBucketRows - 1);
        for (int oy = -reach; oy <= reach; ++oy) {
            for (int ox = -reach; ox <= reach; ++ox) {
                const int cx = bx + ox;
                const int cy = by + oy;
                if (cx < 0 || cy < 0 || cx >= kBucketCols || cy >= kBucketRows) continue;
                for (const int i : placing[cy * kBucketCols + cx]) {
                    if (dist2(x, y, nodes_[i].x, nodes_[i].y) < m2) return false;
                }
            }
        }
        return true;
    };

    const auto push = [&](NodeKind kind, double x, double y) {
        const NodeDef& def = nodeDef(kind);
        ResourceNode node{};
        node.id = static_cast<int>(nodes_.size()) + 1;
        node.kind = kind;
        node.x = x;
        node.y = y;
        node.radius = def.radius * rng.range(0.85, 1.2);
        node.hp = def.hp;
        node.maxHp = def.hp;
        node.respawn = 0;
        node.shake = 0;
        node.seed = static_cast<std::uint32_t>(rng.unit() * 1e5);
        nodes_.push_back(node);
        const int bx = std::clamp(static_cast<int>(x / kBucket), 0, kBucketCols - 1);
        const int by = std::clamp(static_cast<int>(y / kBucket), 0, kBucketRows - 1);
        placing[by * kBucketCols + bx].push_back(static_cast<int>(nodes_.size()) - 1);
    };

    // The tiles of a biome, so a small biome is filled as reliably as a big one.
    const auto tilesOf = [&](const Biome* kinds, int count) {
        std::vector<int> out;
        for (int i = 0; i < static_cast<int>(biomes_.size()); ++i) {
            if (biomeIsOneOf(biomes_[i], kinds, count)) out.push_back(i);
        }
        return out;
    };
    const auto spotIn = [&](const std::vector<int>& tiles, double min, double& outX, double& outY) {
        if (tiles.empty()) return false;
        for (int t = 0; t < 12; ++t) {
            const int i = tiles[static_cast<std::size_t>(rng.unit() * tiles.size())];
            const int tx = i % kBiomeCols;
            const int ty = i / kBiomeCols;
            const double x = (tx + rng.unit()) * kBiomeTile;
            const double y = (ty + rng.unit()) * kBiomeTile;
            if (free(x, y, min)) {
                outX = x;
                outY = y;
                return true;
            }
        }
        return false;
    };
    const auto place = [&](const std::vector<int>& tiles, int count, double min, auto make) {
        int placed = 0;
        for (int tries = 0; placed < count && tries < count * 10; ++tries) {
            double x = 0;
            double y = 0;
            if (!spotIn(tiles, min, x, y)) continue;
            push(make(), x, y);
            ++placed;
        }
        return placed;
    };

    // Trees. The forest at its own density, the grassland at a quarter of what
    // the forest actually came out at, and the snow with its own thinner stand.
    const Biome forestOnly[1] = {Biome::Forest};
    const Biome grassOnly[1] = {Biome::Grass};
    const std::vector<int> forestTiles = tilesOf(forestOnly, 1);
    const std::vector<int> grassTiles = tilesOf(grassOnly, 1);
    place(forestTiles, static_cast<int>(std::lround(kForestPerTile * forestTiles.size())),
          kTreeSpacing, [] { return NodeKind::Tree; });
    const double forestDensity =
        forestTiles.empty() ? 0 : static_cast<double>(nodes_.size()) / forestTiles.size();
    place(grassTiles,
          static_cast<int>(std::lround(forestDensity * kGrassShare * grassTiles.size())),
          kTreeSpacing, [] { return NodeKind::Tree; });
    for (int i = 0; i < 176000; ++i) {
        const double x = rng.range(30, kWorldWidth - 30);
        const double y = rng.range(30, kWorldHeight - 30);
        if (biomeAt(x, y) != Biome::Snow) continue;
        if (rng.unit() > kSnowChance) continue;
        if (!free(x, y, kTreeSpacing)) continue;
        push(NodeKind::Tree, x, y);
    }

    // Ore, a fixed amount per region, scaled by the size of the map.
    const double oreScale = (static_cast<double>(kWorldWidth) * kWorldHeight) / kOreReferenceArea;
    for (const OreRegion& region : kOreRegions) {
        place(tilesOf(region.biomes, region.biomeCount),
              static_cast<int>(std::lround(region.count * oreScale)), kOreSpacing, [&] {
                  const double roll = rng.unit();
                  if (roll < region.shares[0]) return region.mix[0];
                  if (roll < region.shares[0] + region.shares[1]) return region.mix[1];
                  return region.mix[2];
              });
    }

    // Nettle, in the green only.
    const Biome green[2] = {Biome::Grass, Biome::Forest};
    const std::vector<int> greenTiles = tilesOf(green, 2);
    place(greenTiles, static_cast<int>(std::lround(kNettlePerTile * greenTiles.size())),
          kNettleSpacing, [] { return NodeKind::Nettle; });

    // Barrels, along the edges of the roads, in ones, twos and threes.
    for (int ty = 1; ty < kBiomeRows - 1; ++ty) {
        for (int tx = 1; tx < kBiomeCols - 1; ++tx) {
            if (biomes_[ty * kBiomeCols + tx] != Biome::Road) continue;
            int sides[4][2];
            int sideCount = 0;
            const int steps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& step : steps) {
                const Biome b = biomes_[(ty + step[1]) * kBiomeCols + tx + step[0]];
                if (b != Biome::Road && b != Biome::Water) {
                    sides[sideCount][0] = step[0];
                    sides[sideCount][1] = step[1];
                    ++sideCount;
                }
            }
            if (sideCount == 0) continue;
            if (rng.unit() > kBarrelChancePerTile) continue;
            const int pick = static_cast<int>(rng.unit() * sideCount);
            const int dx = sides[pick][0];
            const int dy = sides[pick][1];
            const double inset = kBiomeTile / 2.0 - kBarrelVerge;
            const double along = (rng.unit() - 0.5) * kBiomeTile * 0.4;
            const double x = (tx + 0.5) * kBiomeTile + dx * inset + (dy != 0 ? along : 0);
            const double y = (ty + 0.5) * kBiomeTile + dy * inset + (dx != 0 ? along : 0);
            // Spaced from other clusters, and clear of anything it would overlap.
            bool crowded = false;
            const double m2 = kBarrelSpacing * kBarrelSpacing;
            const double c2 = kBarrelClearance * kBarrelClearance;
            const int reach = static_cast<int>(kBarrelSpacing / kBucket) + 1;
            const int bx = std::clamp(static_cast<int>(x / kBucket), 0, kBucketCols - 1);
            const int by = std::clamp(static_cast<int>(y / kBucket), 0, kBucketRows - 1);
            for (int oy = -reach; oy <= reach && !crowded; ++oy) {
                for (int ox = -reach; ox <= reach && !crowded; ++ox) {
                    const int cx = bx + ox;
                    const int cy = by + oy;
                    if (cx < 0 || cy < 0 || cx >= kBucketCols || cy >= kBucketRows) continue;
                    for (const int i : placing[cy * kBucketCols + cx]) {
                        const double d2 = dist2(x, y, nodes_[i].x, nodes_[i].y);
                        if ((nodes_[i].kind == NodeKind::Barrel && d2 < m2) || d2 < c2) {
                            crowded = true;
                            break;
                        }
                    }
                }
            }
            if (crowded) continue;
            // One, two or three, side by side along the verge, the third tucked
            // in behind rather than in a line.
            const int count = 1 + static_cast<int>(rng.unit() * kBarrelMaxCluster);
            const int ax = dy != 0 ? 1 : 0;
            const int ay = dx != 0 ? 1 : 0;
            double spots[3][2];
            int spotCount = 0;
            const int pair = std::min(count, 2);
            for (int k = 0; k < pair; ++k) {
                const double off = (k - (pair - 1) / 2.0) * kBarrelGap;
                spots[spotCount][0] = x + ax * off;
                spots[spotCount][1] = y + ay * off;
                ++spotCount;
            }
            if (count >= 3) {
                const double back = kBarrelGap * 0.87;
                spots[spotCount][0] = x - dx * back;
                spots[spotCount][1] = y - dy * back;
                ++spotCount;
            }
            for (int k = 0; k < spotCount; ++k) {
                const double bxp = spots[k][0] + (rng.unit() - 0.5) * 4;
                const double byp = spots[k][1] + (rng.unit() - 0.5) * 4;
                if (biomeAt(bxp, byp) != Biome::Road) continue;
                push(NodeKind::Barrel, bxp, byp);
            }
        }
    }

    indexNodes();
}

}  // namespace sim
