#include "sim/rng.hpp"

#include <cmath>

namespace sim {

std::uint64_t Rng::next() {
    // xorshift64*, small and fast, and the same everywhere.
    state_ ^= state_ >> 12;
    state_ ^= state_ << 25;
    state_ ^= state_ >> 27;
    return state_ * 0x2545f4914f6cdd1dULL;
}

double Rng::unit() {
    // The top 53 bits, which is exactly what a double can hold without rounding.
    return static_cast<double>(next() >> 11) / 9007199254740992.0;
}

double Rng::range(double lo, double hi) { return lo + unit() * (hi - lo); }

double seeded(std::uint64_t seed, std::uint64_t slot) {
    std::uint64_t h = seed * 0x9e3779b97f4a7c15ULL + slot * 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return static_cast<double>(h >> 11) / 9007199254740992.0;
}

}  // namespace sim
