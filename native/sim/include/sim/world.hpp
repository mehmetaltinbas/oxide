#pragma once

#include <cstdint>
#include <vector>

#include "sim/biome.hpp"
#include "sim/drop.hpp"
#include "sim/node.hpp"
#include "sim/rng.hpp"
#include "sim/world_size.hpp"

namespace sim {

/**
 * The island: what the ground is, everywhere.
 *
 * Built from a seed alone, so the server sends a number and every client
 * arrives at the same island rather than being sent a map. The generator is a
 * port of the TypeScript game's, step for step, so the two can be compared
 * while the rewrite runs.
 */
class World {
public:
    void generate(std::uint32_t seed);

    std::uint32_t seed() const { return seed_; }

    /** The ground under a world point. Off the map reads as open water. */
    Biome biomeAt(double x, double y) const;

    /**
     * How hot a spot is, in radiation a second.
     *
     * Nought everywhere until the monuments are placed: it is theirs to give.
     */
    double radiationAt(double x, double y) const;

    /** Whether a point is in a lake, as opposed to the sea or dry land. */
    bool freshAt(double x, double y) const;

    Biome tile(int col, int row) const { return biomes_[row * kBiomeCols + col]; }
    const std::vector<Biome>& tiles() const { return biomes_; }

    const std::vector<ResourceNode>& nodes() const { return nodes_; }

    /**
     * Everything standing in a rectangle of the world, appended to `out`.
     *
     * Drawing asks this every frame for whatever is on screen, so it reads a
     * grid of buckets rather than walking the tens of thousands of trees on the
     * island.
     */
    void nodesInRect(double x0, double y0, double x1, double y1,
                     std::vector<const ResourceNode*>& out) const;

    /**
     * A free spot of sand on the coast: where you wake up, with nothing.
     *
     * `roll` is any changing number, so two deaths in a row do not put you back
     * on the same grain of sand.
     */
    void beachSpawn(std::uint32_t roll, double& x, double& y) const;

    /** The one with this id, or nothing if it has been taken. */
    ResourceNode* nodeById(int id);

    /**
     * A blow on something that stands: it loses hit points, and says whether
     * that was the one that finished it.
     */
    bool hurtNode(ResourceNode& node, double damage);

    const std::vector<Dropped>& drops() const { return drops_; }
    /** Puts a stack on the ground, for whoever gets there first. */
    void dropStack(ItemStack stack, double x, double y);
    /** Takes one back off the ground. */
    void removeDrop(int id);

    /**
     * The island's own clock: what was taken grows back, and what was dropped
     * and left rots away.
     */
    void update(double dt);

private:
    void generateBiomes(Rng& rng);
    /** Grass and forest cut off inside the snow is snow; the noise leaves pockets. */
    void closeSnowfield();
    /** A belt of grass between the forest and the desert, which never meet. */
    void separateForestAndDesert();
    /** Any lake the road would cross is drained before the road is laid. */
    void drainLakesOnTheRoad();
    /** Sand where the land meets the sea, snowed over on a cold coast. */
    void growBeaches();
    /** Water that joins the rim, by a flood in from the edge of the map. */
    std::vector<std::uint8_t> seaMask() const;
    /** Where the ring road runs, laid on whatever is dry. */
    std::vector<std::uint8_t> roadMask() const;
    /** The land biome a drained tile should take: whatever most surrounds it. */
    Biome landAround(int index) const;
    void markFreshWater();
    /** Trees, ore, nettle and the roadside barrels. */
    void scatterNodes(Rng& rng);
    void indexNodes();

    std::uint32_t seed_ = 0;
    std::vector<Biome> biomes_;
    /** 1 where the water is a lake rather than the sea. */
    std::vector<std::uint8_t> fresh_;
    std::vector<ResourceNode> nodes_;
    std::vector<Dropped> drops_;
    int nextDropId_ = 1;
    /** Rolls the jitter on regrowth, so a field cleared in one sweep does not
     * all come back on the same tick. */
    std::uint32_t respawnSeed_ = 1;
    /** Node indices by bucket, for looking up what is near a point. */
    std::vector<std::vector<int>> buckets_;
};

}  // namespace sim
