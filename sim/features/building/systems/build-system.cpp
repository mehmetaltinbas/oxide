#include "sim/features/building/systems/build-system.hpp"

#include "sim/features/survival/systems/survival.hpp"
#include "sim/features/building/constants/deployable-defs.constant.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/building/types/deployable.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"
#include "sim/features/monuments/constants/monument-placement.constant.hpp"
#include "sim/features/monuments/types/monument.struct.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"
#include "sim/shared/utils/health.util.hpp"
#include "sim/features/building/constants/deploy-footprint.constant.hpp"

#include <algorithm>
#include <cmath>

namespace sim {

namespace {

/** How long a blown-out place stays unbuildable: see BuildSystem::Scorch. */
constexpr double kRebuildBlock = 30;

constexpr TierDef kTiers[kBuildTierCount] = {
	{BuildTier::Twig, "Twig", 10, {ItemId::Wood, 10}, 1.0},
	{BuildTier::Wood, "Wood", 250, {ItemId::Wood, 50}, 0.45},
	{BuildTier::Stone, "Stone", 500, {ItemId::Stone, 150}, 0.06},
	{BuildTier::Metal, "Metal", 1000, {ItemId::Metal, 200}, 0.03},
};

std::uint64_t cellKey(int gx, int gy) {
	return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(gx)) << 32) |
		   static_cast<std::uint32_t>(gy);
}

std::uint64_t edgeKey(int gx, int gy, EdgeSide side) {
	return (cellKey(gx, gy) << 1) | static_cast<std::uint64_t>(side == EdgeSide::West ? 1 : 0);
}

/** The point on a segment nearest another point. */
void closestOnSegment(double px, double py, double ax, double ay, double bx, double by, double& ox,
					  double& oy) {
	const double dx = bx - ax;
	const double dy = by - ay;
	const double len2 = dx * dx + dy * dy;
	double t = len2 > 0 ? ((px - ax) * dx + (py - ay) * dy) / len2 : 0;
	t = std::clamp(t, 0.0, 1.0);
	ox = ax + dx * t;
	oy = ay + dy * t;
}

/** The two cells an edge separates. */
void edgeCells(int gx, int gy, EdgeSide side, int& ax, int& ay, int& bx, int& by) {
	if (side == EdgeSide::North) {
		ax = gx;
		ay = gy - 1;
	} else {
		ax = gx - 1;
		ay = gy;
	}
	bx = gx;
	by = gy;
}

/** Whether a spot belongs to a monument, and so to everybody. */
bool onMonumentGround(const World& world, double x, double y) {
	for (const Monument& m : world.monuments()) {
		if (std::hypot(x - m.x, y - m.y) < m.radius + kMonumentNoBuildMargin) return true;
	}
	return false;
}

/** Whether anything is still standing where something is about to be built. */
bool naturalCover(const World& world, double x, double y, double radius) {
	static thread_local std::vector<const ResourceNode*> near;
	world.nodesInRect(x - radius - 40, y - radius - 40, x + radius + 40, y + radius + 40, near);
	for (const ResourceNode* node : near) {
		if (node->hp <= 0) continue;
		if (std::hypot(node->x - x, node->y - y) < radius + node->radius * 0.6) return true;
	}
	return false;
}

}  // namespace

const TierDef& tierDef(BuildTier tier) { return kTiers[static_cast<int>(tier)]; }

void edgeSegment(int gx, int gy, EdgeSide side, double& x0, double& y0, double& x1, double& y1) {
	const double x = gx * static_cast<double>(kBuildCell);
	const double y = gy * static_cast<double>(kBuildCell);
	x0 = x;
	y0 = y;
	x1 = side == EdgeSide::North ? x + kBuildCell : x;
	y1 = side == EdgeSide::North ? y : y + kBuildCell;
}

Structure* BuildSystem::foundationAt(int gx, int gy) {
	const auto it = byCell_.find(cellKey(gx, gy));
	return it == byCell_.end() ? nullptr : &pieces_[it->second];
}

const Structure* BuildSystem::foundationAt(int gx, int gy) const {
	const auto it = byCell_.find(cellKey(gx, gy));
	return it == byCell_.end() ? nullptr : &pieces_[it->second];
}

Structure* BuildSystem::edgeAt(int gx, int gy, EdgeSide side) {
	const auto it = byEdge_.find(edgeKey(gx, gy, side));
	return it == byEdge_.end() ? nullptr : &pieces_[it->second];
}

const Structure* BuildSystem::edgeAt(int gx, int gy, EdgeSide side) const {
	const auto it = byEdge_.find(edgeKey(gx, gy, side));
	return it == byEdge_.end() ? nullptr : &pieces_[it->second];
}

bool BuildSystem::scorchedAt(int gx, int gy, EdgeSide side, bool foundation) const {
	for (const Scorch& mark : scorches_) {
		if (mark.gx != gx || mark.gy != gy) continue;
		if (mark.foundation != foundation) continue;
		if (!foundation && mark.side != side) continue;
		return true;
	}
	return false;
}

