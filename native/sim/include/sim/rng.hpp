#pragma once

#include <cstdint>

namespace sim {

/**
 * The island's random numbers.
 *
 * Deterministic and seeded: the same seed builds the same island on every
 * machine, which is what lets the server and every client generate the world
 * from a single number instead of sending it over the wire. It is xorshift
 * rather than the standard library's generators because those are free to
 * differ between platforms, and an island that is not identical on Windows and
 * Mac would be a bug nobody could see until two players compared maps.
 */
class Rng {
public:
    explicit Rng(std::uint64_t seed) : state_(seed ? seed : 0x9e3779b97f4a7c15ULL) {}

    /** The next raw number. */
    std::uint64_t next();

    /** A number from 0 up to but not including 1. */
    double unit();

    /** A number in [lo, hi). */
    double range(double lo, double hi);

private:
    std::uint64_t state_;
};

/**
 * A number in [0, 1) from a seed and a slot, the same every time it is asked.
 *
 * For anything that must look random but never change: which way a tree leans,
 * where the snow lies on it. Asked during drawing, so it must not roll.
 */
double seeded(std::uint64_t seed, std::uint64_t slot);

}  // namespace sim
