#pragma once

#include <algorithm>
#include <cmath>

namespace sim {

/**
 * The clock.
 *
 * Real seconds for one full day, Rust's default hour, so a day is a proper
 * stretch of work and nightfall is something you plan around.
 */
inline constexpr double kDaySeconds = 3600;
/** How deep the dark gets, and how much of the cycle counts as night. */
inline constexpr double kNightDarkness = 0.86;
inline constexpr double kNightFraction = 0.3;
inline constexpr double kTwilight = 0.13;

/** Where in the day we are, nought to one. */
inline double dayFraction(double clock) {
    return std::fmod(clock, kDaySeconds) / kDaySeconds;
}

/** Nought at noon, one at midnight: the single source for everything hourly. */
inline double nightness(double clock) { return std::abs(dayFraction(clock) - 0.5) * 2; }

inline bool isNight(double clock) { return nightness(clock) > 1 - kNightFraction; }

/** Screen darkness: ramps in through twilight and holds through the night. */
inline double darkness(double clock) {
    const double threshold = 1 - kNightFraction - kTwilight;
    return std::clamp((nightness(clock) - threshold) / (kTwilight + 0.1), 0.0, 1.0) *
           kNightDarkness;
}

}  // namespace sim