const char* BuildSystem::refuseFoundation(const World& world, int gx, int gy, int) const {
	if (scorchedAt(gx, gy, EdgeSide::North, true)) return "blown out";
	const double cx = (gx + 0.5) * kBuildCell;
	const double cy = (gy + 0.5) * kBuildCell;
	if (cx < kBuildCell || cy < kBuildCell || cx > kWorldWidth - kBuildCell ||
		cy > kWorldHeight - kBuildCell) {
		return "Outside the map";
	}
	if (foundationAt(gx, gy)) return "Already a foundation here";
	if (onMonumentGround(world, cx, cy)) return "You cannot build at a monument";
	// Nothing is built on water, or on the road everybody uses.
	const Biome biome = world.biomeAt(cx, cy);
	if (biome == Biome::Water) return "You cannot build on water";
	if (biome == Biome::Road) return "You cannot build on the road";
	if (naturalCover(world, cx, cy, kBuildCell * 0.5)) return "Something is in the way";
	return nullptr;
}

const char* BuildSystem::refuseEdge(const World& world, int gx, int gy, EdgeSide side,
									BuildKind kind, int owner) const {
	if (scorchedAt(gx, gy, side, false)) return "blown out";
	double x0 = 0;
	double y0 = 0;
	double x1 = 0;
	double y1 = 0;
	edgeSegment(gx, gy, side, x0, y0, x1, y1);
	const double cx = (x0 + x1) * 0.5;
	const double cy = (y0 + y1) * 0.5;

	const Structure* existing = edgeAt(gx, gy, side);
	if (kind == BuildKind::Door) {
		// A door goes into a doorway you already put up, and nowhere else.
		if (!existing) return "Doors go in a doorway";
		if (existing->kind != BuildKind::Doorway) return "That is not a doorway";
		if (existing->owner != owner) return "Not your doorway";
		return nullptr;
	}
	if (existing) return "Already something on this edge";

	// Walls need something to stand on: one of the two cells the edge divides
	// must already be your foundation.
	int ax = 0;
	int ay = 0;
	int bx = 0;
	int by = 0;
	edgeCells(gx, gy, side, ax, ay, bx, by);
	const Structure* a = foundationAt(ax, ay);
	const Structure* b = foundationAt(bx, by);
	const bool supported = (a && a->owner == owner) || (b && b->owner == owner);
	if (!supported) return "Needs a foundation beside it";
	if (onMonumentGround(world, cx, cy)) return "You cannot build at a monument";
	if (naturalCover(world, cx, cy, kBuildCell * 0.25)) return "Something is in the way";
	return nullptr;
}

Structure* BuildSystem::ceilingAt(int gx, int gy) {
	const auto it = byRoof_.find(cellKey(gx, gy));
	return it == byRoof_.end() ? nullptr : &pieces_[it->second];
}

const Structure* BuildSystem::ceilingAt(int gx, int gy) const {
	const auto it = byRoof_.find(cellKey(gx, gy));
	return it == byRoof_.end() ? nullptr : &pieces_[it->second];
}

const char* BuildSystem::refuseCeiling(const World& world, int gx, int gy, int owner) const {
	if (ceilingAt(gx, gy)) return "Already a ceiling here";
	if (scorchedAt(gx, gy, EdgeSide::North, true)) return "blown out";
	const double cx = (gx + 0.5) * kBuildCell;
	const double cy = (gy + 0.5) * kBuildCell;
	if (onMonumentGround(world, cx, cy)) return "You cannot build at a monument";
	// A ceiling needs something holding it up: a wall on one of the cell's own
	// edges, or a ceiling already beside it to carry on from. Rust's rule, and
	// the reason a roof cannot be conjured over open ground.
	const bool walled = edgeAt(gx, gy, EdgeSide::North) || edgeAt(gx, gy, EdgeSide::West) ||
						edgeAt(gx, gy + 1, EdgeSide::North) || edgeAt(gx + 1, gy, EdgeSide::West);
	const bool joined = ceilingAt(gx - 1, gy) || ceilingAt(gx + 1, gy) || ceilingAt(gx, gy - 1) ||
						ceilingAt(gx, gy + 1);
	if (!walled && !joined) return "Needs a wall under it";
	const Structure* floor = foundationAt(gx, gy);
	if (floor && floor->owner != owner) return "Not your building";
	return nullptr;
}

Structure& BuildSystem::placeCeiling(int gx, int gy, int owner, BuildTier tier) {
	// The same weight as a floor: a roof is what a raid comes through when the
	// walls hold, so it cannot be the cheap way in.
	const int hp = static_cast<int>(std::lround(tierDef(tier).hp * kFoundationHpMul));
	pieces_.push_back(Structure{nextId_++, BuildKind::Ceiling, tier, gx, gy, EdgeSide::North, hp,
								hp, owner, false, false, 0, 0, 0});
	reindex();
	return pieces_.back();
}

