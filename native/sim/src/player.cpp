#include "sim/player.hpp"

#include <cmath>
#include <vector>

namespace sim {

namespace {

/** Pushed back out of anything solid it ended up inside. */
void separate(const World& world, Player& player) {
    static thread_local std::vector<const ResourceNode*> near;
    const double reach = PlayerRules::kRadius + 40;
    world.nodesInRect(player.x - reach, player.y - reach, player.x + reach, player.y + reach, near);
    for (const ResourceNode* node : near) {
        // A nettle is walked through; everything else is stood against.
        if (node->kind == NodeKind::Nettle) continue;
        const double dx = player.x - node->x;
        const double dy = player.y - node->y;
        const double min = PlayerRules::kRadius + node->radius * 0.6;
        const double d2 = dx * dx + dy * dy;
        if (d2 >= min * min || d2 < 0.0001) continue;
        const double d = std::sqrt(d2);
        player.x = node->x + dx / d * min;
        player.y = node->y + dy / d * min;
    }
}

}  // namespace

void stepPlayer(const World& world, Player& player, const PlayerInput& input, double dt) {
    player.aim = input.aim;
    player.swimming = world.biomeAt(player.x, player.y) == Biome::Water;

    double mx = input.moveX;
    double my = input.moveY;
    const double len = std::sqrt(mx * mx + my * my);
    const bool moving = len > 0.0001;
    if (moving) {
        mx /= len;
        my /= len;
    }

    player.sprinting = input.sprint && moving && !player.swimming;
    double speed = PlayerRules::kSpeed * (player.sprinting ? PlayerRules::kSprint : 1.0);
    if (player.swimming) speed *= PlayerRules::kSwim;

    player.x += mx * speed * dt;
    player.y += my * speed * dt;

    // The edge of the map is the edge of the world, as in the other game: you
    // swim to it and no further.
    const double edge = PlayerRules::kRadius;
    if (player.x < edge) player.x = edge;
    if (player.y < edge) player.y = edge;
    if (player.x > kWorldWidth - edge) player.x = kWorldWidth - edge;
    if (player.y > kWorldHeight - edge) player.y = kWorldHeight - edge;

    separate(world, player);

    if (moving) {
        player.walkPhase += dt * (player.sprinting ? 14 : 9);
    } else {
        player.walkPhase -= player.walkPhase * dt * 8;
    }
}

}  // namespace sim
