#include "sim/world.hpp"

#include <algorithm>
#include <utility>
#include <cmath>
#include <vector>

namespace sim {

namespace {

/** How far in from the rim the island is allowed to reach, as a fraction. */
constexpr double kIslandMargin = 0.03;
/** Over what fraction the land fades into open sea at the rim. */
constexpr double kIslandFalloff = 0.11;
/** How wide a beach grows, in tiles. */
constexpr int kBeachWidth = 1;
/** Tiles of sea kept round the very edge, so nothing is stranded on the rim. */
constexpr int kRim = 2;
/** The snowfield is closed up this many times, one tile per pass. */
constexpr int kSnowPasses = 4;
/** A grass or forest tile with this many snow neighbours is snow. */
constexpr int kSnowNeighbours = 4;
/** A green pocket up to this many tiles, mostly ringed by snow, is snowed over. */
constexpr int kSnowPocketTiles = 400;
constexpr double kSnowPocketShare = 0.6;
/** Tiles of grass kept between the forest and the desert. */
constexpr int kForestDesertBelt = 2;

double clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

bool isGreen(Biome b) { return b == Biome::Grass || b == Biome::Forest; }

}  // namespace

void World::generate(std::uint32_t seed) {
    seed_ = seed;
    Rng rng(seed);
    generateBiomes(rng);
    closeSnowfield();
    separateForestAndDesert();
    drainLakesOnTheRoad();
    growBeaches();

    // A hard sea border, so nothing can be built or stranded on the very rim.
    for (int y = 0; y < kBiomeRows; ++y) {
        for (int x = 0; x < kBiomeCols; ++x) {
            if (x < kRim || y < kRim || x >= kBiomeCols - kRim || y >= kBiomeRows - kRim) {
                biomes_[y * kBiomeCols + x] = Biome::Water;
            }
        }
    }

    // The ring road, laid on whatever is dry. The lakes in its way were drained
    // above; the sea stops it rather than being crossed.
    const std::vector<std::uint8_t> road = roadMask();
    for (std::size_t i = 0; i < road.size(); ++i) {
        if (road[i] && biomes_[i] != Biome::Water) biomes_[i] = Biome::Road;
    }

    markFreshWater();
    scatterNodes(rng);
}

void World::generateBiomes(Rng& rng) {
    // A coarse field of noise, smoothed between its corners: the same twelve by
    // twelve grid the TypeScript game used, so the islands match.
    constexpr int kCoarse = 12;
    double field[kCoarse * kCoarse];
    for (double& v : field) v = rng.unit();

    const auto sample = [&](double fx, double fy) {
        const int gx = std::min(kCoarse - 2, static_cast<int>(std::floor(fx * (kCoarse - 1))));
        const int gy = std::min(kCoarse - 2, static_cast<int>(std::floor(fy * (kCoarse - 1))));
        const double tx = fx * (kCoarse - 1) - gx;
        const double ty = fy * (kCoarse - 1) - gy;
        const double a = field[gy * kCoarse + gx];
        const double b = field[gy * kCoarse + gx + 1];
        const double c = field[(gy + 1) * kCoarse + gx];
        const double d = field[(gy + 1) * kCoarse + gx + 1];
        const double sx = tx * tx * (3 - 2 * tx);
        const double sy = ty * ty * (3 - 2 * ty);
        return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy;
    };

    biomes_.assign(kBiomeCols * kBiomeRows, Biome::Water);
    for (int y = 0; y < kBiomeRows; ++y) {
        for (int x = 0; x < kBiomeCols; ++x) {
            const double fx = static_cast<double>(x) / kBiomeCols;
            const double fy = static_cast<double>(y) / kBiomeRows;
            const double n = sample(fx, fy);
            // North is cold, south is hot.
            const double band = fy;
            // The land fades into open sea at the rim rather than ending in a wall.
            const double edge = std::min(std::min(fx, 1 - fx), std::min(fy, 1 - fy));
            const double shore = clamp01((edge - kIslandMargin) / kIslandFalloff);
            const double height = n * (0.5 + 0.5 * shore) - (1 - shore) * 0.45;

            Biome b;
            if (height < 0.17) {
                b = Biome::Water;
            } else if (band < 0.24 && height < 0.6 && n > 0.34 - (0.24 - band)) {
                // The bands are ragged, not stripes: dry ground in the south
                // turns to desert and wet ground stays green.
                b = Biome::Snow;
            } else if (band > 0.76 && height < 0.6 && n > 0.34 - (band - 0.76)) {
                b = Biome::Desert;
            } else if (height > 0.6) {
                b = Biome::Forest;
            } else {
                b = Biome::Grass;
            }
            biomes_[y * kBiomeCols + x] = b;
        }
    }
}

void World::closeSnowfield() {
    // The snow test is on noise, so here and there a tile in the cold north
    // fails it and comes out as grass: a green square in the middle of the
    // snow. A tile mostly surrounded by snow is snow.
    for (int pass = 0; pass < kSnowPasses; ++pass) {
        const std::vector<Biome> was = biomes_;
        for (int y = 1; y < kBiomeRows - 1; ++y) {
            for (int x = 1; x < kBiomeCols - 1; ++x) {
                if (!isGreen(was[y * kBiomeCols + x])) continue;
                int snowy = 0;
                for (int oy = -1; oy <= 1; ++oy) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        if ((ox || oy) && was[(y + oy) * kBiomeCols + x + ox] == Biome::Snow) {
                            ++snowy;
                        }
                    }
                }
                if (snowy >= kSnowNeighbours) biomes_[y * kBiomeCols + x] = Biome::Snow;
            }
        }
    }

    // Then whatever green is still cut off inside the snow. Counting
    // neighbours reaches only a patch's rim; this takes the whole thing.
    std::vector<std::uint8_t> seen(biomes_.size(), 0);
    std::vector<int> stack;
    std::vector<int> patch;
    for (std::size_t start = 0; start < biomes_.size(); ++start) {
        if (seen[start] || !isGreen(biomes_[start])) continue;
        patch.clear();
        int snowEdge = 0;
        int otherEdge = 0;
        stack.push_back(static_cast<int>(start));
        seen[start] = 1;
        while (!stack.empty()) {
            const int i = stack.back();
            stack.pop_back();
            patch.push_back(i);
            const int x = i % kBiomeCols;
            const int y = i / kBiomeCols;
            const int steps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& step : steps) {
                const int nx = x + step[0];
                const int ny = y + step[1];
                if (nx < 0 || ny < 0 || nx >= kBiomeCols || ny >= kBiomeRows) continue;
                const int j = ny * kBiomeCols + nx;
                const Biome nb = biomes_[j];
                if (isGreen(nb)) {
                    if (!seen[j]) {
                        seen[j] = 1;
                        stack.push_back(j);
                    }
                } else if (nb == Biome::Snow) {
                    ++snowEdge;
                } else if (nb != Biome::Water) {
                    ++otherEdge;
                }
            }
        }
        const int edge = snowEdge + otherEdge;
        if (static_cast<int>(patch.size()) <= kSnowPocketTiles && edge > 0 &&
            static_cast<double>(snowEdge) / edge >= kSnowPocketShare) {
            for (const int k : patch) biomes_[k] = Biome::Snow;
        }
    }
}

