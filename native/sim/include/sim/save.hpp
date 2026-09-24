#pragma once

#include <string>

#include "sim/build.hpp"
#include "sim/craft.hpp"
#include "sim/inventory.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/** Everything a session is, apart from the island itself. */
struct Session {
    std::uint32_t seed = 0;
    double clock = 0;
    Player player;
    Inventory inventory;
};

/**
 * Saving and loading.
 *
 * The island is not written down: it is a seed, and the same generator builds
 * it again. What is written is everything that happened to it, which on a
 * quiet island is a few kilobytes rather than a few megabytes.
 *
 * Written as one binary blob with a version on the front, so an old save from
 * a different shape of the game is refused rather than misread.
 */
bool saveSession(const std::string& path, const Session& session, const World& world,
                 const BuildSystem& build);

/** Returns false when there is nothing to load, or it is of the wrong vintage. */
bool loadSession(const std::string& path, Session& session, World& world, BuildSystem& build);

}  // namespace sim
