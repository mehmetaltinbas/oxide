#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

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
    Structure& placeEdge(int gx, int gy, EdgeSide side, BuildKind kind, int owner,
                         BuildTier tier = BuildTier::Twig);

    /** The next tier up from what a piece is, if there is one. */
    bool nextTier(const Structure& piece, BuildTier& out) const;
    /** Puts a piece up a tier, if the pack holds what that costs. */
    bool upgrade(Structure& piece, Inventory& inventory);

    /** A blow or a bullet on a piece. Says whether that was the end of it. */
    bool damage(Structure& piece, double amount);

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
    int nextId_ = 1;
    /** Cell and edge lookups, rebuilt whenever a piece comes or goes. */
    std::unordered_map<std::uint64_t, int> byCell_;
    std::unordered_map<std::uint64_t, int> byEdge_;

    void reindex();
};

}  // namespace sim
