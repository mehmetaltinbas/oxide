// Prints a tally of the island a seed builds, so the C++ generator can be
// compared square by square with the TypeScript one it was ported from.
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "sim/world.hpp"

int main(int argc, char** argv) {
    const std::uint32_t seed = argc > 1 ? static_cast<std::uint32_t>(std::strtoul(argv[1], nullptr, 10)) : 12345u;
    sim::World world;
    world.generate(seed);
    int counts[sim::kBiomeCount] = {0};
    int fresh = 0;
    for (int y = 0; y < sim::kBiomeRows; ++y) {
        for (int x = 0; x < sim::kBiomeCols; ++x) {
            ++counts[static_cast<int>(world.tile(x, y))];
            if (world.freshAt(x * sim::kBiomeTile + 1.0, y * sim::kBiomeTile + 1.0)) ++fresh;
        }
    }
    int nodes[sim::kNodeKindCount] = {0};
    for (const sim::ResourceNode& n : world.nodes()) ++nodes[static_cast<int>(n.kind)];
    int crates = 0;
    for (const sim::LootCrate& crate : world.crates()) {
        (void)crate;
        ++crates;
    }
    std::printf("{\"seed\":%u,\"water\":%d,\"grass\":%d,\"forest\":%d,\"beach\":%d,\"snow_beach\":%d,\"desert\":%d,\"snow\":%d,\"road\":%d,\"fresh\":%d,\"tree\":%d,\"stone\":%d,\"metal\":%d,\"sulfur\":%d,\"nettle\":%d,\"barrel\":%d,\"monuments\":%zu,\"crates\":%d}\n",
                seed, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7], fresh, nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], world.monuments().size(), crates);
    return 0;
}
