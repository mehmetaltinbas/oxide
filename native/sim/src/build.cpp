#include "sim/build.hpp"

#include "sim/survival.hpp"

#include <algorithm>
#include <cmath>

namespace sim {

namespace {

constexpr TierDef kTiers[kBuildTierCount] = {
    {BuildTier::Twig, "Twig", 10, {ItemId::Wood, 10}},
    {BuildTier::Wood, "Wood", 250, {ItemId::Wood, 50}},
    {BuildTier::Stone, "Stone", 500, {ItemId::Stone, 150}},
    {BuildTier::Metal, "Sheet Metal", 1000, {ItemId::Metal, 200}},
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

const char* BuildSystem::refuseFoundation(const World& world, int gx, int gy, int) const {
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

Structure& BuildSystem::placeFoundation(int gx, int gy, int owner, BuildTier tier) {
    const int hp = static_cast<int>(std::lround(tierDef(tier).hp * kFoundationHpMul));
    pieces_.push_back(Structure{nextId_++, BuildKind::Foundation, tier, gx, gy, EdgeSide::North, hp,
                                hp, owner, false, false, 0});
    reindex();
    return pieces_.back();
}

Structure& BuildSystem::placeEdge(int gx, int gy, EdgeSide side, BuildKind kind, int owner,
                                  BuildTier tier) {
    const int hp = tierDef(tier).hp;
    pieces_.push_back(
        Structure{nextId_++, kind, tier, gx, gy, side, hp, hp, owner, false, false, 0});
    reindex();
    return pieces_.back();
}

bool BuildSystem::nextTier(const Structure& piece, BuildTier& out) const {
    if (piece.tier == BuildTier::Metal) return false;
    out = static_cast<BuildTier>(static_cast<int>(piece.tier) + 1);
    return true;
}

bool BuildSystem::upgrade(Structure& piece, Inventory& inventory) {
    BuildTier tier = BuildTier::Twig;
    if (!nextTier(piece, tier)) return false;
    const TierDef& def = tierDef(tier);
    if (inventory.count(def.cost.id) < def.cost.count) return false;
    inventory.take(def.cost.id, def.cost.count);
    piece.tier = tier;
    const double mul = piece.kind == BuildKind::Foundation ? kFoundationHpMul
                       : piece.kind == BuildKind::Door     ? kDoorHpMul
                                                           : 1.0;
    piece.maxHp = static_cast<int>(std::lround(def.hp * mul));
    piece.hp = piece.maxHp;
    return true;
}

bool BuildSystem::damage(Structure& piece, double amount) {
    if (piece.hp <= 0) return false;
    piece.hp -= static_cast<int>(amount);
    piece.flash = 0.12;
    if (piece.hp > 0) return false;
    piece.hp = 0;
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
                const double min = radius + kWallThickness;
                if (d >= min || d <= 0.0001) continue;
                const double push = (min - d) / d;
                x += (x - hx) * push;
                y += (y - hy) * push;
            }
            for (const Deployable& d : deployables_) {
                if (d.kind == DeployKind::SleepingBag) continue;
                if (static_cast<int>(d.x / kBuildCell) != gx) continue;
                if (static_cast<int>(d.y / kBuildCell) != gy) continue;
                const double nx = std::clamp(x, d.x - kDeployHalf, d.x + kDeployHalf);
                const double ny = std::clamp(y, d.y - kDeployHalf, d.y + kDeployHalf);
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

Structure* BuildSystem::nearest(double x, double y, double within) {
    Structure* best = nullptr;
    double bestD = within;
    for (Structure& piece : pieces_) {
        double px = 0;
        double py = 0;
        if (piece.kind == BuildKind::Foundation) {
            px = (piece.gx + 0.5) * kBuildCell;
            py = (piece.gy + 0.5) * kBuildCell;
        } else {
            double x0 = 0;
            double y0 = 0;
            double x1 = 0;
            double y1 = 0;
            edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
            closestOnSegment(x, y, x0, y0, x1, y1, px, py);
        }
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

const char* BuildSystem::refuseDeploy(const World& world, int gx, int gy, DeployKind kind,
                                      int owner) const {
    const double cx = (gx + 0.5) * kBuildCell;
    const double cy = (gy + 0.5) * kBuildCell;
    for (const Deployable& d : deployables_) {
        if (static_cast<int>(d.x / kBuildCell) == gx && static_cast<int>(d.y / kBuildCell) == gy) {
            return "Something is already here";
        }
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
    if (naturalCover(world, cx, cy, kBuildCell * 0.4)) return "Something is in the way";
    return nullptr;
}

Deployable* BuildSystem::deployableById(int id) {
    for (Deployable& d : deployables_) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

int BuildSystem::deploy(DeployKind kind, int gx, int gy, int owner) {
    Deployable d{};
    d.id = nextDeployId_++;
    d.kind = kind;
    d.x = (gx + 0.5) * kBuildCell;
    d.y = (gy + 0.5) * kBuildCell;
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

void BuildSystem::updateDeployables(double dt) {
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
    }
    pieces_.erase(std::remove_if(pieces_.begin(), pieces_.end(),
                                 [](const Structure& piece) { return piece.hp <= 0; }),
                  pieces_.end());
    reindex();
}

void BuildSystem::update(double dt) {
    for (Structure& piece : pieces_) {
        if (piece.flash > 0) piece.flash -= dt;
    }
}

void BuildSystem::reindex() {
    byCell_.clear();
    byEdge_.clear();
    for (int i = 0; i < static_cast<int>(pieces_.size()); ++i) {
        const Structure& piece = pieces_[i];
        if (piece.kind == BuildKind::Foundation) {
            byCell_[cellKey(piece.gx, piece.gy)] = i;
        } else {
            byEdge_[edgeKey(piece.gx, piece.gy, piece.side)] = i;
        }
    }
}

}  // namespace sim
