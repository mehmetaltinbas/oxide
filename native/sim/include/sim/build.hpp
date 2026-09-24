#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "sim/deployable.hpp"
#include "sim/inventory.hpp"
#include "sim/item.hpp"
#include "sim/world.hpp"

namespace sim {

/** Foundations sit on a cell; walls, doorways and doors on the edge between two. */
enum class BuildKind : std::uint8_t { Foundation, Wall, Doorway, Door };

/** What a piece is made of, and therefore how much it takes to get through. */
enum class BuildTier : std::uint8_t { Twig, Wood, Stone, Metal };

inline constexpr int kBuildTierCount = 4;

struct TierDef {
    BuildTier tier;
    const char* name;
    int hp;
    Cost cost;
};

const TierDef& tierDef(BuildTier tier);

/** Which edge of a cell a piece sits on. Every edge belongs to one cell. */
enum class EdgeSide : std::uint8_t { North, West };

/** Melee only bites on the soft side; explosives do not care. */
inline constexpr double kHardSideMeleeMul = 0.1;

/** A foundation is tougher than the walls on it; a door is a little softer. */
inline constexpr double kFoundationHpMul = 1.6;
inline constexpr double kDoorHpMul = 0.8;
/** How thick a wall stands, for walking into one. */
inline constexpr double kWallThickness = 9;

struct Structure {
    int id;
    BuildKind kind;
    BuildTier tier;
    int gx;
    int gy;
    EdgeSide side;
    int hp;
    int maxHp;
    /** Whose it is. Nought is the player at the keyboard, for now. */
    int owner;
    bool open;
    /** A locked door opens for its owner and for nobody else. */
    bool locked;
    /** Which way the builder was standing: the side a blow bites on. */
    double softX;
    double softY;
    double flash;
};

/** The world-space ends of an edge piece. */
void edgeSegment(int gx, int gy, EdgeSide side, double& x0, double& y0, double& x1, double& y1);

/**
 * What everybody has built.
 *
 * Foundations claim a cell of the build grid, walls and doorways the edges
 * between cells, which is what lets a base be laid out square without anyone
 * lining anything up by hand. A wall needs a foundation beside it, nothing is
 * built on top of a standing tree, and a piece is upgraded through twig, wood,
 * stone and sheet metal with a hammer.
 */
class BuildSystem {
public:
    const std::vector<Structure>& list() const { return pieces_; }
    /** The same, to be changed: what a blast and a hammer work on. */
    std::vector<Structure>& list2() { return pieces_; }

    Structure* foundationAt(int gx, int gy);
    Structure* edgeAt(int gx, int gy, EdgeSide side);
    const Structure* foundationAt(int gx, int gy) const;
    const Structure* edgeAt(int gx, int gy, EdgeSide side) const;

    /** Why a foundation may not go on this cell, or nothing if it may. */
    const char* refuseFoundation(const World& world, int gx, int gy, int owner) const;
    /** Why a wall or doorway may not go on this edge, or nothing if it may. */
    const char* refuseEdge(const World& world, int gx, int gy, EdgeSide side, BuildKind kind,
                           int owner) const;

    Structure& placeFoundation(int gx, int gy, int owner, BuildTier tier = BuildTier::Twig);
    /**
     * `fromX`/`fromY` is where the builder was standing, which becomes the
     * piece's soft side, as in Rust: a wall is meant to be broken from outside.
     */
    Structure& placeEdge(int gx, int gy, EdgeSide side, BuildKind kind, int owner,
                         BuildTier tier = BuildTier::Twig, double fromX = 0, double fromY = 0);

    /** The next tier up from what a piece is, if there is one. */
    bool nextTier(const Structure& piece, BuildTier& out) const;
    // ------------------------------------------------------- deployables

    const std::vector<Deployable>& deployables() const { return deployables_; }
    std::vector<Deployable>& deployables() { return deployables_; }

    Deployable* deployableAt(int gx, int gy);
    /** The one nearest a point, for a hand on a fire or a box. */
    Deployable* deployableNear(double x, double y, double within);

