#pragma once

#include <vector>

#include "sim/npc.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/** What the animals did to you this tick, for the client to draw and sound. */
struct NpcEvents {
    /** Damage that landed on the player, and where it came from. */
    double playerDamage = 0;
    double fromX = 0;
    double fromY = 0;
    /** An animal that died this tick, for the sound and the popup. */
    bool killed = false;
    NpcKind killedKind = NpcKind::Boar;
    double killedX = 0;
    double killedY = 0;
};

/**
 * The wildlife.
 *
 * Boars keep to themselves until hurt, wolves and bears do not. All of them
 * stay near where they were born, give up on anyone who swims away, and leave
 * what they are worth on the ground when they fall.
 */
class NpcSystem {
public:
    /** Fills the island: boars, wolves and bears, away from road and water. */
    void populate(const World& world, std::uint32_t seed);

    const std::vector<Npc>& list() const { return npcs_; }

    /** Everything alive in a rectangle, appended to `out`. */
    void inRect(double x0, double y0, double x1, double y1, std::vector<const Npc*>& out) const;

    NpcEvents update(World& world, double dt, const Player& player);

    /**
     * A blow or a bullet landing.
     *
     * It takes the world because the thing may die of it, and what it was
     * worth falls where it stood at that moment rather than a tick later:
     * whoever killed it should be able to pick it up straight away.
     */
    void hurt(World& world, Npc& npc, double amount, double fromX, double fromY);

    /** The nearest one in front of a player, for a swing to land on. */
    Npc* nearest(double x, double y, double within);

private:
    std::vector<Npc> npcs_;
    int nextId_ = 1;
    std::uint32_t rolls_ = 1;

    void drop(World& world, const Npc& npc);
};

}  // namespace sim
