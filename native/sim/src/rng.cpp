#include "sim/rng.hpp"

namespace sim {

namespace {

/** JavaScript's Math.imul: the low 32 bits of the product, signed. */
inline std::uint32_t imul(std::uint32_t a, std::uint32_t b) {
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(a) * static_cast<std::int32_t>(b));
}

}  // namespace

double Rng::unit() {
    state_ += 0x6d2b79f5u;
    std::uint32_t t = state_;
    t = imul(t ^ (t >> 15), t | 1u);
    t ^= t + imul(t ^ (t >> 7), t | 61u);
    return static_cast<double>((t ^ (t >> 14))) / 4294967296.0;
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