void BuildSystem::raiseMonumentRoom(int gx, int gy, int wide, int deep, bool roofed,
								   int doorSide, int doorAt) {
	// The floor, and the roof over it if it has one.
	for (int dy = 0; dy < deep; ++dy) {
		for (int dx = 0; dx < wide; ++dx) {
			placeFoundation(gx + dx, gy + dy, kIslandOwner, BuildTier::Metal);
			if (roofed) placeCeiling(gx + dx, gy + dy, kIslandOwner, BuildTier::Metal);
		}
	}
	// The wall round it. A cell owns its north and its west edge, so the south
	// and east walls belong to the cells just outside the room.
	const auto wall = [&](int wx, int wy, EdgeSide side, bool door) {
		if (edgeAt(wx, wy, side)) return;
		Structure& piece = placeEdge(wx, wy, side, door ? BuildKind::Doorway : BuildKind::Wall,
									 kIslandOwner, BuildTier::Metal);
		piece.open = door;
	};
	for (int dx = 0; dx < wide; ++dx) {
		wall(gx + dx, gy, EdgeSide::North, doorSide == 0 && dx == doorAt);
		wall(gx + dx, gy + deep, EdgeSide::North, doorSide == 1 && dx == doorAt);
	}
	for (int dy = 0; dy < deep; ++dy) {
		wall(gx, gy + dy, EdgeSide::West, doorSide == 2 && dy == doorAt);
		wall(gx + wide, gy + dy, EdgeSide::West, doorSide == 3 && dy == doorAt);
	}
}

bool BuildSystem::demolish(Structure& piece, int owner) {
	if (piece.owner != owner) return false;
	if (piece.age > kFreeDemolishSeconds) return false;
	const int id = piece.id;
	pieces_.erase(std::remove_if(pieces_.begin(), pieces_.end(),
								 [id](const Structure& s) { return s.id == id; }),
				  pieces_.end());
	reindex();
	return true;
}

Structure& BuildSystem::placeFoundation(int gx, int gy, int owner, BuildTier tier) {
	const int hp = static_cast<int>(std::lround(tierDef(tier).hp * kFoundationHpMul));
	pieces_.push_back(Structure{nextId_++, BuildKind::Foundation, tier, gx, gy, EdgeSide::North, hp,
								hp, owner, false, false, 0, 0, 0});
	reindex();
	return pieces_.back();
}

Structure& BuildSystem::placeEdge(int gx, int gy, EdgeSide side, BuildKind kind, int owner,
								  BuildTier tier, double fromX, double fromY) {
	const int hp = tierDef(tier).hp;
	double x0 = 0;
	double y0 = 0;
	double x1 = 0;
	double y1 = 0;
	edgeSegment(gx, gy, side, x0, y0, x1, y1);
	const double dx = fromX - (x0 + x1) * 0.5;
	const double dy = fromY - (y0 + y1) * 0.5;
	const double len = std::hypot(dx, dy);
	const double softX = len > 0.001 ? dx / len : 0;
	const double softY = len > 0.001 ? dy / len : 0;
	pieces_.push_back(Structure{nextId_++, kind, tier, gx, gy, side, hp, hp, owner, false, false,
								softX, softY, 0});
	reindex();
	return pieces_.back();
}

bool BuildSystem::nextTier(const Structure& piece, BuildTier& out) const {
	if (piece.tier == BuildTier::Metal) return false;
	out = static_cast<BuildTier>(static_cast<int>(piece.tier) + 1);
	return true;
}

bool BuildSystem::upgradeTo(Structure& piece, BuildTier tier, Inventory& inventory) {
	// Upwards only. Taking a wall back down to twig to get the stone out of it
	// would make every base a bank, and there is nothing to refund from: what
	// went into it is in it.
	if (static_cast<int>(tier) <= static_cast<int>(piece.tier)) return false;
	const TierDef& def = tierDef(tier);
	if (inventory.count(def.cost.id) < def.cost.count) return false;
	inventory.take(def.cost.id, def.cost.count);
	piece.tier = tier;
	const double mul = piece.kind == BuildKind::Foundation || piece.kind == BuildKind::Ceiling
						   ? kFoundationHpMul
					   : piece.kind == BuildKind::Door ? kDoorHpMul
													   : 1.0;
	piece.maxHp = static_cast<int>(std::lround(def.hp * mul));
	piece.hp = piece.maxHp;
	return true;
}

bool BuildSystem::upgrade(Structure& piece, Inventory& inventory) {
	BuildTier tier = BuildTier::Twig;
	if (!nextTier(piece, tier)) return false;
	return upgradeTo(piece, tier, inventory);
}