void World::separateForestAndDesert() {
    // They are the two extremes of this island and the noise happily puts them
    // side by side, which reads as pine trees growing out of dunes. A belt of
    // grass between them is what the ground would actually do.
    for (int pass = 0; pass < kForestDesertBelt; ++pass) {
        const std::vector<Biome> was = biomes_;
        for (int y = 1; y < kBiomeRows - 1; ++y) {
            for (int x = 1; x < kBiomeCols - 1; ++x) {
                if (was[y * kBiomeCols + x] != Biome::Forest) continue;
                bool touchesDesert = false;
                for (int oy = -1; oy <= 1 && !touchesDesert; ++oy) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        if (was[(y + oy) * kBiomeCols + x + ox] == Biome::Desert) {
                            touchesDesert = true;
                            break;
                        }
                    }
                }
                if (touchesDesert) biomes_[y * kBiomeCols + x] = Biome::Grass;
            }
        }
    }
}

void World::drainLakesOnTheRoad() {
    // A road is laid along a fixed ring, and a pond that happens to sit on it
    // left either a gap in the road or a causeway down the middle of the water.
    // The whole pond goes, not the part under the road.
    const std::vector<std::uint8_t> onRoad = roadMask();
    const std::vector<std::uint8_t> sea = seaMask();
    std::vector<std::uint8_t> seen(biomes_.size(), 0);
    std::vector<int> stack;
    std::vector<int> lake;
    for (std::size_t start = 0; start < biomes_.size(); ++start) {
        if (seen[start] || biomes_[start] != Biome::Water || sea[start]) continue;
        lake.clear();
        bool crossed = false;
        stack.push_back(static_cast<int>(start));
        seen[start] = 1;
        while (!stack.empty()) {
            const int i = stack.back();
            stack.pop_back();
            lake.push_back(i);
            if (onRoad[i]) crossed = true;
            const int x = i % kBiomeCols;
            const int y = i / kBiomeCols;
            const int steps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& step : steps) {
                const int nx = x + step[0];
                const int ny = y + step[1];
                if (nx < 0 || ny < 0 || nx >= kBiomeCols || ny >= kBiomeRows) continue;
                const int j = ny * kBiomeCols + nx;
                if (seen[j] || biomes_[j] != Biome::Water || sea[j]) continue;
                seen[j] = 1;
                stack.push_back(j);
            }
        }
        if (!crossed) continue;
        for (const int k : lake) biomes_[k] = landAround(k);
    }
}

