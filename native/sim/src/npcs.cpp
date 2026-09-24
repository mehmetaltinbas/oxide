#include "sim/npcs.hpp"

#include "sim/projectile.hpp"

#include <algorithm>
#include <cmath>

namespace sim {

namespace {

constexpr double kTau = 6.28318530717959;

/** The island's population. Meeting anything should be an event. */
struct Population {
    NpcKind kind;
    int count;
};

constexpr Population kWildlife[3] = {
    {NpcKind::Boar, 90},
    {NpcKind::Wolf, 55},
    {NpcKind::Bear, 26},
};

/** Everything killed comes back a day later, as every other resource does. */
constexpr double kRegrowthSeconds = 3600;
constexpr double kRegrowthSpread = 0.08;

/** Where wildlife may appear: in off the rim, and never on a road. */
constexpr double kSpawnMargin = 120;
constexpr int kSpawnTries = 40;

double dist(double ax, double ay, double bx, double by) {
    return std::hypot(ax - bx, ay - by);
}

/** Turned towards an angle, by at most this much. */
double approachAngle(double from, double to, double by) {
    double d = to - from;
    while (d > 3.14159265358979) d -= kTau;
    while (d < -3.14159265358979) d += kTau;
    return from + std::clamp(d, -by, by);
}

}  // namespace

void NpcSystem::populate(const World& world, std::uint32_t seed) {
    npcs_.clear();
    Rng rng(seed ^ 0x5eedbeefu);
    for (const Population& group : kWildlife) {
        for (int i = 0; i < group.count; ++i) {
            for (int tries = 0; tries < kSpawnTries; ++tries) {
                const double x = rng.range(kSpawnMargin, kWorldWidth - kSpawnMargin);
                const double y = rng.range(kSpawnMargin, kWorldHeight - kSpawnMargin);
                const Biome biome = world.biomeAt(x, y);
                if (biome == Biome::Water || biome == Biome::Road) continue;
                const NpcDef& def = npcDef(group.kind);
                Npc npc{};
                npc.id = nextId_++;
                npc.kind = group.kind;
                npc.x = x;
                npc.y = y;
                npc.facing = rng.unit() * kTau;
                npc.hp = def.hp;
                npc.state = NpcState::Wander;
                npc.homeX = x;
                npc.homeY = y;
                npc.leash = kNpcLeash;
                npc.seed = static_cast<std::uint32_t>(rng.unit() * 1e5);
                npcs_.push_back(npc);
                break;
            }
        }
    }
}

void NpcSystem::inRect(double x0, double y0, double x1, double y1,
                       std::vector<const Npc*>& out) const {
    out.clear();
    for (const Npc& npc : npcs_) {
        if (npc.hp <= 0) continue;
        if (npc.x < x0 || npc.x > x1 || npc.y < y0 || npc.y > y1) continue;
        out.push_back(&npc);
    }
}

Npc* NpcSystem::nearest(double x, double y, double within) {
    Npc* best = nullptr;
    double bestD = within;
    for (Npc& npc : npcs_) {
        if (npc.hp <= 0) continue;
        const double d = dist(x, y, npc.x, npc.y) - npcDef(npc.kind).radius;
        if (d > bestD) continue;
        best = &npc;
        bestD = d;
    }
    return best;
}

void NpcSystem::hurt(World& world, Npc& npc, double amount, double fromX, double fromY) {
    // Anything hit turns on whoever hit it, however far off they were.
    if (npc.hp <= 0) return;
    npc.hp -= static_cast<int>(amount);
    if (npc.hp <= 0) {
        npc.hp = 0;
        drop(world, npc);
        // Owed back to the island: the same animal, where it lived, a day on.
        Rng rng(rolls_ += 0x9e3779b9u);
        const double slack = kRegrowthSeconds * kRegrowthSpread;
        regrowth_.push_back(Regrowth{npc.kind, npc.guard, npc.homeX, npc.homeY, npc.leash,
                                     kRegrowthSeconds + rng.range(-slack, slack)});
        return;
    }
    npc.flash = 0.12;
    // Anything that is hit fights back, whether or not it started hostile.
    npc.state = NpcState::Chase;
    npc.facing = std::atan2(fromY - npc.y, fromX - npc.x);
    npc.shoreWait = 0;
}

void NpcSystem::drop(World& world, const Npc& npc) {
    const NpcDef& def = npcDef(npc.kind);
    Rng rng(npc.seed * 2246822519u + 7u);
    for (int i = 0; i < def.lootCount; ++i) {
        const NpcLoot& loot = def.loot[i];
        if (loot.id == ItemId::None) continue;
        const int amount = static_cast<int>(std::lround(rng.range(loot.low, loot.high)));
        if (amount <= 0) continue;
        const double a = rng.unit() * kTau;
        const double d = rng.range(4, 18);
        world.dropStack(ItemStack{loot.id, amount}, npc.x + std::cos(a) * d,
                        npc.y + std::sin(a) * d);
    }
    // Whatever they were shooting at you with is on the ground now.
    if (def.gun.damage > 0) {
        const ItemId weapon = npc.kind == NpcKind::Soldier ? ItemId::Ak47 : ItemId::Revolver;
        world.dropStack(ItemStack{weapon, 1}, npc.x, npc.y);
    }
}

void NpcSystem::garrison(const World& world, std::uint32_t seed) {
    Rng rng(seed ^ 0x9a7c15d3u);
    for (const Monument& monument : world.monuments()) {
        const MonumentDef& def = monumentDef(monument.kind);
        const NpcKind kind = def.soldiers ? NpcKind::Soldier : NpcKind::Scientist;
        for (int i = 0; i < def.scientists; ++i) {
            // Posted around the place they hold, not dropped in the middle.
            for (int tries = 0; tries < 40; ++tries) {
                const double a = rng.unit() * kTau;
                const double r = rng.range(monument.radius * 0.2, monument.radius * 0.85);
                const double x = monument.x + std::cos(a) * r;
                const double y = monument.y + std::sin(a) * r;
                if (world.biomeAt(x, y) == Biome::Water) continue;
                Npc npc{};
                npc.id = nextId_++;
                npc.kind = kind;
                npc.x = x;
                npc.y = y;
                npc.facing = rng.unit() * kTau;
                npc.hp = npcDef(kind).hp;
                npc.state = NpcState::Wander;
                npc.homeX = x;
                npc.homeY = y;
                // A guard holds its ground rather than wandering the island.
                npc.leash = monument.radius * 0.9;
                npc.guard = true;
                npc.seed = static_cast<std::uint32_t>(rng.unit() * 1e5);
                npcs_.push_back(npc);
                break;
            }
        }
    }
}

NpcEvents NpcSystem::update(World& world, const BuildSystem& build, Projectiles& projectiles,
                            double dt, const Player& player) {
    NpcEvents events;
    const double active2 = kNpcActiveRadius * kNpcActiveRadius;
    const bool swimming = world.biomeAt(player.x, player.y) == Biome::Water;

    // What was killed comes back where it belonged, a day on.
    for (std::size_t i = regrowth_.size(); i-- > 0;) {
        regrowth_[i].seconds -= dt;
        if (regrowth_[i].seconds > 0) continue;
        Rng rng(rolls_ += 0x9e3779b9u);
        Npc npc{};
        npc.id = nextId_++;
        npc.kind = regrowth_[i].kind;
        npc.x = regrowth_[i].homeX;
        npc.y = regrowth_[i].homeY;
        npc.facing = rng.unit() * kTau;
        npc.hp = npcDef(npc.kind).hp;
        npc.state = NpcState::Wander;
        npc.homeX = npc.x;
        npc.homeY = npc.y;
        npc.leash = regrowth_[i].leash;
        npc.guard = regrowth_[i].guard;
        npc.seed = static_cast<std::uint32_t>(rng.unit() * 1e5);
        npcs_.push_back(npc);
        regrowth_.erase(regrowth_.begin() + static_cast<long>(i));
    }
    // A body is cleared away once it has given up what it was worth.
    npcs_.erase(std::remove_if(npcs_.begin(), npcs_.end(),
                               [](const Npc& npc) { return npc.hp <= 0; }),
                npcs_.end());

    // What was killed comes back where it belonged, a day on.
    for (std::size_t i = regrowth_.size(); i-- > 0;) {
        regrowth_[i].seconds -= dt;
        if (regrowth_[i].seconds > 0) continue;
        Rng rng(rolls_ += 0x9e3779b9u);
        Npc npc{};
        npc.id = nextId_++;
        npc.kind = regrowth_[i].kind;
        npc.x = regrowth_[i].homeX;
        npc.y = regrowth_[i].homeY;
        npc.facing = rng.unit() * kTau;
        npc.hp = npcDef(npc.kind).hp;
        npc.state = NpcState::Wander;
        npc.homeX = npc.x;
        npc.homeY = npc.y;
        npc.leash = regrowth_[i].leash;
        npc.guard = regrowth_[i].guard;
        npc.seed = static_cast<std::uint32_t>(rng.unit() * 1e5);
        npcs_.push_back(npc);
        regrowth_.erase(regrowth_.begin() + static_cast<long>(i));
    }
    // A body is cleared away once it has given up what it was worth.
    npcs_.erase(std::remove_if(npcs_.begin(), npcs_.end(),
                               [](const Npc& npc) { return npc.hp <= 0; }),
                npcs_.end());

    for (Npc& npc : npcs_) {
        if (npc.hp <= 0) continue;
        const double dx = npc.x - player.x;
        const double dy = npc.y - player.y;
        // Outside the active radius it is left frozen, which is what keeps the
        // cost of this flat however big the island gets.
        if (dx * dx + dy * dy > active2) continue;

        const NpcDef& def = npcDef(npc.kind);
        npc.stateTime += dt;
        npc.attackTimer -= dt;
        npc.gunTimer -= dt;
        if (npc.flash > 0) npc.flash -= dt;
        npc.animPhase += dt * (2 + std::hypot(npc.vx, npc.vy) / 40);

        const double toPlayer = dist(npc.x, npc.y, player.x, player.y);
        const double fromHome = dist(npc.x, npc.y, npc.homeX, npc.homeY);
        const bool provoked = npc.state == NpcState::Chase || npc.state == NpcState::Attack;
        const bool wantsFight = def.hostile || provoked;

        // Out in the water you are out of reach, and after a moment an animal
        // stops pretending otherwise: it drops the chase and goes home.
        if (swimming && provoked) {
            npc.shoreWait += dt;
            if (npc.shoreWait >= kShoreGiveUp) {
                npc.state = NpcState::Return;
                npc.shoreWait = 0;
            }
        } else if (!swimming) {
            npc.shoreWait = 0;
        }
        const bool givenUp = swimming && npc.state != NpcState::Chase && npc.state != NpcState::Attack;

        double wantX = 0;
        double wantY = 0;
        // Anything with a gun opens up from much further off than a bear does.
        const double aggroRange = def.gun.damage > 0 ? def.gun.range * 0.85 : 300;
        if (def.gun.damage > 0 && wantsFight && player.alive && toPlayer < aggroRange &&
            fromHome < npc.leash * 2.4) {
            // Shooting: hold a firing distance, and do not shoot through walls.
            const double a = std::atan2(player.y - npc.y, player.x - npc.x);
            npc.facing = approachAngle(npc.facing, a, 6 * dt);
            npc.state = NpcState::Chase;
            double blocked = 2;
            const bool clear =
                const_cast<BuildSystem&>(build).hitSegment(npc.x, npc.y, player.x, player.y,
                                                           blocked) == nullptr;
            if (clear && npc.gunTimer <= 0 && toPlayer < def.gun.range) {
                Rng rng(rolls_ += 0x9e3779b9u);
                npc.gunTimer = def.gun.cooldown * rng.range(0.85, 1.2);
                const double spread = rng.range(-def.gun.spread, def.gun.spread);
                Gun gun{};
                gun.damage = def.gun.damage;
                gun.speed = def.gun.speed;
                gun.range = def.gun.range;
                projectiles.spawnHostile(npc.x + std::cos(a) * (def.radius + 8),
                                         npc.y + std::sin(a) * (def.radius + 8), a + spread, gun,
                                         def.gun.damage);
            }
            const double ideal = def.gun.range * 0.55;
            double speed = 0;
            if (toPlayer > ideal * 1.15) {
                speed = def.speed;
            } else if (toPlayer < ideal * 0.6) {
                speed = -def.speed * 0.8;
            }
            if (!clear) speed = def.speed;
            wantX = std::cos(a) * speed;
            wantY = std::sin(a) * speed;
        } else if (wantsFight && !givenUp && toPlayer < aggroRange &&
                   fromHome < npc.leash * 2.4) {
            npc.state = toPlayer <= def.attackRange + def.radius ? NpcState::Attack : NpcState::Chase;
            npc.facing = approachAngle(npc.facing, std::atan2(player.y - npc.y, player.x - npc.x),
                                       8 * dt);
            if (npc.state == NpcState::Attack) {
                if (npc.attackTimer <= 0) {
                    npc.attackTimer = def.attackCooldown;
                    events.playerDamage += def.damage;
                    events.fromX = npc.x;
                    events.fromY = npc.y;
                }
            } else {
                wantX = std::cos(npc.facing) * def.speed;
                wantY = std::sin(npc.facing) * def.speed;
            }
        } else if (fromHome > npc.leash) {
            npc.state = NpcState::Return;
            npc.facing = approachAngle(npc.facing, std::atan2(npc.homeY - npc.y, npc.homeX - npc.x),
                                       4 * dt);
            wantX = std::cos(npc.facing) * def.speed * 0.5;
            wantY = std::sin(npc.facing) * def.speed * 0.5;
        } else {
            npc.state = NpcState::Wander;
            Rng rng(rolls_ += 0x9e3779b9u);
            if (npc.stateTime > rng.range(1.5, 4)) {
                npc.stateTime = 0;
                npc.facing = rng.unit() * kTau;
            }
            const double drift = rng.unit() < 0.6 ? def.speed * 0.3 : 0;
            wantX = std::cos(npc.facing) * drift;
            wantY = std::sin(npc.facing) * drift;
        }

        npc.vx = wantX + npc.knockX;
        npc.vy = wantY + npc.knockY;
        npc.knockX *= 1 - std::clamp(9 * dt, 0.0, 1.0);
        npc.knockY *= 1 - std::clamp(9 * dt, 0.0, 1.0);
        double nx = npc.x + npc.vx * dt;
        double ny = npc.y + npc.vy * dt;
        build.resolve(nx, ny, def.radius);
        // Nobody walks into the sea. Stepping one axis at a time lets them run
        // the shoreline instead, which is what someone who can swim but would
        // rather not actually does. Anything already in the water is left free
        // to move, or it would be stuck there for good.
        const bool wading = world.biomeAt(npc.x, npc.y) == Biome::Water;
        if (world.biomeAt(nx, ny) == Biome::Water && !wading) {
            if (world.biomeAt(nx, npc.y) != Biome::Water) {
                ny = npc.y;
            } else if (world.biomeAt(npc.x, ny) != Biome::Water) {
                nx = npc.x;
            } else {
                nx = npc.x;
                ny = npc.y;
            }
        }
        npc.x = nx;
        npc.y = ny;
        npc.x = std::clamp(npc.x, def.radius, static_cast<double>(kWorldWidth) - def.radius);
        npc.y = std::clamp(npc.y, def.radius, static_cast<double>(kWorldHeight) - def.radius);
    }
    return events;
}

}  // namespace sim