bool BuildSystem::damage(Structure& piece, double amount, double fromX, double fromY, bool melee,
						 bool blast) {
	if (piece.hp <= 0) return false;
	// The island's own building is not a thing you get through: a monument
	// that could be opened with enough charges would be opened once and never
	// be a monument again.
	if (piece.owner == kIslandOwner) return false;
	// Anything short of a blast only marks twig: see breaksByHand.
	if (!blast && !breaksByHand(piece.tier)) return false;
	if (melee) amount *= tierDef(piece.tier).meleeMul;
	if (melee && piece.kind != BuildKind::Foundation) {
		// Struck from the hard side, a wall barely notices: that is what stops
		// anyone chopping their way out of a base from the inside.
		const double dx = fromX - (piece.gx + 0.5) * kBuildCell;
		const double dy = fromY - (piece.gy + 0.5) * kBuildCell;
		const double len = std::hypot(dx, dy);
		if (len > 0.001) {
			const double dot = (dx / len) * piece.softX + (dy / len) * piece.softY;
			if (dot < 0.15) amount *= kHardSideMeleeMul;
		}
	}
	piece.hp -= static_cast<int>(amount);
	tookDamage(piece);
	piece.flash = 0.12;
	if (piece.hp > 0) return false;
	piece.hp = 0;
	// Blown out, the place it stood stays open for a while: see Scorch.
	if (blast) {
		scorches_.push_back(Scorch{piece.gx, piece.gy, piece.side,
								   piece.kind == BuildKind::Foundation, kRebuildBlock});
	}
	// The piece is gone; what stood on it is left standing, as Rust leaves it.
	const int id = piece.id;
	pieces_.erase(std::remove_if(pieces_.begin(), pieces_.end(),
								 [id](const Structure& s) { return s.id == id; }),
				  pieces_.end());
	reindex();
	return true;
}

void BuildSystem::resolve(double& x, double& y, double radius) const {
	const int g0x = static_cast<int>(std::floor((x - radius - kBuildCell) / kBuildCell));
	const int g1x = static_cast<int>(std::floor((x + radius + kBuildCell) / kBuildCell));
	const int g0y = static_cast<int>(std::floor((y - radius - kBuildCell) / kBuildCell));
	const int g1y = static_cast<int>(std::floor((y + radius + kBuildCell) / kBuildCell));
	for (int gy = g0y; gy <= g1y; ++gy) {
		for (int gx = g0x; gx <= g1x; ++gx) {
			for (const EdgeSide side : {EdgeSide::North, EdgeSide::West}) {
				const Structure* piece = edgeAt(gx, gy, side);
				if (!piece) continue;
				// A doorway is a hole in a wall, and an open door is a hole too.
				if (piece->kind == BuildKind::Doorway) continue;
				if (piece->kind == BuildKind::Door && piece->open) continue;
				double x0 = 0;
				double y0 = 0;
				double x1 = 0;
				double y1 = 0;
				edgeSegment(gx, gy, side, x0, y0, x1, y1);
				double hx = 0;
				double hy = 0;
				closestOnSegment(x, y, x0, y0, x1, y1, hx, hy);
				const double d = std::hypot(x - hx, y - hy);
				const double min = radius + kWallHalf;
				if (d >= min || d <= 0.0001) continue;
				const double push = (min - d) / d;
				x += (x - hx) * push;
				y += (y - hy) * push;
			}
			for (const Deployable& d : deployables_) {
				if (d.kind == DeployKind::SleepingBag) continue;
				if (static_cast<int>(d.x / kBuildCell) != gx) continue;
				if (static_cast<int>(d.y / kBuildCell) != gy) continue;
				double halfWide = 0;
				double halfDeep = 0;
				deployBounds(d.kind, d.turned, d.x, d.y, halfWide, halfDeep);
				const double nx = std::clamp(x, d.x - halfWide, d.x + halfWide);
				const double ny = std::clamp(y, d.y - halfDeep, d.y + halfDeep);
				const double dd = std::hypot(x - nx, y - ny);
				if (dd >= radius || dd <= 0.0001) continue;
				const double push = (radius - dd) / dd;
				x += (x - nx) * push;
				y += (y - ny) * push;
			}
		}
	}
}

Structure* BuildSystem::hitSegment(double ax, double ay, double bx, double by, double& at) {
	Structure* best = nullptr;
	double bestAt = 2;
	for (Structure& piece : pieces_) {
		if (piece.kind == BuildKind::Foundation) continue;
		if (piece.kind == BuildKind::Doorway) continue;
		if (piece.kind == BuildKind::Door && piece.open) continue;
		double x0 = 0;
		double y0 = 0;
		double x1 = 0;
		double y1 = 0;
		edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
		// Where the two segments cross, if they do.
		const double r1 = bx - ax;
		const double r2 = by - ay;
		const double s1 = x1 - x0;
		const double s2 = y1 - y0;
		const double denom = r1 * s2 - r2 * s1;
		if (std::abs(denom) < 1e-9) continue;
		const double t = ((x0 - ax) * s2 - (y0 - ay) * s1) / denom;
		const double u = ((x0 - ax) * r2 - (y0 - ay) * r1) / denom;
		if (t < 0 || t > 1 || u < 0 || u > 1) continue;
		if (t >= bestAt) continue;
		bestAt = t;
		best = &piece;
	}
	at = bestAt;
	return best;
}

