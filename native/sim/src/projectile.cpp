#include "sim/projectile.hpp"

#include <algorithm>
#include <cmath>

namespace sim {

namespace {

/**
 * Whether a line from a to b passes within `radius` of a point, and how far
 * along it that happens.
 *
 * This is the whole reason bullets are stepped as segments rather than points:
 * a rifle round covers twenty metres between two frames, and a point test at
 * each end walks straight through the tree in the middle.
 */
bool segmentHitsCircle(double ax, double ay, double bx, double by, double cx, double cy,
                       double radius, double& at) {
    const double dx = bx - ax;
    const double dy = by - ay;
    const double len2 = dx * dx + dy * dy;
    double t = len2 > 0 ? ((cx - ax) * dx + (cy - ay) * dy) / len2 : 0;
    t = std::clamp(t, 0.0, 1.0);
    const double px = ax + dx * t;
    const double py = ay + dy * t;
    const double d2 = (px - cx) * (px - cx) + (py - cy) * (py - cy);
    if (d2 > radius * radius) return false;
    at = t;
    return true;
}

}  // namespace

void Projectiles::spawn(double x, double y, double angle, const Gun& gun, double damage,
                        bool arrow) {
    Bullet bullet{};
    bullet.id = nextId_++;
    bullet.x = x;
    bullet.y = y;
    bullet.fromX = x;
    bullet.fromY = y;
    bullet.vx = std::cos(angle) * gun.speed;
    bullet.vy = std::sin(angle) * gun.speed;
    bullet.left = gun.range;
    bullet.damage = damage;
    bullet.fromPlayer = true;
    bullet.arrow = arrow;
    bullets_.push_back(bullet);
}

std::vector<BulletHit> Projectiles::update(World& world, NpcSystem& npcs, double dt) {
    std::vector<BulletHit> hits;
    static thread_local std::vector<const ResourceNode*> near;

    for (Bullet& bullet : bullets_) {
        if (bullet.left <= 0) continue;
        const double ax = bullet.x;
        const double ay = bullet.y;
        double step = std::hypot(bullet.vx, bullet.vy) * dt;
        if (step > bullet.left) step = bullet.left;
        const double ux = bullet.vx / std::hypot(bullet.vx, bullet.vy);
        const double uy = bullet.vy / std::hypot(bullet.vx, bullet.vy);
        const double bx = ax + ux * step;
        const double by = ay + uy * step;

        // The first thing along the line, animal or otherwise, is what it hits.
        double bestAt = 2;
        Npc* hitNpc = nullptr;
        const ResourceNode* hitNode = nullptr;

        for (const Npc& npc : npcs.list()) {
            if (npc.hp <= 0) continue;
            const double reach = npcDef(npc.kind).radius;
            if (std::abs(npc.x - ax) > step + reach + 40) continue;
            if (std::abs(npc.y - ay) > step + reach + 40) continue;
            double at = 0;
            if (!segmentHitsCircle(ax, ay, bx, by, npc.x, npc.y, reach, at)) continue;
            if (at >= bestAt) continue;
            bestAt = at;
            hitNpc = const_cast<Npc*>(&npc);
            hitNode = nullptr;
        }

        const double lo = std::min(ax, bx) - 40;
        const double hi = std::max(ax, bx) + 40;
        const double lo2 = std::min(ay, by) - 40;
        const double hi2 = std::max(ay, by) + 40;
        world.nodesInRect(lo, lo2, hi, hi2, near);
        for (const ResourceNode* node : near) {
            // A nettle is not cover, and neither is something already taken.
            if (node->hp <= 0 || node->kind == NodeKind::Nettle) continue;
            double at = 0;
            if (!segmentHitsCircle(ax, ay, bx, by, node->x, node->y, node->radius, at)) continue;
            if (at >= bestAt) continue;
            bestAt = at;
            hitNode = node;
            hitNpc = nullptr;
        }

        if (hitNpc) {
            BulletHit hit{};
            hit.x = ax + (bx - ax) * bestAt;
            hit.y = ay + (by - ay) * bestAt;
            hit.npc = true;
            hit.npcKind = hitNpc->kind;
            hit.killed = hitNpc->hp - static_cast<int>(bullet.damage) <= 0;
            npcs.hurt(world, *hitNpc, bullet.damage, ax, ay);
            hits.push_back(hit);
            bullet.left = 0;
            continue;
        }
        if (hitNode) {
            BulletHit hit{};
            hit.x = ax + (bx - ax) * bestAt;
            hit.y = ay + (by - ay) * bestAt;
            hit.node = true;
            hit.nodeKind = hitNode->kind;
            // A barrel is broken open by gunfire; a tree only stops the round.
            if (nodeDef(hitNode->kind).loot) {
                if (ResourceNode* live = world.nodeById(hitNode->id)) {
                    hit.brokeNode = world.hurtNode(*live, bullet.damage);
                }
            }
            hits.push_back(hit);
            bullet.left = 0;
            continue;
        }

        bullet.x = bx;
        bullet.y = by;
        bullet.left -= step;
        // Out over the sea, a round is gone: there is nothing for it to hit.
        if (bullet.x < 0 || bullet.y < 0 || bullet.x > kWorldWidth || bullet.y > kWorldHeight) {
            bullet.left = 0;
        }
    }

    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                  [](const Bullet& b) { return b.left <= 0; }),
                   bullets_.end());
    return hits;
}

}  // namespace sim