void World::growBeaches() {
    // Only the sea makes a beach: a lake has a green bank, and people wash up
    // out of the sea. Desert is already sand and grows none; a snow coast gets
    // one under snow, and nobody wakes up there.
    const std::vector<Biome> base = biomes_;
    const std::vector<std::uint8_t> sea = seaMask();
    for (int y = 0; y < kBiomeRows; ++y) {
        for (int x = 0; x < kBiomeCols; ++x) {
            const Biome here = base[y * kBiomeCols + x];
            if (!isGreen(here) && here != Biome::Snow) continue;
            bool coastal = false;
            for (int oy = -kBeachWidth; oy <= kBeachWidth && !coastal; ++oy) {
                for (int ox = -kBeachWidth; ox <= kBeachWidth; ++ox) {
                    const int tx = x + ox;
                    const int ty = y + oy;
                    if (tx < 0 || ty < 0 || tx >= kBiomeCols || ty >= kBiomeRows) continue;
                    const int j = ty * kBiomeCols + tx;
                    if (base[j] == Biome::Water && sea[j]) {
                        coastal = true;
                        break;
                    }
                }
            }
            if (coastal) {
                biomes_[y * kBiomeCols + x] =
                    here == Biome::Snow ? Biome::SnowBeach : Biome::Beach;
            }
        }
    }
}

std::vector<std::uint8_t> World::seaMask() const {
    std::vector<std::uint8_t> sea(biomes_.size(), 0);
    std::vector<int> stack;
    for (int x = 0; x < kBiomeCols; ++x) {
        stack.push_back(x);
        stack.push_back((kBiomeRows - 1) * kBiomeCols + x);
    }
    for (int y = 0; y < kBiomeRows; ++y) {
        stack.push_back(y * kBiomeCols);
        stack.push_back(y * kBiomeCols + kBiomeCols - 1);
    }
    while (!stack.empty()) {
        const int i = stack.back();
        stack.pop_back();
        if (sea[i] || biomes_[i] != Biome::Water) continue;
        sea[i] = 1;
        const int x = i % kBiomeCols;
        const int y = i / kBiomeCols;
        if (x > 0) stack.push_back(i - 1);
        if (x < kBiomeCols - 1) stack.push_back(i + 1);
        if (y > 0) stack.push_back(i - kBiomeCols);
        if (y < kBiomeRows - 1) stack.push_back(i + kBiomeCols);
    }
    return sea;
}