Structure* BuildSystem::nearestDoor(double x, double y, double within) {
	// Not `nearest`: standing in a doorway, the nearest piece of building is
	// the floor under your feet, which is why pressing E on a door did
	// nothing at all.
	Structure* best = nullptr;
	double bestD = within;
	for (Structure& piece : pieces_) {
		if (piece.kind != BuildKind::Door && piece.kind != BuildKind::Doorway) continue;
		double x0 = 0;
		double y0 = 0;
		double x1 = 0;
		double y1 = 0;
		edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
		double px = 0;
		double py = 0;
		closestOnSegment(x, y, x0, y0, x1, y1, px, py);
		const double d = std::hypot(px - x, py - y);
		if (d > bestD) continue;
		best = &piece;
		bestD = d;
	}
	return best;
}

Structure* BuildSystem::nearest(double x, double y, double within, bool underRoof) {
	// Edges first, and only then the floor and the roof.
	//
	// A cell piece was measured from the middle of its cell, so standing in a
	// base the floor under you was nearer the cursor than the wall you were
	// pointing at, and a hammer aimed at the north wall kept finding the
	// ground. What you are aiming at is the thing with an edge; the cell is
	// what you get when you are aiming at nothing.
	Structure* best = nullptr;
	double bestD = within;
	for (Structure& piece : pieces_) {
		if (onCell(piece.kind)) continue;
		double x0 = 0;
		double y0 = 0;
		double x1 = 0;
		double y1 = 0;
		edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
		double px = 0;
		double py = 0;
		closestOnSegment(x, y, x0, y0, x1, y1, px, py);
		const double d = std::hypot(px - x, py - y);
		if (d > bestD) continue;
		best = &piece;
		bestD = d;
	}
	if (best) return best;

	for (Structure& piece : pieces_) {
		if (!onCell(piece.kind)) continue;
		// Inside, the roof is over your head and out of reach; outside, the
		// floor is under the roof and out of sight.
		if (underRoof && piece.kind == BuildKind::Ceiling) continue;
		if (!underRoof && piece.kind == BuildKind::Foundation && ceilingAt(piece.gx, piece.gy)) {
			continue;
		}
		// To the cell itself rather than to its middle: standing on a floor
		// you are on it, whichever corner of it you happen to be in.
		const double x0 = piece.gx * static_cast<double>(kBuildCell);
		const double y0 = piece.gy * static_cast<double>(kBuildCell);
		const double px = std::clamp(x, x0, x0 + kBuildCell);
		const double py = std::clamp(y, y0, y0 + kBuildCell);
		const double d = std::hypot(x - px, y - py);
		if (d > bestD) continue;
		best = &piece;
		bestD = d;
	}
	return best;
}

Deployable* BuildSystem::deployableAt(int gx, int gy) {
	for (Deployable& d : deployables_) {
		if (static_cast<int>(d.x / kBuildCell) == gx && static_cast<int>(d.y / kBuildCell) == gy) {
			return &d;
		}
	}
	return nullptr;
}

Deployable* BuildSystem::deployableNear(double x, double y, double within) {
	Deployable* best = nullptr;
	double bestD = within;
	for (Deployable& d : deployables_) {
		const double dd = std::hypot(d.x - x, d.y - y);
		if (dd > bestD) continue;
		best = &d;
		bestD = dd;
	}
	return best;
}

bool BuildSystem::claimed(double x, double y, int owner) const {
	const Deployable* best = nullptr;
	double bestD = kToolCupboardRadius;
	for (const Deployable& d : deployables_) {
		if (d.kind != DeployKind::ToolCupboard) continue;
		const double dd = std::hypot(d.x - x, d.y - y);
		if (dd >= bestD) continue;
		best = &d;
		bestD = dd;
	}
	return best != nullptr && best->owner != owner;
}

void BuildSystem::deployBounds(DeployKind kind, bool turned, double x, double y,
							   double& halfWide, double& halfDeep) {
	const DeployFootprint f = deployFootprint(kind, turned);
	halfWide = f.wide * kDeployCell * 0.5;
	halfDeep = f.deep * kDeployCell * 0.5;
	(void)x;
	(void)y;
}

void BuildSystem::snapDeploy(DeployKind kind, bool turned, double& x, double& y) {
	// The middle of a thing sits on a fine-grid line when it is an even number
	// of squares across, and in the middle of one when it is odd. Snapping
	// both the same way put odd things half a square off their own floor.
	const DeployFootprint f = deployFootprint(kind, turned);
	// A fine square is centred on a building-grid line, so a thing an odd
	// number of squares across has its middle on one of those lines and an
	// even one has its middle half a square off. See kDeployOrigin.
	const auto lay = [](double v, int span) {
		const double off = (span % 2 == 0) ? kDeployCell * 0.5 : 0.0;
		return std::floor((v - off) / kDeployCell + 0.5) * kDeployCell + off;
	};
	x = lay(x, f.wide);
	y = lay(y, f.deep);
}

