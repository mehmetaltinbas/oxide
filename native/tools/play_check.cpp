// Plays a minute of the game with nobody watching: walks to the nearest tree,
// swings until it falls, picks up what is lying about, and prints what ended
// up in the pack. A quick check that the rules still hold after a change.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "sim/action.hpp"
#include "sim/blast.hpp"
#include "sim/build.hpp"
#include "sim/survival.hpp"
#include "sim/craft.hpp"
#include "sim/save.hpp"
#include "sim/projectile.hpp"

int main() {
    sim::World world;
    world.generate(12345);

    sim::Player player;
    player.x = 10368;
    player.y = 10368;
    sim::Inventory inventory;
    sim::BuildSystem build;
    sim::NpcSystem npcs;
    sim::Projectiles projectiles;
    npcs.populate(world, 12345);
    npcs.garrison(world, 12345);

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
        sim::stepPlayer(world, build, player, input, dt);
        world.update(dt);
        npcs.update(world, build, projectiles, dt, player);
        if (d <= 40) {
            const sim::SwingResult blow = sim::swing(world, npcs, player, inventory);
            if (blow.swung) ++blows;
            if (blow.broke) felled = true;
        }
    }

    // Every item has a row of its own: eight of them once drifted out of step
    // with their ids, and a rock stopped being able to fell a tree.
    {
        int nameless = 0;
        for (int i = 1; i < sim::kItemCount; ++i) {
            const sim::ItemDef& def = sim::itemDef(static_cast<sim::ItemId>(i));
            if (def.name[0] == '\0') ++nameless;
        }
        std::printf("items: %d of %d have a definition\n", sim::kItemCount - 1 - nameless,
                    sim::kItemCount - 1);
        if (nameless > 0) return 1;
    }

    // A day of standing still: what hunger, thirst and the cold do on their own.
    {
        sim::Player idle = player;
        idle.x = player.x;
        idle.y = player.y;
        sim::Inventory bare;
        double clock = 0;
        while (idle.alive && clock < 3600) {
            sim::updateSurvival(world, idle, bare, 1.0 / 30, 0, 0);
            clock += 1.0 / 30;
        }
        std::printf("survival: after %.0f minutes still %s, food %.0f, water %.0f, health %.0f\n",
                    clock / 60, idle.alive ? "up" : "down", idle.calories, idle.hydration,
                    idle.health);
    }

    // A monument's guards: put in front of one and shot at, to see that they
    // shoot back and that they do not shoot each other.
    {
        const sim::Monument& monument = world.monuments().front();
        sim::Player visitor;
        visitor.x = monument.x;
        visitor.y = monument.y + monument.radius * 0.4;
        sim::Inventory kit;
        sim::Projectiles fire;
        int guards = 0;
        for (const sim::Npc& npc : npcs.list()) {
            if (sim::npcDef(npc.kind).gun.damage > 0 &&
                std::hypot(npc.x - monument.x, npc.y - monument.y) < monument.radius) {
                ++guards;
            }
        }
        double taken = 0;
        for (int i = 0; i < 60 * 20; ++i) {
            npcs.update(world, build, fire, dt, visitor);
            for (const sim::BulletHit& hit : fire.update(world, npcs, build, dt, &visitor)) {
                if (hit.player) taken += hit.damage;
            }
        }
        int standing = 0;
        for (const sim::Npc& npc : npcs.list()) {
            if (npc.hp > 0 && sim::npcDef(npc.kind).gun.damage > 0) ++standing;
        }
        std::printf("guards: %s holds %d, took %.0f in twenty seconds, %d armed still up\n",
                    sim::monumentDef(monument.kind).name, guards, taken, standing);
    }

    // A fire with meat on it and a furnace with ore in it: both left to burn.
    {
        const int fireId = build.deploy(sim::DeployKind::Campfire, 2, 2, 0);
        const int furnaceId = build.deploy(sim::DeployKind::Furnace, 4, 2, 0);
        {
            sim::Deployable& fire = *build.deployableById(fireId);
            fire.lit = true;
            fire.container.add(sim::ItemId::Wood, 30);
            fire.container.add(sim::ItemId::MeatRaw, 4);
            sim::Deployable& furnace = *build.deployableById(furnaceId);
            furnace.lit = true;
            furnace.container.add(sim::ItemId::Wood, 30);
            furnace.container.add(sim::ItemId::MetalOre, 6);
        }
        for (int i = 0; i < 60 * 40; ++i) build.updateDeployables(dt);
        const sim::Deployable& fire = *build.deployableById(fireId);
        const sim::Deployable& furnace = *build.deployableById(furnaceId);
        std::printf("fire: %d cooked, %d raw left; furnace: %d metal, %d charcoal, %d ore left\n",
                    fire.container.count(sim::ItemId::MeatCooked),
                    fire.container.count(sim::ItemId::MeatRaw),
                    furnace.container.count(sim::ItemId::Metal),
                    furnace.container.count(sim::ItemId::Charcoal),
                    furnace.container.count(sim::ItemId::MetalOre));
    }

    // A foundation and a wall, and a shoulder against the wall: what is built
    // has to stop you walking through it, or none of it means anything.
    {
        const int gx = static_cast<int>(player.x / sim::kBuildCell);
        const int gy = static_cast<int>(player.y / sim::kBuildCell) + 2;
        build.placeFoundation(gx, gy, 0, sim::BuildTier::Wood);
        build.placeEdge(gx, gy, sim::EdgeSide::North, sim::BuildKind::Wall, 0, sim::BuildTier::Wood);
        const double wallY = gy * static_cast<double>(sim::kBuildCell);
        player.x = (gx + 0.5) * sim::kBuildCell;
        player.y = wallY - 40;
        const double startY = player.y;
        for (int i = 0; i < 180; ++i) {
            sim::PlayerInput push;
            push.moveY = 1;
            push.aim = 1.5707963;
            sim::stepPlayer(world, build, player, push, 1.0 / 60);
        }
        std::printf("wall: walked %.0f into it, stopped %.0f short, through it: %s\n",
                    player.y - startY, wallY - player.y, player.y > wallY ? "yes" : "no");
    }

    // A raid: a wall of stone, and a rocket into it.
    {
        sim::Explosives explosives;
        const int gx = 40;
        const int gy = 40;
        build.placeFoundation(gx, gy, 0, sim::BuildTier::Stone);
        build.placeFoundation(gx + 1, gy, 0, sim::BuildTier::Stone);
        const int wallA = build.placeEdge(gx, gy, sim::EdgeSide::North, sim::BuildKind::Wall, 0,
                                          sim::BuildTier::Stone).id;
        const int wallB = build.placeEdge(gx + 1, gy, sim::EdgeSide::North, sim::BuildKind::Wall, 0,
                                          sim::BuildTier::Stone).id;
        double x0 = 0;
        double y0 = 0;
        double x1 = 0;
        double y1 = 0;
        sim::edgeSegment(gx, gy, sim::EdgeSide::North, x0, y0, x1, y1);
        sim::Player raider;
        raider.x = (x0 + x1) * 0.5;
        raider.y = y0 - 200;
        sim::Inventory kit;
        int rockets = 0;
        bool downA = false;
        while (!downA && rockets < 8) {
            explosives.detonate(world, build, npcs, raider, kit, (x0 + x1) * 0.5, y0, 100, 275, wallA,
                                true);
            ++rockets;
            downA = true;
            for (const sim::Structure& piece : build.list()) {
                if (piece.id == wallA) downA = false;
            }
        }
        int neighbour = 0;
        for (const sim::Structure& piece : build.list()) {
            if (piece.id == wallB) neighbour = piece.hp;
        }
        std::printf("raid: %d rockets through a stone wall, the one beside it on %d\n", rockets,
                    neighbour);
    }

    // What was chopped, turned into a hatchet: queued, waited out, and in the
    // pack at the end of it.
    sim::Crafting crafting;
    // One tree is four wood short of a hatchet and there is no stone in it at
    // all, so the check tops up rather than felling another two.
    inventory.add(sim::ItemId::Wood, 10);
    inventory.add(sim::ItemId::Stone, 40);
    const sim::Recipe* hatchet = nullptr;
    for (const sim::Recipe& recipe : sim::recipes()) {
        if (recipe.out == sim::ItemId::Hatchet) hatchet = &recipe;
    }
    const bool queued = hatchet && crafting.queue(inventory, *hatchet, 0);
    for (int i = 0; i < 60 * 6; ++i) crafting.update(dt, inventory);
    std::printf("craft: %s, wood left %d, hatchets %d\n", queued ? "queued a hatchet" : "could not",
                inventory.count(sim::ItemId::Wood), inventory.count(sim::ItemId::Hatchet));

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
            sim::stepPlayer(world, build, player, input, dt);
            world.update(dt);
            const sim::NpcEvents events = npcs.update(world, build, projectiles, dt, player);
            player.health -= static_cast<int>(events.playerDamage);
            if (d <= 30 && sim::swing(world, npcs, player, inventory).hitNpc) ++hits;
        }
        // Everything it dropped, gathered up.
        for (int i = 0; i < 12; ++i) {
            if (!sim::pickUp(world, player, inventory).picked) break;
        }
    }

    // And a rifle, at something far enough off that a bullet has to fly.
    inventory.hotbar()[2] = sim::ItemStack{sim::ItemId::Ak47, 1};
    inventory.selectSlot(2);
    inventory.add(sim::ItemId::RifleAmmo, 60);
    sim::reload(player, inventory);
    for (int i = 0; i < 400; ++i) sim::tickReload(player, inventory, dt);

    const sim::Npc* mark = nullptr;
    double markD = 1e9;
    for (const sim::Npc& npc : npcs.list()) {
        if (npc.hp <= 0) continue;
        const double d = std::hypot(npc.x - player.x, npc.y - player.y);
        if (d < markD) {
            markD = d;
            mark = &npc;
        }
    }
    int shots = 0;
    int landed = 0;
    if (mark) {
        player.aim = std::atan2(mark->y - player.y, mark->x - player.x);
        for (int frame = 0; frame < 60 * 20; ++frame) {
            player.attackTimer = std::max(0.0, player.attackTimer - dt);
            const sim::FireResult shot = sim::fire(player, inventory, projectiles, true, dt);
            if (shot.fired) ++shots;
            for (const sim::BulletHit& hit : projectiles.update(world, npcs, build, dt, &player)) {
                if (hit.npc) ++landed;
            }
        }
    }
    std::printf("rifle: %d rounds out at %.0f away, %d into an animal, %d left in the gun\n", shots,
                markD, landed, player.rounds);

    std::printf("hunt: %s %s after %d hits, leather %d, meat %d, health %d\n", preyName,
                killed ? "killed" : "got away", hits, inventory.count(sim::ItemId::Leather),
                inventory.count(sim::ItemId::MeatRaw), static_cast<int>(player.health));

    // Written down and read back: the same island, and everything that has
    // happened to it.
    {
        sim::Session out;
        out.seed = 12345;
        out.clock = 1234.5;
        out.player = player;
        out.inventory = inventory;
        const std::string path = "/tmp/oxide-save-check";
        const bool wrote = sim::saveSession(path, out, world, build);
        sim::World back;
        sim::BuildSystem backBuild;
        sim::Session in;
        const bool readBack = sim::loadSession(path, in, back, backBuild);
        int felled = 0;
        for (const sim::ResourceNode& node : back.nodes()) {
            if (node.hp < node.maxHp || node.respawn > 0) ++felled;
        }
        std::printf("save: %s, %s, wood %d, pieces %zu, deployables %zu, worked nodes %d\n",
                    wrote ? "wrote" : "could not write", readBack ? "read back" : "would not read",
                    in.inventory.count(sim::ItemId::Wood), backBuild.list().size(),
                    backBuild.deployables().size(), felled);
    }

    std::printf("tree at %.0f, %.0f: %s after %d blows, wood %d, stone %d, drops %zu\n", target->x,
                target->y, felled ? "felled" : "still standing", blows,
                inventory.count(sim::ItemId::Wood), inventory.count(sim::ItemId::Stone),
                world.drops().size());
    return felled ? 0 : 1;
}