std::vector<std::uint8_t> World::roadMask() const {
    std::vector<std::uint8_t> mask(kBiomeCols * kBiomeRows, 0);
    const double cx = kBiomeCols / 2.0;
    const double cy = kBiomeRows / 2.0;
    const double rr = std::min(kBiomeCols, kBiomeRows) * 0.34;
    for (int a = 0; a < 720; ++a) {
        const double ang = (a / 720.0) * 6.283185307179586;
        const double wobble = 1 + std::sin(ang * 3) * 0.08;
        const int x = static_cast<int>(std::lround(cx + std::cos(ang) * rr * wobble));
        const int y = static_cast<int>(std::lround(cy + std::sin(ang) * rr * wobble * 0.9));
        for (int ox = -1; ox <= 1; ++ox) {
            for (int oy = -1; oy <= 1; ++oy) {
                const int tx = x + ox;
                const int ty = y + oy;
                if (tx < 0 || ty < 0 || tx >= kBiomeCols || ty >= kBiomeRows) continue;
                mask[ty * kBiomeCols + tx] = 1;
            }
        }
    }
    return mask;
}

Biome World::landAround(int index) const {
    const int x0 = index % kBiomeCols;
    const int y0 = index / kBiomeCols;
    // Counted in the order the tiles are seen, and ties go to whichever was
    // seen first. That is what the TypeScript generator does, and a drained
    // lake bed is often an exact tie between grass and snow: pick differently
    // and the two islands stop matching.
    std::vector<std::pair<Biome, int>> votes;
    for (int r = 1; r <= 6; ++r) {
        for (int oy = -r; oy <= r; ++oy) {
            for (int ox = -r; ox <= r; ++ox) {
                const int tx = x0 + ox;
                const int ty = y0 + oy;
                if (tx < 0 || ty < 0 || tx >= kBiomeCols || ty >= kBiomeRows) continue;
                const Biome b = biomes_[ty * kBiomeCols + tx];
                if (b == Biome::Water) continue;
                auto it = std::find_if(votes.begin(), votes.end(),
                                       [b](const auto& v) { return v.first == b; });
                if (it == votes.end()) {
                    votes.emplace_back(b, 1);
                } else {
                    ++it->second;
                }
            }
        }
        if (!votes.empty()) break;
    }
    Biome best = Biome::Grass;
    int bestCount = 0;
    for (const auto& [biome, count] : votes) {
        if (count > bestCount) {
            bestCount = count;
            best = biome;
        }
    }
    return best;
}

void World::markFreshWater() {
    const std::vector<std::uint8_t> sea = seaMask();
    fresh_.assign(biomes_.size(), 0);
    for (std::size_t i = 0; i < fresh_.size(); ++i) {
        if (biomes_[i] == Biome::Water && !sea[i]) fresh_[i] = 1;
    }
}

Biome World::biomeAt(double x, double y) const {
    const int bx = static_cast<int>(std::floor(x / kBiomeTile));
    const int by = static_cast<int>(std::floor(y / kBiomeTile));
    if (bx < 0 || by < 0 || bx >= kBiomeCols || by >= kBiomeRows) return Biome::Water;
    return biomes_[by * kBiomeCols + bx];
}

bool World::freshAt(double x, double y) const {
    const int bx = static_cast<int>(std::floor(x / kBiomeTile));
    const int by = static_cast<int>(std::floor(y / kBiomeTile));
    if (bx < 0 || by < 0 || bx >= kBiomeCols || by >= kBiomeRows) return false;
    return fresh_[by * kBiomeCols + bx] == 1;
}

}  // namespace sim