const char* BuildSystem::refuseDeploy(const World& world, double x, double y, DeployKind kind,
									  bool turned, int owner) const {
	const double cx = x;
	const double cy = y;
	double halfWide = 0;
	double halfDeep = 0;
	deployBounds(kind, turned, cx, cy, halfWide, halfDeep);
	// Nothing overlaps anything: two boxes may share a cell, and a furnace may
	// not sit on top of a campfire.
	for (const Deployable& d : deployables_) {
		double otherWide = 0;
		double otherDeep = 0;
		deployBounds(d.kind, d.turned, d.x, d.y, otherWide, otherDeep);
		if (std::abs(d.x - cx) < halfWide + otherWide &&
			std::abs(d.y - cy) < halfDeep + otherDeep) {
			return "Something is already here";
		}
	}
	// Nothing goes in a wall. A wall takes up room, the same room it stops you
	// walking through, so a box half inside one is a box you cannot reach and
	// a wall you cannot repair. Checked against the box, not against the
	// middle of the thing: a furnace's corner counts.
	for (const Structure& piece : pieces_) {
		if (onCell(piece.kind)) continue;
		double x0 = 0;
		double y0 = 0;
		double x1 = 0;
		double y1 = 0;
		edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
		// The nearest point of the wall's line to the box, and then whether
		// that point is inside the box grown by half the wall.
		const double px = std::clamp((x0 + x1) * 0.5, cx - halfWide, cx + halfWide);
		const double py = std::clamp((y0 + y1) * 0.5, cy - halfDeep, cy + halfDeep);
		double hx = 0;
		double hy = 0;
		closestOnSegment(px, py, x0, y0, x1, y1, hx, hy);
		const double nx = std::clamp(hx, cx - halfWide, cx + halfWide);
		const double ny = std::clamp(hy, cy - halfDeep, cy + halfDeep);
		if (std::hypot(hx - nx, hy - ny) < kWallHalf) return "There is a wall there";
	}
	if (world.biomeAt(cx, cy) == Biome::Water) return "Not in the water";
	if (kind == DeployKind::SleepingBag) {
		// Anyone's bag, not only yours: a spot has room for one.
		for (const Deployable& d : deployables_) {
			if (d.kind != DeployKind::SleepingBag) continue;
			if (std::hypot(d.x - cx, d.y - cy) < kSleepingBagSpacing) {
				return "Too close to another sleeping bag";
			}
		}
	}
	if (onMonumentGround(world, cx, cy)) return "You cannot build at a monument";
	if (claimed(cx, cy, owner)) return "Blocked by a tool cupboard";
	if (naturalCover(world, cx, cy, std::max(halfWide, halfDeep))) return "Something is in the way";
	return nullptr;
}

Deployable* BuildSystem::deployableById(int id) {
	for (Deployable& d : deployables_) {
		if (d.id == id) return &d;
	}
	return nullptr;
}

int BuildSystem::deploy(DeployKind kind, double x, double y, bool turned, int owner) {
	Deployable d{};
	d.id = nextDeployId_++;
	d.kind = kind;
	d.turned = turned;
	snapDeploy(kind, turned, x, y);
	d.x = x;
	d.y = y;
	d.hp = kDeployableHp;
	d.maxHp = kDeployableHp;
	d.owner = owner;
	d.container.slots.assign(containerSlots(kind), ItemStack{});
	deployables_.push_back(d);
	return d.id;
}

void BuildSystem::removeDeployable(int id) {
	deployables_.erase(std::remove_if(deployables_.begin(), deployables_.end(),
									  [id](const Deployable& d) { return d.id == id; }),
					   deployables_.end());
}

bool BuildSystem::canReach(double fromX, double fromY, double toX, double toY, int owner) const {
	double at = 0;
	if (const_cast<BuildSystem*>(this)->hitSegment(fromX, fromY, toX, toY, at)) return false;
	const int region = regionAt(toX, toY);
	if (region == 0) return true;
	if (regionAt(fromX, fromY) == region) return true;
	return regionOwner(region) == owner;
}

int BuildSystem::benchTierAt(double x, double y, int owner) const {
	int best = 0;
	for (const Deployable& d : deployables_) {
		if (d.owner != owner) continue;
		if (std::hypot(d.x - x, d.y - y) > kBenchReach) continue;
		best = std::max(best, benchTier(d.kind));
	}
	return best;
}

double BuildSystem::warmthAt(double x, double y) const {
	double warmth = 0;
	for (const Deployable& d : deployables_) {
		if (!d.lit) continue;
		const double dd = std::hypot(d.x - x, d.y - y);
		if (dd < kFireWarmthRadius) warmth += kFireWarmth * (1 - dd / kFireWarmthRadius);
	}
	return warmth;
}

