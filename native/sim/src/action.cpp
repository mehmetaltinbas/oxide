#include "sim/action.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sim {

namespace {

/** How long a tool's swing is drawn for; fists and spears are quicker. */
constexpr double kToolSwingSeconds = 0.34;
constexpr double kFistSwingSeconds = 0.2;

/** How wide a blow reaches to either side of where you are looking. */
constexpr double kSwingCone = 0.9;

/** What is in a barrel, and how often. */
struct Loot {
    ItemId id;
    int low;
    int high;
    double chance;
};

constexpr Loot kBarrelLoot[4] = {
    {ItemId::Scrap, 3, 6, 1.0},
    {ItemId::Metal, 10, 25, 0.3},
    {ItemId::LowGrade, 5, 12, 0.25},
    {ItemId::PistolAmmo, 4, 8, 0.12},
};

bool inCone(const Player& p, double x, double y) {
    const double a = std::atan2(y - p.y, x - p.x);
    double d = a - p.aim;
    while (d > 3.14159265358979) d -= 6.28318530717959;
    while (d < -3.14159265358979) d += 6.28318530717959;
    return std::abs(d) <= kSwingCone;
}

double dist(double ax, double ay, double bx, double by) {
    return std::sqrt((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
}

/** What bare hands can work: a tree, and a plain stone. */
bool fistsCanWork(NodeKind kind) { return kind == NodeKind::Tree || kind == NodeKind::Stone; }

}  // namespace

SwingResult swing(World& world, NpcSystem& npcs, Player& player, Inventory& inventory) {
    SwingResult out;
    if (player.attackTimer > 0 || player.swimming) return out;

    const ItemId held = inventory.held();
    const bool tool = itemDef(held).category == ItemCategory::Tool;
    Melee melee = tool ? itemDef(held).melee : itemDef(ItemId::Rock).melee;
    if (!tool) {
        // Bare hands: half the rock's damage and half its yield.
        melee.damage *= kFistFraction;
        melee.gather *= kFistFraction;
    }

    out.swung = true;
    player.attackTimer = melee.cooldown;
    player.swingLength = tool ? kToolSwingSeconds : kFistSwingSeconds;
    player.swingAnim = player.swingLength;

    // Where the head of it lands, for the puff of dust when it hits nothing.
    out.x = player.x + std::cos(player.aim) * melee.reach * 0.75;
    out.y = player.y + std::sin(player.aim) * melee.reach * 0.75;

    out.damage = melee.damage;

    // Living things first: a swing that could hit either takes the animal.
    if (Npc* animal = npcs.nearest(player.x, player.y, melee.reach)) {
        if (inCone(player, animal->x, animal->y)) {
            const bool died = animal->hp - static_cast<int>(melee.damage) <= 0;
            npcs.hurt(world, *animal, melee.damage, player.x, player.y);
            out.killed = died;
            out.landed = true;
            out.hitNpc = true;
            out.npcKind = animal->kind;
            out.x = animal->x;
            out.y = animal->y;
            return out;
        }
    }

    static thread_local std::vector<const ResourceNode*> near;
    const double look = melee.reach + 40;
    world.nodesInRect(player.x - look, player.y - look, player.x + look, player.y + look, near);
    // The nearest thing in front of you, rather than whichever the grid
    // happened to hand over first.
    const ResourceNode* pick = nullptr;
    double best = 0;
    for (const ResourceNode* node : near) {
        if (node->hp <= 0 || node->kind == NodeKind::Nettle) continue;
        if (!tool && !fistsCanWork(node->kind)) continue;
        const double d = dist(player.x, player.y, node->x, node->y);
        if (d > melee.reach + node->radius) continue;
        if (!inCone(player, node->x, node->y)) continue;
        if (!pick || d < best) {
            pick = node;
            best = d;
        }
    }
    if (!pick) return out;

    ResourceNode* node = world.nodeById(pick->id);
    if (!node) return out;
    const NodeDef& def = nodeDef(node->kind);
    out.landed = true;
    out.nodeId = node->id;
    out.kind = node->kind;
    out.x = node->x;
    out.y = node->y;

    if (def.loot) {
        // A barrel gives nothing until it goes, and then all of it at once.
        out.broke = world.hurtNode(*node, melee.damage);
        if (!out.broke) return out;
        Rng rng(node->seed * 2654435761u + 17u);
        for (const Loot& loot : kBarrelLoot) {
            if (rng.unit() > loot.chance) continue;
            const int amount = static_cast<int>(std::lround(rng.range(loot.low, loot.high)));
            if (amount <= 0) continue;
            // Spilled where it stood, scattered a little so the stacks do not
            // sit in one pile.
            const double a = rng.unit() * 6.28318530717959;
            const double d = rng.range(4, 16);
            world.dropStack(ItemStack{loot.id, amount}, node->x + std::cos(a) * d,
                            node->y + std::sin(a) * d);
        }
        return out;
    }

    // The right tool for the job: hatchets on wood, pickaxes on rock.
    double mult = melee.gather;
    if (held == ItemId::Hatchet) mult *= def.prefers == Work::Chop ? 1.6 : 0.45;
    if (held == ItemId::Pickaxe) mult *= def.prefers == Work::Mine ? 1.6 : 0.45;

    out.broke = world.hurtNode(*node, melee.damage);
    for (int i = 0; i < def.yieldCount; ++i) {
        const int amount = std::max(1, static_cast<int>(std::lround(def.yields[i].per * mult * 0.35)));
        const int left = inventory.add(def.yields[i].id, amount);
        if (left > 0) {
            out.packFull = true;
            // What will not fit falls at your feet rather than vanishing.
            world.dropStack(ItemStack{def.yields[i].id, left}, player.x, player.y);
        }
        if (i == 0) out.gained = ItemStack{def.yields[i].id, amount};
    }
    return out;
}

PickResult pickUp(World& world, const Player& player, Inventory& inventory) {
    PickResult out;
    // Anything lying on the ground first: that is what the key is mostly for.
    const Dropped* nearest = nullptr;
    double best = kPickReach;
    for (const Dropped& drop : world.drops()) {
        const double d = dist(player.x, player.y, drop.x, drop.y);
        if (d > best) continue;
        nearest = &drop;
        best = d;
    }
    if (nearest) {
        const int left = inventory.add(nearest->stack.id, nearest->stack.count);
        out.picked = true;
        out.stack = nearest->stack;
        out.x = nearest->x;
        out.y = nearest->y;
        if (left > 0) {
            // Room for some of it: the rest stays where it is.
            out.packFull = true;
            out.stack.count -= left;
            const int id = nearest->id;
            world.removeDrop(id);
            if (out.stack.count > 0) world.dropStack(ItemStack{out.stack.id, left}, out.x, out.y);
        } else {
            world.removeDrop(nearest->id);
        }
        return out;
    }

    // Then a nettle, which is picked by hand rather than swung at.
    static thread_local std::vector<const ResourceNode*> near;
    world.nodesInRect(player.x - kPickReach, player.y - kPickReach, player.x + kPickReach,
                      player.y + kPickReach, near);
    for (const ResourceNode* node : near) {
        if (node->kind != NodeKind::Nettle || node->hp <= 0) continue;
        if (dist(player.x, player.y, node->x, node->y) > kPickReach + node->radius) continue;
        ResourceNode* live = world.nodeById(node->id);
        if (!live) continue;
        const NodeDef& def = nodeDef(NodeKind::Nettle);
        world.hurtNode(*live, live->maxHp);
        const int left = inventory.add(def.yields[0].id, def.yields[0].per);
        out.picked = true;
        out.stack = ItemStack{def.yields[0].id, def.yields[0].per - left};
        out.x = live->x;
        out.y = live->y;
        out.packFull = left > 0;
        if (left > 0) world.dropStack(ItemStack{def.yields[0].id, left}, live->x, live->y);
        return out;
    }
    return out;
}

}  // namespace sim
