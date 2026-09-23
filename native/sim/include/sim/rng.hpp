#pragma once

#include <cstdint>

namespace sim {

/**
 * The island's random numbers: mulberry32, the same generator the TypeScript
 * game used, down to the arithmetic.
 *
 * Kept identical on purpose. The same seed has to build the same island on
 * every machine, and while the port runs it also has to build the same island
 * as the old game, so the two can be compared square by square. That check is
 * worth more than a nicer generator.
 */
class Rng {
public:
    explicit Rng(std::uint32_t seed) : state_(seed) {}

    /** A number in [0, 1). */
    double unit();

    /** A number in [lo, hi). */
    double range(double lo, double hi);

private:
    std::uint32_t state_;
};

/**
 * A number in [0, 1) from a seed and a slot, the same every time it is asked.
 *
 * For anything that must look random but never change: which way a tree leans,
 * where the snow lies on it. Asked while drawing, so it must not roll.
 */
double seeded(std::uint64_t seed, std::uint64_t slot);

}  // namespace sim