const Deployable* BuildSystem::fireAt(double x, double y) const {
	const Deployable* best = nullptr;
	double bestD = kFireWarmthRadius;
	for (const Deployable& d : deployables_) {
		if (!d.lit) continue;
		const double dd = std::hypot(d.x - x, d.y - y);
		if (dd >= bestD) continue;
		best = &d;
		bestD = dd;
	}
	return best;
}

void BuildSystem::updateDeployables(World& world, double dt) {
	for (Deployable& thing : deployables_) ageWound(thing, dt);
	// What has been broken spills out where it stood, box and fire alike.
	std::vector<int> broken;
	for (const Deployable& d : deployables_) {
		if (d.hp <= 0) broken.push_back(d.id);
	}
	for (const int id : broken) {
		Deployable* d = deployableById(id);
		if (!d) continue;
		for (const ItemStack& stack : d->container.slots) {
			if (stack.id != ItemId::None) world.dropStack(stack, d->x, d->y);
		}
		// And the thing itself, for whoever knocked it down.
		world.dropStack(ItemStack{itemOf(d->kind), 1}, d->x, d->y);
		removeDeployable(id);
	}

	for (Deployable& d : deployables_) {
		if (d.flash > 0) d.flash -= dt;
		if (!d.lit || d.container.slots.empty()) continue;

		// Wood first: a fire that runs out of it goes out.
		if (d.fuel <= 0) {
			if (d.container.take(ItemId::Wood, 1) > 0) {
				d.fuel = kWoodBurnSeconds;
			} else {
				d.lit = false;
				continue;
			}
		}
		d.fuel -= dt;
		d.progress += dt;

		if (d.kind == DeployKind::Furnace && d.progress >= kSmeltSeconds) {
			d.progress = 0;
			if (d.container.take(ItemId::MetalOre, 1) > 0) {
				d.container.add(ItemId::Metal, 1);
			} else if (d.container.take(ItemId::SulfurOre, 1) > 0) {
				d.container.add(ItemId::Sulfur, 1);
			} else if (d.container.take(ItemId::HqMetalOre, 1) > 0) {
				d.container.add(ItemId::HqMetal, 1);
			}
			Rng rng(smeltRolls_ += 0x9e3779b9u);
			if (rng.unit() < kCharcoalChance) d.container.add(ItemId::Charcoal, 1);
		}
		if (d.kind == DeployKind::Campfire && d.progress >= kCookSeconds) {
			d.progress = 0;
			if (d.container.take(ItemId::MeatRaw, 1) > 0) d.container.add(ItemId::MeatCooked, 1);
		}
	}
}

void BuildSystem::applyDecay(double hours) {
	if (hours <= 0) return;
	// Five percent of a piece's full health an hour, as the other game had it.
	constexpr double kDecayPerHour = 0.05;
	const double loss = kDecayPerHour * hours;
	for (Structure& piece : pieces_) {
		piece.hp -= static_cast<int>(piece.maxHp * loss);
		tookDamage(piece);
	}
	pieces_.erase(std::remove_if(pieces_.begin(), pieces_.end(),
								 [](const Structure& piece) { return piece.hp <= 0; }),
				  pieces_.end());
	reindex();
}

void BuildSystem::update(double dt) {
	for (std::size_t i = scorches_.size(); i-- > 0;) {
		scorches_[i].left -= dt;
		if (scorches_[i].left <= 0) scorches_.erase(scorches_.begin() + static_cast<long>(i));
	}
	for (Structure& piece : pieces_) {
		piece.age += dt;
		ageWound(piece, dt);
		if (piece.flash > 0) piece.flash -= dt;
		// A door swung open or shut changes what is sealed.
		if (piece.kind == BuildKind::Door && piece.open != wasOpen_[piece.id]) {
			wasOpen_[piece.id] = piece.open;
			enclosureDirty_ = true;
		}
	}
}

namespace {

/** Whether you can step from one cell to its neighbour, or a wall is between. */
struct Step {
	int gx;
	int gy;
	EdgeSide side;
};

Step between(int cx, int cy, int nx, int ny) {
	if (cx == nx) return Step{cx, std::max(cy, ny), EdgeSide::North};
	return Step{std::max(cx, nx), cy, EdgeSide::West};
}

}  // namespace

