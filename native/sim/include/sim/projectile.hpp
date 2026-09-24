#pragma once

#include <vector>

#include "sim/build.hpp"
#include "sim/npcs.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/** One round in flight. */
struct Bullet {
    int id;
    double x;
    double y;
    double vx;
    double vy;
    /** Where it came from, so a tracer can be drawn behind it. */
    double fromX;
    double fromY;
    /** How far it has left before it is spent: a bullet does not fly forever. */
    double left;
    double damage;
    /** Whose it is. Only a player's for now; the monuments come later. */
    bool fromPlayer;
    /** An arrow is drawn as a shaft, a bullet as a streak of light. */
    bool arrow;
};

/** What the rounds in the air did this tick. */
struct BulletHit {
    double x;
    double y;
    /** What it went into: an animal, something standing, or nothing at all. */
    bool npc;
    NpcKind npcKind;
    bool killed;
    bool node;
    NodeKind nodeKind;
    bool brokeNode;
    /** Or a wall, which takes the damage and may come down. */
    bool built;
    bool brokeBuilt;
    /** Or the player, when it was a monument's guard that fired it. */
    bool player;
    double damage;
};

/**
 * Everything in the air.
 *
 * A round is stepped along its line and whatever that line crosses first is
 * what it hits, so a fast bullet cannot skip through a trunk between two
 * frames the way a point test lets it.
 */
class Projectiles {
public:
    void spawn(double x, double y, double angle, const Gun& gun, double damage, bool arrow);
    /** One fired at the player rather than by them. */
    void spawnHostile(double x, double y, double angle, const Gun& gun, double damage);

    const std::vector<Bullet>& list() const { return bullets_; }

    std::vector<BulletHit> update(World& world, NpcSystem& npcs, BuildSystem& build, double dt,
                                  const Player* target);

    void clear() { bullets_.clear(); }

private:
    std::vector<Bullet> bullets_;
    int nextId_ = 1;
};

}  // namespace sim
