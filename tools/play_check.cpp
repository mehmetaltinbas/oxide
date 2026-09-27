// Plays a minute of the game with nobody watching: walks to the nearest tree,
// swings until it falls, picks up what is lying about, and prints what ended
// up in the pack. A quick check that the rules still hold after a change.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "sim/features/combat/systems/action.hpp"
#include "sim/features/combat/systems/explosives.hpp"
#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/survival/systems/survival.hpp"
#include "sim/features/crafting/systems/crafting.hpp"
#include "sim/features/session/systems/save.hpp"
#include "sim/features/combat/systems/projectiles.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/building/types/deployable.struct.hpp"
#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/item-def.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"
#include "sim/features/monuments/constants/monument-defs.constant.hpp"
#include "sim/features/monuments/types/monument.struct.hpp"
#include "sim/features/survival/constants/player-rules.constant.hpp"
#include "sim/features/survival/types/player-input.struct.hpp"
#include "sim/features/survival/types/player.struct.hpp"
#include "sim/features/survival/utils/step-player.util.hpp"
#include "sim/features/wildlife/constants/npc-defs.constant.hpp"
#include "sim/features/wildlife/types/npc-kind.enum.hpp"
#include "sim/features/wildlife/types/npc-state.enum.hpp"
#include "sim/features/wildlife/types/npc.struct.hpp"
#include "sim/features/wildlife/utils/npc-traits.util.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"
#include "sim/shared/utils/health.util.hpp"
#include "sim/shared/utils/collide.util.hpp"
#include "sim/features/building/constants/deploy-footprint.constant.hpp"

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
			const sim::SwingResult blow = sim::swing(world, npcs, build, player, inventory);
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
		const int fireId = build.deploy(sim::DeployKind::Campfire, 2.5 * sim::kBuildCell,
										2.5 * sim::kBuildCell, false, 0);
		const int furnaceId = build.deploy(sim::DeployKind::Furnace, 4.5 * sim::kBuildCell,
										   2.5 * sim::kBuildCell, false, 0);
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
		for (int i = 0; i < 60 * 40; ++i) build.updateDeployables(world, dt);
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
			if (d <= 30 && sim::swing(world, npcs, build, player, inventory).hitNpc) ++hits;
		}
		// Everything it dropped, gathered up.
		for (int i = 0; i < 12; ++i) {
			if (!sim::pickUp(world, build, player, inventory).picked) break;
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
		for (int frame = 0; frame < 60 * 20; ++frame) {
			// Tracked rather than aimed once: the skittish animals run now, and
			// a check that fires at where a deer used to be proves nothing.
			player.aim = std::atan2(mark->y - player.y, mark->x - player.x);
			player.attackTimer = std::max(0.0, player.attackTimer - dt);
			const sim::FireResult shot = sim::fire(player, inventory, projectiles, true, false, dt);
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

	// Felled, it is gone: you can stand where it was, and it comes back a day
	// later, which is the same rule every resource on the island follows.
	bool walkedThrough = false;
	double grewBack = 0;
	if (felled) {
		sim::Player rambler;
		rambler.x = target->x - 60;
		rambler.y = target->y;
		for (int i = 0; i < 240; ++i) {
			sim::PlayerInput push;
			push.moveX = 1;
			push.aim = 0;
			sim::stepPlayer(world, build, rambler, push, dt);
		}
		walkedThrough = rambler.x > target->x + 10;

		// A day of island time, a minute at a go.
		for (int i = 0; i < 4200 && grewBack == 0; ++i) {
			world.update(1.0);
			const sim::ResourceNode* back = nullptr;
			for (const sim::ResourceNode& node : world.nodes()) {
				if (node.id == target->id) back = &node;
			}
			if (back && back->hp > 0) grewBack = i + 1;
		}
	}
	std::printf("regrowth: %s, back after %.0f minutes\n",
				walkedThrough ? "walked through where it stood" : "STILL IN THE WAY",
				grewBack / 60);

	std::printf("tree at %.0f, %.0f: %s after %d blows, wood %d, stone %d, drops %zu\n", target->x,
				target->y, felled ? "felled" : "still standing", blows,
				inventory.count(sim::ItemId::Wood), inventory.count(sim::ItemId::Stone),
				world.drops().size());
	// Every animal standing in a country it belongs to, which is the whole of
	// what "a deer lives in the trees" means once it is in the data.
	{
		sim::World island;
		island.generate(12345);
		sim::NpcSystem wild;
		wild.populate(island, 12345);
		int wrong = 0;
		int counts[sim::kNpcKindCount] = {};
		for (const sim::Npc& npc : wild.list()) {
			++counts[static_cast<int>(npc.kind)];
			if (!sim::livesIn(sim::npcDef(npc.kind), island.biomeAt(npc.x, npc.y))) ++wrong;
		}
		std::printf("wildlife:");
		for (int i = 0; i < 5; ++i) {
			std::printf(" %s %d", sim::npcDef(static_cast<sim::NpcKind>(i)).name, counts[i]);
		}
		std::printf(", %d in the wrong country\n", wrong);
	}

	// The food chain: a wolf put beside a deer goes for it, and the deer runs.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		sim::NpcSystem wild;
		wild.mutableList().clear();
		sim::Player watcher{};
		watcher.x = 10368;
		watcher.y = 10368;
		watcher.alive = true;
		const auto put = [&](sim::NpcKind kind, double x, double y) {
			sim::Npc npc{};
			npc.id = static_cast<int>(kind) + 1;
			npc.kind = kind;
			npc.x = x;
			npc.y = y;
			npc.hp = sim::npcDef(kind).hp;
			npc.homeX = x;
			npc.homeY = y;
			npc.leash = 4000;
			wild.mutableList().push_back(npc);
		};
		put(sim::NpcKind::Wolf, 10368 + 300, 10368);
		put(sim::NpcKind::Elk, 10368 + 460, 10368);
		const double gap0 = 160;
		bool chased = false;
		bool bled = false;
		for (int i = 0; i < 60 * 20; ++i) {
			wild.update(island, empty, none, dt, watcher);
			const sim::NpcState state = wild.list()[0].state;
			if (state == sim::NpcState::Chase || state == sim::NpcState::Attack) chased = true;
			if (wild.list().size() < 2 || wild.list()[1].hp < sim::npcDef(sim::NpcKind::Elk).hp) {
				bled = true;
			}
		}
		const sim::Npc& wolf = wild.list()[0];
		std::printf("chain: wolf %s the elk, %s it, gap %.0f from %.0f\n",
					chased ? "went for" : "IGNORED", bled ? "caught" : "never caught",
					wild.list().size() > 1
						? std::hypot(wild.list()[1].x - wolf.x, wild.list()[1].y - wolf.y)
						: 0.0,
					gap0);
	}

	// A shotgun fills shell by shell and a rifle fills all at once, and half a
	// shotgun reload leaves you with half a tube rather than with nothing.
	{
		sim::Player gunner{};
		sim::Inventory bag;
		bag.hotbar()[1] = sim::ItemStack{sim::ItemId::PumpShotgun, 1};
		bag.selectSlot(1);
		bag.add(sim::ItemId::ShotgunShell, 20);
		sim::reload(gunner, bag);
		// Long enough for two shells and no more.
		for (int i = 0; i < 90; ++i) sim::tickReload(gunner, bag, dt);
		const int partway = gunner.rounds;
		for (int i = 0; i < 600; ++i) sim::tickReload(gunner, bag, dt);
		const int full = gunner.rounds;

		sim::Player rifleman{};
		sim::Inventory kit;
		kit.hotbar()[1] = sim::ItemStack{sim::ItemId::Rifle, 1};
		kit.selectSlot(1);
		kit.add(sim::ItemId::RifleAmmo, 40);
		sim::reload(rifleman, kit);
		for (int i = 0; i < 90; ++i) sim::tickReload(rifleman, kit, dt);
		const int riflePartway = rifleman.rounds;
		for (int i = 0; i < 600; ++i) sim::tickReload(rifleman, kit, dt);
		std::printf("reload: shotgun %d after a second and a half then %d, rifle %d then %d\n",
					partway, full, riflePartway, rifleman.rounds);
	}

	// A rocket launcher loads and fires. Its own damage is nought, because the
	// rocket carries the blast, and every "is this a gun" test in the codebase
	// used to read that as "not a gun": it could not be loaded and would not
	// go off. Both halves are checked, because passing one and failing the
	// other is exactly what happened.
	{
		sim::Projectiles flying;
		sim::Player soldier{};
		soldier.x = 10368;
		soldier.y = 10368;
		soldier.alive = true;
		sim::Inventory tube;
		tube.hotbar()[1] = sim::ItemStack{sim::ItemId::RocketLauncher, 1};
		tube.selectSlot(1);
		tube.add(sim::ItemId::Rocket, 3);
		sim::reload(soldier, tube);
		for (int i = 0; i < 60 * 8; ++i) sim::tickReload(soldier, tube, dt);
		const int loaded = soldier.rounds;
		const sim::FireResult shot = sim::fire(soldier, tube, flying, true, false, dt);
		int away = 0;
		for (const sim::Bullet& b : flying.list()) {
			if (b.rocket) ++away;
		}
		std::printf("rocket: loaded %d of 1, fired %s, %d in the air\n", loaded,
					shot.fired ? "yes" : "NO", away);
		if (loaded < 1 || !shot.fired || away < 1) {
			std::printf("  ROCKET LAUNCHER IS NOT A GUN\n");
		}
	}

	// Being hit does not cost you what you were in the middle of. It used to
	// wipe the reload and the bandage, so the moment you most needed either
	// was the moment you could not finish one.
	{
		sim::Player bitten{};
		bitten.alive = true;
		bitten.health = 100;
		sim::Inventory kit;
		kit.hotbar()[1] = sim::ItemStack{sim::ItemId::Rifle, 1};
		kit.selectSlot(1);
		kit.add(sim::ItemId::RifleAmmo, 40);
		kit.add(sim::ItemId::Bandage, 1);
		sim::reload(bitten, kit);
		sim::consume(bitten, kit, sim::ItemId::Bandage);
		const double reloadWas = bitten.reloadLeft;
		const double useWas = bitten.useLeft;
		sim::hurtPlayer(bitten, kit, 20);
		std::printf("interrupt: after a bite, reload %.1f of %.1f, bandage %.1f of %.1f\n",
					bitten.reloadLeft, reloadWas, bitten.useLeft, useWas);
		if (bitten.reloadLeft <= 0 || bitten.useLeft <= 0) {
			std::printf("  A HIT CANCELLED IT\n");
		}

		// But putting the thing away does stop it, and from inside the action
		// rather than from whoever changed the slot.
		kit.selectSlot(0);
		sim::tickReload(bitten, kit, dt);
		sim::updateUse(bitten, kit, dt, false);
		std::printf("channel: after changing slot, reload %.1f, bandage %.1f\n",
					bitten.reloadLeft, bitten.useLeft);
		if (bitten.reloadLeft > 0 || bitten.useLeft > 0) {
			std::printf("  PUTTING IT AWAY DID NOT STOP IT\n");
		}
	}

	// A wound stops being advertised after a while, and the animal keeps the
	// health it had. Both halves: the bar goes, the hit points do not.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		sim::NpcSystem wild;
		wild.mutableList().clear();
		sim::Player away{};
		away.x = 0;
		away.y = 0;
		away.alive = true;
		sim::Npc elk{};
		elk.id = 1;
		elk.kind = sim::NpcKind::Elk;
		elk.x = 10368;
		elk.y = 10368;
		elk.hp = sim::npcDef(sim::NpcKind::Elk).hp;
		elk.homeX = elk.x;
		elk.homeY = elk.y;
		elk.leash = 400;
		wild.mutableList().push_back(elk);
		wild.hurt(island, wild.mutableList()[0], 30, elk.x - 10, elk.y);
		const int after = wild.list()[0].hp;
		const sim::NpcDef& elkDef = sim::npcDef(sim::NpcKind::Elk);
		const bool shownAtOnce =
			sim::showsHealth(wild.list()[0].hp, elkDef.hp, wild.list()[0].sinceHurt);
		// Past the rule itself, not past a number copied out of it: the rule
		// moved from one minute to fifteen and the checks went on waiting one.
		const int ticks = static_cast<int>((sim::kHealthShownFor + 1) / dt);
		for (int i = 0; i < ticks; ++i) wild.update(island, empty, none, dt, away);
		const bool shownLater =
			sim::showsHealth(wild.list()[0].hp, elkDef.hp, wild.list()[0].sinceHurt);
		std::printf("wound: elk on %d, bar at once %s, %.0f minutes later %s, still on %d\n",
					after, shownAtOnce ? "yes" : "NO", sim::kHealthShownFor / 60,
					shownLater ? "STILL" : "gone", wild.list()[0].hp);
		if (!shownAtOnce || shownLater || wild.list()[0].hp != after) {
			std::printf("  WOUND TIMEOUT IS WRONG\n");
		}
	}

	// A spear lands before the thing in front of you does. That is the only
	// reason to carry one, so it is asserted rather than assumed, against the
	// longest bite on the island as well as the commonest.
	{
		const double reach = sim::itemDef(sim::ItemId::Spear).melee.reach;
		// What NpcSystem::nearest measures: the gap to the animal's edge.
		const auto biteAt = [](sim::NpcKind kind) {
			const sim::NpcDef& def = sim::npcDef(kind);
			return def.attackRange + sim::PlayerRules::kRadius;
		};
		const double bear = biteAt(sim::NpcKind::Bear);
		const double wolf = biteAt(sim::NpcKind::Wolf);
		std::printf("spear: reach %.0f, bear bites at %.0f, wolf at %.0f\n", reach, bear, wolf);
		if (reach <= bear || reach <= wolf) std::printf("  SPEAR IS OUTREACHED\n");
	}

	// The same rule on the things that stand still. All four kinds of bar are
	// checked, because the rule used to live in four places and three of them
	// were missed.
	{
		sim::World island;
		island.generate(12345);
		int treeId = 0;
		for (const sim::ResourceNode& node : island.nodes()) {
			if (node.kind != sim::NodeKind::Tree) continue;
			treeId = node.id;
			break;
		}
		bool treeAtOnce = false;
		bool treeLater = true;
		if (sim::ResourceNode* tree = island.nodeById(treeId)) {
			island.hurtNode(*tree, 20);
			treeAtOnce = sim::showsHealth(*tree);
			const int ticks = static_cast<int>((sim::kHealthShownFor + 1) / dt);
			for (int i = 0; i < ticks; ++i) island.update(dt);
			treeLater = sim::showsHealth(*island.nodeById(treeId));
		}
		std::printf("wound: tree bar at once %s, %.0f minutes later %s\n",
					treeAtOnce ? "yes" : "NO", sim::kHealthShownFor / 60,
					treeLater ? "STILL" : "gone");
		if (!treeAtOnce || treeLater) std::printf("  NODE TIMEOUT IS WRONG\n");
	}

	// Nothing lands inside a wall. Dropped dead on top of a barrel, the stack
	// comes to rest outside it, and it does so because the world was wired to
	// the building system rather than because this caller remembered.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::settleDropsAgainst(island, empty);
		const sim::ResourceNode* barrel = nullptr;
		for (const sim::ResourceNode& node : island.nodes()) {
			if (node.kind != sim::NodeKind::Barrel) continue;
			barrel = &node;
			break;
		}
		double gap = -1;
		if (barrel) {
			const double bx = barrel->x;
			const double by = barrel->y;
			const double radius = sim::solidRadius(*barrel);
			island.dropStack(sim::ItemStack{sim::ItemId::Wood, 10}, bx, by);
			const sim::Dropped& put = island.drops().back();
			gap = std::hypot(put.x - bx, put.y - by) - radius;
		}
		std::printf("settle: a stack dropped on a barrel came to rest %.0f clear of its solid"
					" part\n", gap);
		if (gap < 0) std::printf("  A DROP LANDED INSIDE SOMETHING SOLID\n");
	}

	// Armour turns the same fraction of everything that hits you, whatever it
	// was: a bear, a bullet and a blast all come through hurtPlayer, and the
	// suit is applied there once rather than at each of them.
	{
		const double bite = 40;
		double bare = 0;
		double clad = 0;
		double heavy = 0;
		{
			sim::Player p{};
			p.alive = true;
			p.health = 100;
			sim::Inventory none;
			sim::hurtPlayer(p, none, bite);
			bare = 100 - p.health;
		}
		{
			sim::Player p{};
			p.alive = true;
			p.health = 100;
			sim::Inventory kit;
			kit.worn() = sim::ItemStack{sim::ItemId::MetalSuit, 1};
			sim::hurtPlayer(p, kit, bite);
			clad = 100 - p.health;
		}
		{
			sim::Player p{};
			p.alive = true;
			p.health = 100;
			sim::Inventory kit;
			kit.worn() = sim::ItemStack{sim::ItemId::HeavyMetalSuit, 1};
			sim::hurtPlayer(p, kit, bite);
			heavy = 100 - p.health;
		}
		const double metalArmour = sim::itemDef(sim::ItemId::MetalSuit).wear.armor;
		const double heavyArmour = sim::itemDef(sim::ItemId::HeavyMetalSuit).wear.armor;
		std::printf("armour: a %.0f blow takes %.0f bare, %.0f in metal (%.0f%%), %.0f in plate"
					" (%.0f%%)\n",
					bite, bare, clad, metalArmour * 100, heavy, heavyArmour * 100);
		if (std::abs(clad - bite * (1 - metalArmour)) > 0.01 ||
			std::abs(heavy - bite * (1 - heavyArmour)) > 0.01 || heavy >= clad || clad >= bare) {
			std::printf("  ARMOUR DOES NOT TURN WHAT IT SAYS\n");
		}
	}

	// An empty stomach is slower than an empty canteen, and both are slow.
	{
		const double food = sim::PlayerVitals::kStarveDamage;
		const double water = sim::PlayerVitals::kThirstDamage;
		std::printf("needs: starving %.2f/s (%.0fs to die), parched %.2f/s (%.0fs), both %.0fs\n",
					food, 100 / food, water, 100 / water, 100 / (food + water));
		if (water <= food) std::printf("  THIRST SHOULD BITE HARDER THAN HUNGER\n");
	}

	// A wall blown out leaves a hole you cannot plug at once, and a wall taken
	// down by hand does not. Both halves, because the point of the rule is the
	// difference between them.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem build;
		build.placeFoundation(40, 40, 0, sim::BuildTier::Wood);
		sim::Structure& blown = build.placeEdge(40, 40, sim::EdgeSide::North,
												sim::BuildKind::Wall, 0, sim::BuildTier::Wood);
		build.damage(blown, 9999, 0, 0, false, true);
		const bool blockedNow =
			build.refuseEdge(island, 40, 40, sim::EdgeSide::North, sim::BuildKind::Wall, 0) !=
			nullptr;
		for (int i = 0; i < 31 * 60; ++i) build.update(dt);
		const bool blockedLater =
			build.refuseEdge(island, 40, 40, sim::EdgeSide::North, sim::BuildKind::Wall, 0) !=
			nullptr;

		build.placeFoundation(44, 44, 0, sim::BuildTier::Wood);
		sim::Structure& chopped = build.placeEdge(44, 44, sim::EdgeSide::North,
												  sim::BuildKind::Wall, 0, sim::BuildTier::Wood);
		build.damage(chopped, 9999, 0, 0, true, false);
		const bool choppedBlocked =
			build.refuseEdge(island, 44, 44, sim::EdgeSide::North, sim::BuildKind::Wall, 0) !=
			nullptr;
		std::printf("scorch: blown %s at once, %s after thirty seconds; chopped %s\n",
					blockedNow ? "blocked" : "OPEN", blockedLater ? "STILL BLOCKED" : "open",
					choppedBlocked ? "BLOCKED" : "open");
		if (!blockedNow || blockedLater || choppedBlocked) {
			std::printf("  SCORCH RULE IS WRONG\n");
		}
	}

	// The power plant cannot be walked into without the suit, and can be with
	// it. Both halves: a monument nobody can enter is as broken as one anybody
	// can. Measured by standing in the worst of it until something gives.
	{
		const double rads = sim::monumentDef(sim::MonumentKind::PowerPlant).rads;
		const auto endure = [&](sim::ItemId suit) {
			sim::World island;
			island.generate(12345);
			sim::Player p{};
			p.alive = true;
			p.health = 100;
			p.calories = 100;
			p.hydration = 100;
			p.temperature = 20;
			sim::Inventory kit;
			if (suit != sim::ItemId::None) kit.worn() = sim::ItemStack{suit, 1};
			const double keptOut = sim::itemDef(suit).wear.radiation;
			double lived = 0;
			for (int i = 0; i < 60 * 300 && p.alive; ++i) {
				p.radiation = std::min(sim::PlayerVitals::kMaxRadiation,
									   p.radiation + rads * (1 - keptOut) * dt);
				double damage = 0;
				if (p.radiation > 45) {
					damage = sim::PlayerVitals::kRadDamage * ((p.radiation - 45) / 55);
				}
				p.health -= damage * dt;
				if (p.health <= 0) p.alive = false;
				lived += dt;
			}
			return lived;
		};
		const double bare = endure(sim::ItemId::None);
		const double suited = endure(sim::ItemId::RadSuit);
		std::printf("plant: %.0f rads a second, bare you last %.0fs, in the suit %.0fs\n", rads,
					bare, suited);
		if (bare > 30 || suited < 120) std::printf("  THE PLANT IS NOT A SUIT PROBLEM\n");
	}

	// A piece you have just built comes down with a hammer, and one you built
	// a quarter of an hour ago does not. Both halves: the window is the rule.
	{
		sim::BuildSystem build;
		build.placeFoundation(60, 60, 0, sim::BuildTier::Wood);
		sim::Structure& fresh = build.placeEdge(60, 60, sim::EdgeSide::North,
												sim::BuildKind::Wall, 0, sim::BuildTier::Wood);
		const int freshId = fresh.id;
		bool tookDown = false;
		for (sim::Structure& piece : build.mutableList()) {
			if (piece.id != freshId) continue;
			tookDown = build.demolish(piece, 0);
			break;
		}

		build.placeFoundation(64, 64, 0, sim::BuildTier::Wood);
		const int oldId = build.placeEdge(64, 64, sim::EdgeSide::North, sim::BuildKind::Wall, 0,
										  sim::BuildTier::Wood)
							  .id;
		const double window = sim::BuildSystem::kFreeDemolishSeconds;
		const int ticks = static_cast<int>((window + 1) / dt);
		for (int i = 0; i < ticks; ++i) build.update(dt);
		bool tookOld = true;
		for (sim::Structure& piece : build.mutableList()) {
			if (piece.id != oldId) continue;
			tookOld = build.demolish(piece, 0);
			break;
		}
		std::printf("hammer: a new wall comes down %s, one %.0f minutes old %s\n",
					tookDown ? "yes" : "NO", window / 60, tookOld ? "TOO" : "does not");
		if (!tookDown || tookOld) std::printf("  THE DEMOLISH WINDOW IS WRONG\n");
	}

	// A tool takes twig apart and barely marks stone. Both ends, because the
	// whole point of the tier is the difference between them.
	{
		sim::BuildSystem build;
		const double swing = sim::itemDef(sim::ItemId::Hatchet).melee.damage;
		// On a floor, which has no soft side: this is about the tier and
		// nothing else, and a wall would fold the hard-side rule in with it.
		int cell = 80;
		const auto bite = [&](sim::BuildTier tier) {
			sim::Structure& floor = build.placeFoundation(cell, cell, 0, tier);
			cell += 4;
			const int before = floor.hp;
			build.damage(floor, swing, 0, 0, true);
			const int took = before - floor.hp;
			return 100.0 * took / std::max(1, before);
		};
		const double twig = bite(sim::BuildTier::Twig);
		const double stone = bite(sim::BuildTier::Stone);
		std::printf("chop: a hatchet takes %.0f%% off twig and %.1f%% off stone\n", twig, stone);
		if (twig < 90 || stone > 2) std::printf("  THE TIERS DO NOT HOLD\n");
	}

	// Things take the floor their footprint says, two small ones share a cell,
	// and turning one swaps which way it is long.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem build;
		const double base = 120.0 * sim::kBuildCell;
		island.clearNaturalIn(base - 400, base - 400, base + 400, base + 400);

		double wide = 0;
		double deep = 0;
		sim::BuildSystem::deployBounds(sim::DeployKind::LargeBox, false, 0, 0, wide, deep);
		double turnedWide = 0;
		double turnedDeep = 0;
		sim::BuildSystem::deployBounds(sim::DeployKind::LargeBox, true, 0, 0, turnedWide,
									   turnedDeep);

		// Two small boxes side by side inside one building cell.
		double ax = base + sim::kDeployCell * 2;
		double ay = base;
		sim::BuildSystem::snapDeploy(sim::DeployKind::WoodenBox, false, ax, ay);
		build.deploy(sim::DeployKind::WoodenBox, ax, ay, false, 0);
		double bx = base + sim::kDeployCell * 4.2;
		double by = base;
		sim::BuildSystem::snapDeploy(sim::DeployKind::WoodenBox, false, bx, by);
		const bool room =
			build.refuseDeploy(island, bx, by, sim::DeployKind::WoodenBox, false, 0) == nullptr;
		// And one on top of the first, which is not allowed.
		const bool onTop =
			build.refuseDeploy(island, ax, ay, sim::DeployKind::WoodenBox, false, 0) != nullptr;

		std::printf("footprint: large box %.0f by %.0f, turned %.0f by %.0f; two small in a cell"
					" %s, stacked %s\n",
					wide * 2, deep * 2, turnedWide * 2, turnedDeep * 2, room ? "fits" : "NO",
					onTop ? "refused" : "ALLOWED");
		if (!room || !onTop || turnedWide != deep || turnedDeep != wide) {
			std::printf("  FOOTPRINTS ARE WRONG\n");
		}
	}

	// A fire mends you out of your own surplus, and only out of the surplus.
	// Three halves: it heals, it eats, and it does neither on an empty
	// stomach.
	{
		sim::World island;
		island.generate(12345);
		sim::Inventory kit;
		const auto sit = [&](double food, double comfort) {
			sim::Player p{};
			p.alive = true;
			p.health = 50;
			p.calories = food;
			p.hydration = food;
			p.temperature = 20;
			p.x = 10368;
			p.y = 10368;
			for (int i = 0; i < 60; ++i) {
				sim::updateSurvival(island, p, kit, dt, 0, 0, comfort);
			}
			return p;
		};
		const sim::Player alone = sit(180, sim::PlayerVitals::kComfortPerHead);
		const sim::Player four = sit(180, 1.0);
		const sim::Player hungry = sit(60, 1.0);
		std::printf("comfort: a second at a fire heals %.2f alone and %.2f with four, and %.2f"
					" on an empty stomach\n",
					alone.health - 50, four.health - 50, hungry.health - 50);
		std::printf("         four heads cost %.2f food in that second\n", 180 - four.calories);
		if (four.health - 50 < alone.health - 50 || hungry.health - 50 > 0.7 ||
			180 - four.calories < 0.9) {
			std::printf("  COMFORT IS WRONG\n");
		}
	}

	// Prey outruns you and a wolf still eats. Both halves, because the second
	// is what the burst is carefully not: a rabbit bolts from a person faster
	// than you can sprint, and an elk running from a wolf runs at its own
	// speed so the wolf can catch it.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		sim::NpcSystem wild;
		wild.mutableList().clear();
		sim::Player chaser{};
		chaser.x = 10368;
		chaser.y = 10368;
		chaser.alive = true;
		sim::Npc rabbit{};
		rabbit.id = 1;
		rabbit.kind = sim::NpcKind::Rabbit;
		rabbit.x = chaser.x + 90;
		rabbit.y = chaser.y;
		rabbit.hp = sim::npcDef(sim::NpcKind::Rabbit).hp;
		rabbit.homeX = rabbit.x;
		rabbit.homeY = rabbit.y;
		rabbit.leash = 6000;
		wild.mutableList().push_back(rabbit);
		sim::PlayerInput after;
		after.moveX = 1;
		after.sprint = true;
		for (int i = 0; i < 60 * 6; ++i) {
			island.clearNaturalIn(chaser.x - 300, chaser.y - 300, chaser.x + 1600,
								  chaser.y + 300);
			sim::stepPlayer(island, empty, chaser, after, dt);
			wild.update(island, empty, none, dt, chaser);
		}
		const double gap = wild.list()[0].x - chaser.x;
		std::printf("bolt: sprinting after a rabbit for six seconds, gap %.0f from 90\n", gap);
		if (gap <= 90) std::printf("  YOU CAN RUN A RABBIT DOWN\n");
	}

	// A wolf comes at you and an elk runs from you. Both directions, because
	// "skittish" once meant both "bolts from a bear" and "bolts from you", and
	// the wolf read the second one.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		const auto towards = [&](sim::NpcKind kind) {
			sim::NpcSystem wild;
			wild.mutableList().clear();
			sim::Player you{};
			you.x = 10368;
			you.y = 10368;
			you.alive = true;
			sim::Npc npc{};
			npc.id = 1;
			npc.kind = kind;
			npc.x = you.x + 120;
			npc.y = you.y;
			npc.hp = sim::npcDef(kind).hp;
			npc.homeX = npc.x;
			npc.homeY = npc.y;
			npc.leash = 4000;
			wild.mutableList().push_back(npc);
			for (int i = 0; i < 60 * 4; ++i) wild.update(island, empty, none, dt, you);
			// Negative means it closed on you, positive means it ran.
			return std::hypot(wild.list()[0].x - you.x, wild.list()[0].y - you.y) - 120;
		};
		const double wolf = towards(sim::NpcKind::Wolf);
		const double elk = towards(sim::NpcKind::Elk);
		std::printf("temper: wolf %s you by %.0f, elk %s you by %.0f\n",
					wolf < 0 ? "closed on" : "RAN FROM", std::abs(wolf),
					elk > 0 ? "ran from" : "CLOSED ON", std::abs(elk));
	}

	// A sprint gets you away from a wolf, and only just. Both halves matter:
	// under a sprint or a wolf is a death sentence, over an elk or it starves.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		sim::NpcSystem wild;
		wild.mutableList().clear();
		sim::Player runner{};
		runner.x = 10368;
		runner.y = 10368;
		runner.alive = true;
		sim::Npc wolf{};
		wolf.id = 1;
		wolf.kind = sim::NpcKind::Wolf;
		wolf.x = runner.x - 90;
		wolf.y = runner.y;
		wolf.hp = sim::npcDef(sim::NpcKind::Wolf).hp;
		wolf.homeX = wolf.x;
		wolf.homeY = wolf.y;
		wolf.leash = 6000;
		wild.mutableList().push_back(wolf);
		sim::PlayerInput away;
		away.moveX = 1;
		away.sprint = true;
		away.aim = 0;
		for (int i = 0; i < 60 * 10; ++i) {
			// Open ground, so this measures speed and not a tree in the way.
			island.clearNaturalIn(runner.x - 300, runner.y - 300, runner.x + 900,
								  runner.y + 300);
			sim::stepPlayer(island, empty, runner, away, dt);
			wild.update(island, empty, none, dt, runner);
		}
		const double gap = runner.x - wild.list()[0].x;
		std::printf("outrun: sprinting away from a wolf for ten seconds, gap %.0f from 90\n",
					gap);
		std::printf("speeds: sprint %.0f, wolf %.0f, bear %.0f, elk %.0f, rabbit %.0f\n",
					sim::PlayerRules::kSpeed * sim::PlayerRules::kSprint,
					sim::npcDef(sim::NpcKind::Wolf).speed, sim::npcDef(sim::NpcKind::Bear).speed,
					sim::npcDef(sim::NpcKind::Elk).speed,
					sim::npcDef(sim::NpcKind::Rabbit).speed);
	}

	// Something startled keeps running. Standing still after it has bolted,
	// the rabbit must put real ground between itself and you rather than
	// stopping dead the moment it is one unit outside the startle range.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		sim::NpcSystem wild;
		wild.mutableList().clear();
		sim::Player watcher{};
		watcher.x = 10368;
		watcher.y = 10368;
		watcher.alive = true;
		sim::Npc rabbit{};
		rabbit.id = 1;
		rabbit.kind = sim::NpcKind::Rabbit;
		rabbit.x = watcher.x + 120;
		rabbit.y = watcher.y;
		rabbit.hp = sim::npcDef(sim::NpcKind::Rabbit).hp;
		rabbit.homeX = rabbit.x;
		rabbit.homeY = rabbit.y;
		rabbit.leash = 400;
		wild.mutableList().push_back(rabbit);
		double atRange = 0;
		for (int i = 0; i < 60 * 8; ++i) {
			island.clearNaturalIn(watcher.x - 300, watcher.y - 300, watcher.x + 1400,
								  watcher.y + 300);
			wild.update(island, empty, none, dt, watcher);
			const double d = std::hypot(wild.list()[0].x - watcher.x,
										wild.list()[0].y - watcher.y);
			// Where it was as it left the range it was startled inside.
			if (atRange == 0 && d > 155) atRange = d;
		}
		const double gone = std::hypot(wild.list()[0].x - watcher.x,
									   wild.list()[0].y - watcher.y);
		std::printf("bolt: startled at 120, still running at %.0f, eight seconds later %.0f\n",
					atRange, gone);
		if (gone < 600) std::printf("  BOLT STOPPED SHORT\n");
	}

	// Nothing walks through a barrel, whoever it is: the one collision routine.
	{
		sim::World island;
		island.generate(12345);
		sim::BuildSystem empty;
		sim::Projectiles none;
		const sim::ResourceNode* wall = nullptr;
		for (const sim::ResourceNode& node : island.nodes()) {
			if (node.kind != sim::NodeKind::Barrel) continue;
			wall = &node;
			break;
		}
		double through = -1;
		if (wall) {
			sim::NpcSystem wild;
			wild.mutableList().clear();
			sim::Npc npc{};
			npc.id = 1;
			npc.kind = sim::NpcKind::Bear;
			npc.x = wall->x - 90;
			npc.y = wall->y;
			npc.hp = sim::npcDef(npc.kind).hp;
			npc.homeX = npc.x;
			npc.homeY = npc.y;
			npc.leash = 4000;
			wild.mutableList().push_back(npc);
			sim::Player bait{};
			bait.x = wall->x + 90;
			bait.y = wall->y;
			bait.alive = true;
			for (int i = 0; i < 60 * 8; ++i) wild.update(island, empty, none, dt, bait);
			through = wild.list()[0].x - wall->x;
		}
		std::printf("collide: a bear walking at a barrel stopped %.0f short of its middle\n",
					-through);
	}

	return felled ? 0 : 1;
}
