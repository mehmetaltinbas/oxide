#pragma once

#include "sim/item.hpp"

namespace sim {

/** A stack lying on the ground, waiting for whoever walks past. */
struct Dropped {
    int id;
    ItemStack stack;
    double x;
    double y;
    /** How long it has lain there, so it can rot away in time. */
    double age;
};

/** How long a dropped stack lasts before the island takes it back. */
inline constexpr double kDropLifetime = 300;

}  // namespace sim
