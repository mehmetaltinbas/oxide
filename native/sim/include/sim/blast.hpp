#pragma once

#include <vector>

#include "sim/build.hpp"
#include "sim/npcs.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/**
 * How a blast that spreads falls off across what it reaches: full damage at
 * the middle, nothing at the edge, curved so the pieces right beside the hit
 * take most of it and the far side takes little.
 */
inline constexpr double kBlastFalloff = 1.5;

/** A charge that has been thrown and is counting down. */
struct Charge {
    int id;
    ItemId kind;
    double x;
    double y;
    double vx;
    double vy;
    double fuse;
    /** The piece it stuck to, which takes the whole charge rather than a share. */
    int stuckTo;
};

/** How hard a charge can be lobbed, and how gently it can be dropped. */
inline constexpr double kThrowMin = 180;
inline constexpr double kThrowMax = 520;
/** How fast a thrown charge sheds its speed once it is in the air. */
inline constexpr double kThrowDrag = 2.6;

/**
 * Everything that goes off.
 *
 * A charge placed on a wall breaks that wall; a rocket spreads, and takes the
 * trees, the ore, the barrels, the animals and anyone standing too close along
 * with whatever it hit. One routine either way, so a new explosive is a table
 * entry rather than a system.
 */
class Explosives {
public:
    /**
     * Thrown at a point: it flies, slows, and sticks to the first wall it
     * reaches, which is how a satchel ends up on somebody's door rather than
     * in the grass in front of it.
     */
    void throwAt(ItemId kind, double fromX, double fromY, double toX, double toY);
    /** Stuck straight onto a piece, the way a charge is placed by hand. */
    void place(ItemId kind, double x, double y, int stuckTo);

    const std::vector<Charge>& list() const { return charges_; }

    /** Counts the fuses down and sets off anything that has run out. */
    void update(World& world, BuildSystem& build, NpcSystem& npcs, Player& player,
                Inventory& inventory, double dt);

    /**
     * One blast, wherever it came from.
     *
     * `splash` is whether it damages everything built that it reaches rather
     * than only the piece it is stuck to: a rocket does, a charge does not.
     */
    void detonate(World& world, BuildSystem& build, NpcSystem& npcs, Player& player,
                  Inventory& inventory, double x, double y, double radius, double damage,
                  int stuckTo, bool splash);

private:
    std::vector<Charge> charges_;
    int nextId_ = 1;
};

}  // namespace sim
