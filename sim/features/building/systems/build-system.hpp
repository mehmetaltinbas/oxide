#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/building/types/deployable.struct.hpp"
#include "sim/features/items/systems/inventory.hpp"
#include "sim/features/items/types/cost.struct.hpp"
#include "sim/features/items/types/melee.struct.hpp"
#include "sim/features/world/systems/world.hpp"

namespace sim {

/** Foundations sit on a cell; walls, doorways and doors on the edge between two. */
/**
 * Foundation and Ceiling sit on a cell; the rest sit on one of its edges.
 *
 * A ceiling is the floor of the storey above and the roof of the one below.
 * Without one a base is a set of pens: you can wall a room in and still be
 * looked into, and there is nothing to stand on to build higher.
 */
enum class BuildKind : std::uint8_t { Foundation, Wall, Doorway, Door, Ceiling };

inline constexpr int kBuildKindCount = 5;

/** Whether a piece belongs to a whole cell rather than to one of its edges. */
inline bool onCell(BuildKind kind) {
	return kind == BuildKind::Foundation || kind == BuildKind::Ceiling;
}

/** What a piece is made of, and therefore how much it takes to get through. */
enum class BuildTier : std::uint8_t { Twig, Wood, Stone, Metal };

inline constexpr int kBuildTierCount = 4;

struct TierDef {
	BuildTier tier;
	const char* name;
	int hp;
	Cost cost;
	/**
	 * How much of a swing gets through, before the soft-side rule.
	 *
	 * Twig is kindling: a hatchet takes it apart, which is the point of a
	 * tier you put up to claim ground and never keep. Wood gives way slowly.
	 * Stone and sheet metal barely notice a tool at all, so from stone
	 * upwards the only way in is explosives, which is what makes a stone base
	 * a decision rather than a delay.
	 */
	double meleeMul;
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
	/** Seconds since anything last took health off it: see kHealthShownFor. */
	double sinceHurt = 0;
	/**
	 * Seconds since it was put up.
	 *
	 * A piece you have just built can be taken back down with a hammer; an
	 * older one has to be broken. Rust's rule, and the reason a misplaced
	 * wall is a mistake rather than a rebuild: see kFreeDemolishSeconds.
	 */
	double age = 0;
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
	/** For the one caller that has an id and needs the piece it names. */
	std::vector<Structure>& mutableList() { return pieces_; }
	/** The same, to be changed: what a blast and a hammer work on. */
	std::vector<Structure>& list2() { return pieces_; }

	Structure* foundationAt(int gx, int gy);
	Structure* ceilingAt(int gx, int gy);
	const Structure* ceilingAt(int gx, int gy) const;
	Structure* edgeAt(int gx, int gy, EdgeSide side);
	const Structure* foundationAt(int gx, int gy) const;
	const Structure* edgeAt(int gx, int gy, EdgeSide side) const;

	/** Why a foundation may not go on this cell, or nothing if it may. */
	const char* refuseFoundation(const World& world, int gx, int gy, int owner) const;
	/** Why a ceiling may not go over this cell, or nothing if it may. */
	const char* refuseCeiling(const World& world, int gx, int gy, int owner) const;
	/** Why a wall or doorway may not go on this edge, or nothing if it may. */
	const char* refuseEdge(const World& world, int gx, int gy, EdgeSide side, BuildKind kind,
						   int owner) const;

	Structure& placeFoundation(int gx, int gy, int owner, BuildTier tier = BuildTier::Twig);
	Structure& placeCeiling(int gx, int gy, int owner, BuildTier tier = BuildTier::Twig);
	/**
	 * `fromX`/`fromY` is where the builder was standing, which becomes the
	 * piece's soft side, as in Rust: a wall is meant to be broken from outside.
	 */
	Structure& placeEdge(int gx, int gy, EdgeSide side, BuildKind kind, int owner,
						 BuildTier tier = BuildTier::Twig, double fromX = 0, double fromY = 0);

	/**
	 * How long after building you may still take a piece down by hand.
	 *
	 * Fifteen minutes. Long enough that a wall in the wrong place is a
	 * mistake you fix, short enough that it is no use to a raider and no use
	 * for moving a finished base about. Only your own, and nothing is given
	 * back: the time you get is the mercy, not the materials.
	 */
	static constexpr double kFreeDemolishSeconds = 900;

	/** Takes a piece back down. Says whether it was young enough and yours. */
	bool demolish(Structure& piece, int owner);

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
	/** Straight to a named tier, which is what the wheel asks for. Up only. */
	bool upgradeTo(Structure& piece, BuildTier tier, Inventory& inventory);

	/**
	 * A blow or a bullet on a piece. Says whether that was the end of it.
	 *
	 * A melee blow from the hard side of a wall barely scratches it, which is
	 * what stops anyone chopping their way out of a base from the inside.
	 */
	/**
	 * A place a building was blown out of, which nobody may build on yet.
	 *
	 * Thirty seconds. A raid is decided in the moment the wall comes down, and
	 * a defender who can drop a fresh one into the hole the same second has
	 * not defended anything: they have made the charge free. The gap has to
	 * stay a gap long enough to be walked through.
	 *
	 * Only a blast leaves one. Knocking down your own twig wall with a hammer
	 * and putting a better one up is the normal way to build.
	 */
	struct Scorch {
		int gx;
		int gy;
		EdgeSide side;
		/** Whether it was the floor rather than something on an edge of it. */
		bool foundation;
		double left;
	};

	const std::vector<Scorch>& scorches() const { return scorches_; }

	/** Whether that place was blown out recently enough to still be blocked. */
	bool scorchedAt(int gx, int gy, EdgeSide side, bool foundation) const;

	bool damage(Structure& piece, double amount, double fromX = 0, double fromY = 0,
				bool melee = false, bool blast = false);

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
	/** The nearest door or doorway, which is what E is asking about. */
	Structure* nearestDoor(double x, double y, double within);

	void update(double dt);

private:
	std::vector<Scorch> scorches_;
	std::unordered_map<std::uint64_t, int> byRoof_;
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