    /** Why one may not go on this cell, or nothing if it may. */
    const char* refuseDeploy(const World& world, int gx, int gy, DeployKind kind, int owner) const;
    /**
     * Puts one down and gives back its id.
     *
     * An id rather than a reference on purpose: the next thing put down may
     * move the whole list in memory, and a reference held across that is a
     * pointer into nothing.
     */
    int deploy(DeployKind kind, int gx, int gy, int owner);
    Deployable* deployableById(int id);
    void removeDeployable(int id);

    /**
     * Whose ground this is, by the nearest tool cupboard, or nobody's.
     *
     * A cupboard claims a circle around itself, and inside it only its owner
     * builds. It is the whole of why a base is yours rather than everybody's.
     */
    bool claimed(double x, double y, int owner) const;

    /**
     * Which sealed room a point is in, or nought for open ground.
     *
     * A room is a set of cells the open air cannot walk into: that is what
     * makes a base a base rather than a fence. Rebuilt whenever anything is
     * built, broken or swung open, because a hole anywhere opens all of it.
     */
    int regionAt(double x, double y) const;
    /** Whose room that is, or minus one when nobody owns the floor under it. */
    int regionOwner(int region) const;
    /** Every sealed cell and the room it belongs to, for putting a roof on. */
    const std::unordered_map<std::uint64_t, int>& enclosedCells() const;

    /**
     * Whether a point can be reached from another: not through a wall, and not
     * into a room somebody else has sealed.
     */
    bool canReach(double fromX, double fromY, double toX, double toY, int owner) const;

    /** The best workbench within reach of a point, and nought for none. */
    int benchTierAt(double x, double y, int owner) const;

    /** How warm any lit fire nearby makes a point. */
    double warmthAt(double x, double y) const;

    /**
     * What rots while nobody is here to keep it up.
     *
     * Rust's rule, kept simple: everything you own loses a slice of its full
     * health for every hour the island ran without you, and what that finishes
     * off is gone when you come back.
     */
    void applyDecay(double hours);

    /**
     * Fires burn, meat cooks, ore smelts, and anything that has been broken
     * spills what was inside it onto the ground.
     */
    void updateDeployables(World& world, double dt);

    /** Puts a piece up a tier, if the pack holds what that costs. */
    bool upgrade(Structure& piece, Inventory& inventory);

    /**
     * A blow or a bullet on a piece. Says whether that was the end of it.
     *
     * A melee blow from the hard side of a wall barely scratches it, which is
     * what stops anyone chopping their way out of a base from the inside.
     */
    bool damage(Structure& piece, double amount, double fromX = 0, double fromY = 0,
                bool melee = false);

    /** Pushes a point out of any wall it is inside. */
    void resolve(double& x, double& y, double radius) const;

    /**
     * The first solid piece a line crosses, and how far along it that happens.
     *
     * A bullet is stopped by a wall like anything else, and it is stopped
     * where the wall is rather than where the frame happened to end.
     */
    Structure* hitSegment(double ax, double ay, double bx, double by, double& at);

    /** The piece nearest a point, for a hammer or a hand on a door. */
    Structure* nearest(double x, double y, double within);

    void update(double dt);

private:
    std::vector<Structure> pieces_;
    std::vector<Deployable> deployables_;
    int nextId_ = 1;
    int nextDeployId_ = 1;
    /** Rolls the charcoal a furnace throws off. */
    mutable std::uint32_t smeltRolls_ = 1;
    /** Cell and edge lookups, rebuilt whenever a piece comes or goes. */
    std::unordered_map<std::uint64_t, int> byCell_;
    std::unordered_map<std::uint64_t, int> byEdge_;

    void reindex();
    /** Works out what is sealed off from the open air. */
    void rebuildEnclosure() const;

    mutable std::unordered_map<std::uint64_t, int> enclosure_;
    mutable std::unordered_map<int, int> enclosureOwner_;
    mutable bool enclosureDirty_ = true;
    /** Which doors were open last tick, so swinging one re-floods the base. */
    std::unordered_map<int, bool> wasOpen_;
};

}  // namespace sim