void BuildSystem::rebuildEnclosure() const {
	enclosure_.clear();
	enclosureOwner_.clear();
	enclosureDirty_ = false;
	if (pieces_.empty()) return;

	// One box round everything built, with two cells of pad so the border ring
	// is open ground: that ring is where the flood from outside starts.
	int minX = 1 << 30;
	int minY = 1 << 30;
	int maxX = -(1 << 30);
	int maxY = -(1 << 30);
	for (const Structure& piece : pieces_) {
		minX = std::min(minX, piece.gx - 2);
		minY = std::min(minY, piece.gy - 2);
		maxX = std::max(maxX, piece.gx + 2);
		maxY = std::max(maxY, piece.gy + 2);
	}
	const int w = maxX - minX + 1;
	const int h = maxY - minY + 1;
	if (w <= 0 || h <= 0 || static_cast<long>(w) * h > 4000000) return;

	const auto open = [&](int cx, int cy, int nx, int ny) {
		const Step step = between(cx, cy, nx, ny);
		const Structure* blocker = edgeAt(step.gx, step.gy, step.side);
		if (!blocker) return true;
		// A doorway is a hole, and so is a door standing open.
		return blocker->kind == BuildKind::Doorway ||
			   (blocker->kind == BuildKind::Door && blocker->open);
	};
	const auto at = [&](int gx, int gy) { return (gy - minY) * w + (gx - minX); };

	// Pass one: everything the open air can walk to. However much wall is
	// about, if you can stroll in from outside it is not a room.
	std::vector<std::uint8_t> outside(static_cast<std::size_t>(w) * h, 0);
	std::vector<int> stack;
	const auto push = [&](int gx, int gy) {
		if (gx < minX || gx > maxX || gy < minY || gy > maxY) return;
		const int i = at(gx, gy);
		if (outside[i]) return;
		outside[i] = 1;
		stack.push_back(gx);
		stack.push_back(gy);
	};
	for (int gx = minX; gx <= maxX; ++gx) {
		push(gx, minY);
		push(gx, maxY);
	}
	for (int gy = minY; gy <= maxY; ++gy) {
		push(minX, gy);
		push(maxX, gy);
	}
	while (!stack.empty()) {
		const int cy = stack.back();
		stack.pop_back();
		const int cx = stack.back();
		stack.pop_back();
		if (open(cx, cy, cx, cy - 1)) push(cx, cy - 1);
		if (open(cx, cy, cx, cy + 1)) push(cx, cy + 1);
		if (open(cx, cy, cx - 1, cy)) push(cx - 1, cy);
		if (open(cx, cy, cx + 1, cy)) push(cx + 1, cy);
	}

	// Pass two: what is left is sealed. Grouped into rooms, so a room is one
	// region however many cells it spans and a hole anywhere opens all of it.
	int region = 1;
	std::vector<std::uint8_t> grouped(static_cast<std::size_t>(w) * h, 0);
	for (int gy = minY; gy <= maxY; ++gy) {
		for (int gx = minX; gx <= maxX; ++gx) {
			const int seed = at(gx, gy);
			if (outside[seed] || grouped[seed]) continue;
			grouped[seed] = 1;
			std::vector<int> room{gx, gy};
			std::vector<std::uint64_t> cells;
			int owner = -1;
			while (!room.empty()) {
				const int cy = room.back();
				room.pop_back();
				const int cx = room.back();
				room.pop_back();
				cells.push_back(cellKey(cx, cy));
				if (const Structure* floor = foundationAt(cx, cy)) {
					if (owner == -1) owner = floor->owner;
				}
				const int steps[4][2] = {{cx, cy - 1}, {cx, cy + 1}, {cx - 1, cy}, {cx + 1, cy}};
				for (const auto& step : steps) {
					const int nx = step[0];
					const int ny = step[1];
					if (nx < minX || nx > maxX || ny < minY || ny > maxY) continue;
					const int ni = at(nx, ny);
					if (grouped[ni] || outside[ni]) continue;
					if (!open(cx, cy, nx, ny)) continue;
					grouped[ni] = 1;
					room.push_back(nx);
					room.push_back(ny);
				}
			}
			for (const std::uint64_t key : cells) enclosure_[key] = region;
			enclosureOwner_[region] = owner;
			++region;
		}
	}
}

int BuildSystem::regionAt(double x, double y) const {
	if (enclosureDirty_) rebuildEnclosure();
	const auto it = enclosure_.find(cellKey(static_cast<int>(std::floor(x / kBuildCell)),
											static_cast<int>(std::floor(y / kBuildCell))));
	return it == enclosure_.end() ? 0 : it->second;
}

int BuildSystem::regionOwner(int region) const {
	if (enclosureDirty_) rebuildEnclosure();
	const auto it = enclosureOwner_.find(region);
	return it == enclosureOwner_.end() ? -1 : it->second;
}

const std::unordered_map<std::uint64_t, int>& BuildSystem::enclosedCells() const {
	if (enclosureDirty_) rebuildEnclosure();
	return enclosure_;
}

void BuildSystem::reindex() {
	// What is sealed changes with every piece, and with every door swung.
	enclosureDirty_ = true;
	byCell_.clear();
	byEdge_.clear();
	byRoof_.clear();
	for (int i = 0; i < static_cast<int>(pieces_.size()); ++i) {
		const Structure& piece = pieces_[i];
		if (piece.kind == BuildKind::Foundation) {
			byCell_[cellKey(piece.gx, piece.gy)] = i;
		} else if (piece.kind == BuildKind::Ceiling) {
			byRoof_[cellKey(piece.gx, piece.gy)] = i;
		} else {
			byEdge_[edgeKey(piece.gx, piece.gy, piece.side)] = i;
		}
	}
}

}  // namespace sim
