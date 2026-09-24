#include "sim/blast.hpp"

#include <algorithm>
#include <cmath>

#include "sim/survival.hpp"

namespace sim {

namespace {

/** Where a built piece sits, for measuring a blast against it. */
void centerOf(const Structure& piece, double& x, double& y) {
    if (piece.kind == BuildKind::Foundation) {
        x = (piece.gx + 0.5) * kBuildCell;
        y = (piece.gy + 0.5) * kBuildCell;
        return;
    }
    double x0 = 0;
    double y0 = 0;
    double x1 = 0;
    double y1 = 0;
    edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
    x = (x0 + x1) * 0.5;
    y = (y0 + y1) * 0.5;
}

}  // namespace

void Explosives::place(ItemId kind, double x, double y, int stuckTo) {
    charges_.push_back(Charge{nextId_++, kind, x, y, 0, 0, itemDef(kind).boom.fuse, stuckTo});
}

void Explosives::throwAt(ItemId kind, double fromX, double fromY, double toX, double toY) {
    const double a = std::atan2(toY - fromY, toX - fromX);
    // How far you aimed is how hard it is thrown, between a drop and a lob.
    const double power = std::clamp(std::hypot(toX - fromX, toY - fromY) * 2.4, kThrowMin, kThrowMax);
    charges_.push_back(Charge{nextId_++, kind, fromX + std::cos(a) * 18, fromY + std::sin(a) * 18,
                              std::cos(a) * power, std::sin(a) * power, itemDef(kind).boom.fuse,
                              0});
}

void Explosives::update(World& world, BuildSystem& build, NpcSystem& npcs, Player& player,
                        Inventory& inventory, double dt) {
    for (std::size_t i = charges_.size(); i-- > 0;) {
        Charge& flying = charges_[i];
        flying.fuse -= dt;
        if (flying.stuckTo == 0) {
            flying.x += flying.vx * dt;
            flying.y += flying.vy * dt;
            flying.vx *= 1 - kThrowDrag * dt;
            flying.vy *= 1 - kThrowDrag * dt;
            // While it is still moving it can find a wall to stick to; once it
            // has slowed it has simply landed where it landed.
            if (std::hypot(flying.vx, flying.vy) > 30) {
                if (Structure* wall = build.nearest(flying.x, flying.y, 26)) {
                    if (wall->kind != BuildKind::Foundation) {
                        flying.stuckTo = wall->id;
                        double x0 = 0;
                        double y0 = 0;
                        double x1 = 0;
                        double y1 = 0;
                        edgeSegment(wall->gx, wall->gy, wall->side, x0, y0, x1, y1);
                        flying.x = (x0 + x1) * 0.5;
                        flying.y = (y0 + y1) * 0.5;
                        flying.vx = 0;
                        flying.vy = 0;
                    }
                }
            }
        }
        if (charges_[i].fuse > 0) continue;
        const Charge charge = charges_[i];
        charges_.erase(charges_.begin() + static_cast<long>(i));
        const Boom& boom = itemDef(charge.kind).boom;
        // A charge breaks what it is stuck to and shakes everything else.
        detonate(world, build, npcs, player, inventory, charge.x, charge.y, boom.radius,
                 boom.damage, charge.stuckTo, false);
    }
}

void Explosives::detonate(World& world, BuildSystem& build, NpcSystem& npcs, Player& player,
                          Inventory& inventory, double x, double y, double radius, double damage,
                          int stuckTo, bool splash) {
    // What it was stuck to takes the whole charge, however big the piece is.
    if (stuckTo > 0) {
        for (Structure& piece : build.list2()) {
            if (piece.id != stuckTo) continue;
            build.damage(piece, damage);
            break;
        }
    }

    if (splash) {
        // Everything built that the blast reaches, by how far off it is.
        std::vector<int> ids;
        for (const Structure& piece : build.list()) {
            if (piece.id == stuckTo) continue;
            double cx = 0;
            double cy = 0;
            centerOf(piece, cx, cy);
            if (std::hypot(x - cx, y - cy) > radius) continue;
            ids.push_back(piece.id);
        }
        for (const int id : ids) {
            for (Structure& piece : build.list2()) {
                if (piece.id != id) continue;
                double cx = 0;
                double cy = 0;
                centerOf(piece, cx, cy);
                const double d = std::hypot(x - cx, y - cy);
                const double dealt = damage * std::pow(1 - d / radius, kBlastFalloff);
                if (dealt >= 1) build.damage(piece, dealt);
                break;
            }
        }

        // The world takes it as well: trees, ore, barrels, whatever is there.
        static thread_local std::vector<const ResourceNode*> near;
        world.nodesInRect(x - radius, y - radius, x + radius, y + radius, near);
        std::vector<int> nodeIds;
        for (const ResourceNode* node : near) {
            if (node->hp <= 0) continue;
            if (std::hypot(x - node->x, y - node->y) > radius + node->radius) continue;
            nodeIds.push_back(node->id);
        }
        for (const int id : nodeIds) {
            if (ResourceNode* node = world.nodeById(id)) {
                const double d = std::hypot(x - node->x, y - node->y);
                world.hurtNode(*node, damage * std::pow(1 - std::min(1.0, d / radius), kBlastFalloff));
            }
        }

        // And the deployables standing in it.
        for (Deployable& thing : build.deployables()) {
            const double d = std::hypot(x - thing.x, y - thing.y);
            if (d > radius) continue;
            thing.hp -= static_cast<int>(damage * std::pow(1 - d / radius, kBlastFalloff));
            thing.flash = 0.2;
        }
        std::vector<int> gone;
        for (const Deployable& thing : build.deployables()) {
            if (thing.hp <= 0) gone.push_back(thing.id);
        }
        for (const int id : gone) build.removeDeployable(id);
    }

    // Anything alive near it. A quarter of the charge to an animal, less to a
    // player: a raid is aimed at walls, and catching someone is a bonus.
    for (Npc& npc : npcs.mutableList()) {
        if (npc.hp <= 0) continue;
        const double reach = radius + npcDef(npc.kind).radius;
        const double d = std::hypot(x - npc.x, y - npc.y);
        if (d > reach) continue;
        npcs.hurt(world, npc, damage * 0.25 * (1 - d / reach), x, y);
    }
    if (player.alive) {
        const double reach = radius + PlayerRules::kRadius;
        const double d = std::hypot(x - player.x, y - player.y);
        if (d < reach) hurtPlayer(player, inventory, damage * 0.18 * (1 - d / reach));
    }
}

}  // namespace sim
