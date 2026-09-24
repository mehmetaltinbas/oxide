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
        if (d <= 40) {
            const sim::SwingResult blow = sim::swing(world, player, inventory);
            if (blow.swung) ++blows;
            if (blow.broke) felled = true;
        }
    }

    std::printf("tree at %.0f, %.0f: %s after %d blows, wood %d, stone %d, drops %zu\n", target->x,
                target->y, felled ? "felled" : "still standing", blows,
                inventory.count(sim::ItemId::Wood), inventory.count(sim::ItemId::Stone),
                world.drops().size());
    return felled ? 0 : 1;
}
