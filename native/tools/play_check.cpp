// Plays a minute of the game with nobody watching: walks to the nearest tree,
// swings until it falls, picks up what is lying about, and prints what ended
// up in the pack. A quick check that the rules still hold after a change.
#include <cmath>
#include <cstdio>
#include <vector>

#include "sim/action.hpp"

int main() {
    sim::World world;
    world.generate(12345);

    sim::Player player;
    player.x = 10368;
    player.y = 10368;
    sim::Inventory inventory;
    sim::NpcSystem npcs;
    npcs.populate(world, 12345);

    // The nearest tree to where we woke up.
    std::vector<const sim::ResourceNode*> near;
    world.nodesInRect(player.x - 600, player.y - 600, player.x + 600, player.y + 600, near);
    const sim::ResourceNode* target = nullptr;
    double best = 1e9;
    for (const sim::ResourceNode* node : near) {
        if (node->kind != sim::NodeKind::Tree) continue;
        const double d = std::hypot(node->x - player.x, node->y - player.y);
        if (d < best) {
            best = d;
            target = node;
        }
    }
    if (!target) {
        std::printf("no tree near the middle of the island\n");
        return 1;
    }

    const double dt = 1.0 / 60;
    int blows = 0;
    bool felled = false;
    for (int frame = 0; frame < 60 * 60 && !felled; ++frame) {
        const double dx = target->x - player.x;
        const double dy = target->y - player.y;
        const double d = std::hypot(dx, dy);
        sim::PlayerInput input;
        input.aim = std::atan2(dy, dx);
        if (d > 28) {
            input.moveX = dx / d;
            input.moveY = dy / d;
        }
        sim::stepPlayer(world, player, input, dt);
        world.update(dt);
        npcs.update(world, dt, player);
        if (d <= 40) {
            const sim::SwingResult blow = sim::swing(world, npcs, player, inventory);
            if (blow.swung) ++blows;
            if (blow.broke) felled = true;
        }
    }

    // Then the nearest animal: walked up to, hit until it falls, and what it
    // leaves picked up off the ground.
    const sim::Npc* prey = nullptr;
    double preyD = 1e9;
    for (const sim::Npc& npc : npcs.list()) {
        const double d = std::hypot(npc.x - player.x, npc.y - player.y);
        if (d < preyD) {
            preyD = d;
            prey = &npc;
        }
    }
    int hits = 0;
    bool killed = false;
    const char* preyName = prey ? sim::npcDef(prey->kind).name : "nothing";
    if (prey) {
        const int id = prey->id;
        for (int frame = 0; frame < 60 * 180 && !killed; ++frame) {
            const sim::Npc* live = nullptr;
            for (const sim::Npc& npc : npcs.list()) {
                if (npc.id == id) live = &npc;
            }
            if (!live || live->hp <= 0) {
                killed = true;
                break;
            }
            const double dx = live->x - player.x;
            const double dy = live->y - player.y;
            const double d = std::hypot(dx, dy);
            sim::PlayerInput input;
            input.aim = std::atan2(dy, dx);
            if (d > 24) {
                input.moveX = dx / d;
                input.moveY = dy / d;
            }
            sim::stepPlayer(world, player, input, dt);
            world.update(dt);
            const sim::NpcEvents events = npcs.update(world, dt, player);
            player.health -= static_cast<int>(events.playerDamage);
            if (d <= 30 && sim::swing(world, npcs, player, inventory).hitNpc) ++hits;
        }
        // Everything it dropped, gathered up.
        for (int i = 0; i < 12; ++i) {
            if (!sim::pickUp(world, player, inventory).picked) break;
        }
    }

    std::printf("hunt: %s %s after %d hits, leather %d, meat %d, health %d\n", preyName,
                killed ? "killed" : "got away", hits, inventory.count(sim::ItemId::Leather),
                inventory.count(sim::ItemId::MeatRaw), player.health);

    std::printf("tree at %.0f, %.0f: %s after %d blows, wood %d, stone %d, drops %zu\n", target->x,
                target->y, felled ? "felled" : "still standing", blows,
                inventory.count(sim::ItemId::Wood), inventory.count(sim::ItemId::Stone),
                world.drops().size());
    return felled ? 0 : 1;
}
