#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "client/features/creatures/draw/animal.hpp"
#include "client/features/audio/systems/audio.hpp"
#include "client/features/building/draw/structure.hpp"
#include "client/features/building/draw/deployable.hpp"
#include "client/features/building/draw/monument.hpp"
#include "client/features/items/draw/item-glyph.hpp"
#include "client/features/render/systems/night.hpp"
#include "client/features/ui/systems/hud.hpp"
#include "client/features/ui/systems/map-screen.hpp"
#include "client/features/net/systems/net-client.hpp"
#include "client/features/ui/systems/panel.hpp"
#include "client/features/creatures/draw/human.hpp"
#include "client/features/render/systems/paint.hpp"
#include "client/features/render/systems/particles.hpp"
#include "client/features/render/systems/text.hpp"
#include "client/design/tokens/world.tokens.hpp"
#include "sim/features/combat/systems/action.hpp"
#include "sim/features/combat/systems/explosives.hpp"
#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/crafting/systems/crafting.hpp"
#include "sim/features/session/systems/save.hpp"
#include "sim/features/world/constants/daylight.constant.hpp"
#include "sim/features/building/constants/deployable-defs.constant.hpp"
#include "sim/features/building/types/container.struct.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/building/types/deployable.struct.hpp"
#include "sim/features/survival/systems/survival.hpp"
#include "sim/features/items/systems/inventory.hpp"
#include "sim/features/wildlife/systems/npc-system.hpp"
#include "sim/features/combat/systems/projectiles.hpp"
#include "sim/features/survival/constants/player-rules.constant.hpp"
#include "sim/features/survival/types/player-input.struct.hpp"
#include "sim/features/survival/types/player.struct.hpp"
#include "sim/features/survival/utils/step-player.util.hpp"
#include "sim/features/world/systems/world.hpp"
#include "client/features/render/systems/sprites.hpp"
#include "client/features/render/systems/terrain.hpp"
#include "sim/features/items/constants/bow-draw-seconds.constant.hpp"
#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/cost.struct.hpp"
#include "sim/features/items/types/gun.struct.hpp"
#include "sim/features/items/types/item-category.enum.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"
#include "sim/features/monuments/constants/monument-placement.constant.hpp"
#include "sim/features/monuments/types/loot-crate.struct.hpp"
#include "sim/features/monuments/types/monument.struct.hpp"
#include "sim/features/net/types/protocol.struct.hpp"
#include "sim/features/wildlife/constants/npc-defs.constant.hpp"
#include "sim/features/wildlife/types/npc-def.struct.hpp"
#include "sim/features/wildlife/types/npc-kind.enum.hpp"
#include "sim/features/wildlife/types/npc-state.enum.hpp"
#include "sim/features/wildlife/types/npc.struct.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/world/types/dropped.struct.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/audio/constants/gun-sounds.constant.hpp"
#include "client/features/audio/types/gun-sound.struct.hpp"
#include "client/features/net/types/room-entry.struct.hpp"
#include "client/features/render/types/align.enum.hpp"
#include "client/features/render/types/face.enum.hpp"
#include "client/features/render/types/point.struct.hpp"
#include "client/features/render/utils/shape-points.util.hpp"
#include "client/features/ui/types/item-actions.struct.hpp"
#include "client/features/ui/types/tab.enum.hpp"
#include "sim/features/items/utils/is-ranged.util.hpp"
#include "sim/shared/utils/health.util.hpp"
#include "sim/shared/utils/collide.util.hpp"
#include "client/features/ui/systems/wheel.hpp"
#include "client/features/building/draw/build-plan-options.util.hpp"
#include "client/features/building/draw/tier-options.util.hpp"
#include "client/design/tokens/interface.tokens.hpp"
#include "sim/features/building/constants/deploy-footprint.constant.hpp"

/**
 * The game as you play it: an island from a seed, someone standing on it, and
 * a camera that follows them.
 *
 * Every rule here comes from oxide_sim, which the server runs too, so the two
 * cannot drift. What this file owns is the window, the input and the order
 * things are painted in.
 */
namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
/**
 * The shape and the size of what anybody can see.
 *
 * Sixteen by nine, and a rectangle of island 1280 by 720 units across. Every
 * window is fitted to this: a bigger one draws the same ground larger, and a
 * wider one has the extra covered over. Nobody sees further than anybody else
 * because their monitor is a different shape. See docs/systems/fair-view.md.
 */
constexpr double kViewAspect = 16.0 / 9.0;
constexpr double kViewArea = 1280.0 * 720.0;

constexpr double kZoomMin = 0.80;
constexpr double kZoomMax = 1.8;

/** Where you wake up: on a beach, as in the other game. */
void dropIn(const sim::World& world, sim::Player& player, std::uint32_t roll) {
	world.beachSpawn(roll, player.x, player.y);
	player.alive = true;
	player.health = sim::PlayerVitals::kMaxHealth;
	// You wake half fed and half watered, as the other game had it.
	player.calories = sim::PlayerVitals::kMaxCalories * 0.5;
	player.hydration = sim::PlayerVitals::kMaxHydration * 0.5;
	player.temperature = sim::PlayerVitals::kComfortTemp;
	player.radiation = 0;
	player.bleeding = 0;
	player.healOverTime = 0;
	player.attackTimer = 0;
	player.swingAnim = 0;
	player.applying = sim::ItemId::None;
	player.useLeft = 0;
}

}  // namespace

int main(int argc, char** argv) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::fprintf(stderr, "SDL would not start: %s\n", SDL_GetError());
		return 1;
	}

	// Read before the window is made, because one of them says how big it is.
	int winW = kWindowWidth;
	int winH = kWindowHeight;
	for (int i = 1; i < argc; ++i) {
		if (SDL_strcmp(argv[i], "--window") == 0 && i + 2 < argc) {
			winW = SDL_atoi(argv[i + 1]);
			winH = SDL_atoi(argv[i + 2]);
		}
	}

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	if (!SDL_CreateWindowAndRenderer("Oxide", winW, winH,
									 SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window,
									 &renderer)) {
		std::fprintf(stderr, "No window: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	// Wait for the display rather than spinning: the old game capped itself at
	// 120 by hand because the browser gave it no say in this.
	SDL_SetRenderVSync(renderer, 1);

	// The lettering: the comic faces the browser game used, bundled here.
	client::Text lettering;
	{
		const char* base = SDL_GetBasePath();
		const std::string here = base ? base : "";
		// Beside the binary first, which is how it ships, then the places it
		// sits while being worked on.
		if (!lettering.open(renderer, here + "assets/fonts") &&
			!lettering.open(renderer, here + "../assets/fonts") &&
			!lettering.open(renderer, "assets/fonts") &&
			!lettering.open(renderer, "native/assets/fonts")) {
			std::printf("No fonts: %s\n", SDL_GetError());
		}
	}


	/**
	 * Which island.
	 *
	 * A new one every launch, so running the game twice is two islands rather
	 * than the same one with your old footprints on it. A number on the
	 * command line pins it, which is what every check and every screenshot in
	 * this repository relies on; the title screen's "carry on" takes the seed
	 * out of the save.
	 */
	std::uint32_t seed = static_cast<std::uint32_t>(SDL_GetPerformanceCounter());
	// A frame straight to a file and then out again, so the look of the game
	// can be checked without a pair of eyes at the window.
	const char* shotPath = nullptr;
	double startX = -1;
	double startY = -1;
	int benchFrames = 0;
	// A swing frozen part way through, for checking how a blow is drawn.
	double poseSwing = -1;
	/** A bow frozen part way through its draw, for checking how it is held. */
	double poseDraw = -1;
	/** Armed to the teeth, for trying the guns before crafting exists. */
	bool armed = false;
	/** The pack open with something in it, for a look at the screen itself. */
	bool showPanel = false;
	bool showCraft = false;
	/** The view's scale to open at, for looking at how the ink scales. */
	double startZoom = 1.0;
	/** One of every animal in a row, for looking at how they are drawn. */
	bool zoo = false;
	/** One of every node kind in a row, for looking at how they are drawn. */
	bool rocks = false;
	/** How far through a reload to freeze, for looking at the hands. */
	double reloadAt = -1;
	/** Every item picture in a grid, for looking at them all at once. */
	bool icons = false;
	/** An arrow and a rifle round frozen in the air, for looking at them. */
	bool arrowShot = false;
	/** Which belt slot to start holding, for looking at a tool in a hand. */
	int holdSlot = -1;
	/** Seconds of bleeding to start with, for a look at the readout. */
	double bleedFor = 0;
	/** The building plan's ring, held open for a screenshot. */
	bool showWheel = false;
	/** Something worn from the off, for a screenshot. */
	sim::ItemId wearWhat = sim::ItemId::None;
	/** Which crafting category to open, for a screenshot. */
	int craftCategory = -1;
	/** A queued job picked up and held over another row, for a screenshot. */
	int dragFrom = -1;
	int dragOver = 0;
	/** Seconds left of the note that says multiplayer is not here yet. */
	double titleNotice = 0;
	/** The island's own map, opened for a look at it. */
	bool showMap = false;
	bool showTitle = false;
	/** Dropped in at midnight, for a look at the dark. */
	bool startAtNight = false;
	/** Nothing to worry about and everything to hand, for trying things out. */
	bool sandbox = false;
	/** Kills you, waits, presses the key, and says whether you woke up. */
	bool deathTest = false;
	/** A couple of floating numbers, for a look at how they read. */
	bool showPopups = false;
	/** Everything round you felled at once, for a look at the empty ground. */
	bool clearAround = false;
	/** Where to play: nowhere is this machine, a host is somebody's island. */
	std::string connectTo;
	std::string playerName = "survivor";
	/** Which island to land on: a number, a new one, or whatever is running. */
	int joinRoom = 0;
	bool newRoom = false;
	/** A small base put up where you stand, for a look at what one looks like. */
	bool showBase = false;
	/** Dropped in at the nth looting place, for a look at one. */
	int atMonument = -1;
	for (int i = 1; i < argc; ++i) {
		if (SDL_strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
			shotPath = argv[++i];
		} else if (SDL_strcmp(argv[i], "--at") == 0 && i + 2 < argc) {
			startX = SDL_atof(argv[++i]);
			startY = SDL_atof(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--monument") == 0 && i + 1 < argc) {
			atMonument = SDL_atoi(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--base") == 0) {
			showBase = true;
		} else if (SDL_strcmp(argv[i], "--connect") == 0 && i + 1 < argc) {
			connectTo = argv[++i];
		} else if (SDL_strcmp(argv[i], "--island") == 0 && i + 1 < argc) {
			joinRoom = SDL_atoi(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--new-island") == 0) {
			newRoom = true;
		} else if (SDL_strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
			playerName = argv[++i];
		} else if (SDL_strcmp(argv[i], "--draw") == 0 && i + 1 < argc) {
			poseDraw = SDL_atof(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--sandbox") == 0) {
			sandbox = true;
		} else if (SDL_strcmp(argv[i], "--cleared") == 0) {
			clearAround = true;
		} else if (SDL_strcmp(argv[i], "--popups") == 0) {
			showPopups = true;
		} else if (SDL_strcmp(argv[i], "--death-test") == 0) {
			deathTest = true;
		} else if (SDL_strcmp(argv[i], "--night") == 0) {
			startAtNight = true;
		} else if (SDL_strcmp(argv[i], "--title") == 0) {
			showTitle = true;
		} else if (SDL_strcmp(argv[i], "--map") == 0) {
			showMap = true;
		} else if (SDL_strcmp(argv[i], "--panel") == 0) {
			showPanel = true;
		} else if (SDL_strcmp(argv[i], "--window") == 0 && i + 2 < argc) {
			// Read already, before the window was made.
			i += 2;
		} else if (SDL_strcmp(argv[i], "--hold") == 0 && i + 1 < argc) {
			holdSlot = SDL_atoi(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--arrow") == 0) {
			arrowShot = true;
		} else if (SDL_strcmp(argv[i], "--icons") == 0) {
			icons = true;
		} else if (SDL_strcmp(argv[i], "--dragqueue") == 0 && i + 2 < argc) {
			dragFrom = SDL_atoi(argv[++i]);
			dragOver = SDL_atoi(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--wheel") == 0) {
			showWheel = true;
		} else if (SDL_strcmp(argv[i], "--wear") == 0 && i + 1 < argc) {
			const char* what = argv[++i];
			wearWhat = SDL_strcmp(what, "rad") == 0     ? sim::ItemId::RadSuit
					   : SDL_strcmp(what, "heavy") == 0 ? sim::ItemId::HeavyMetalSuit
					   : SDL_strcmp(what, "metal") == 0 ? sim::ItemId::MetalSuit
														: sim::ItemId::Clothing;
		} else if (SDL_strcmp(argv[i], "--category") == 0 && i + 1 < argc) {
			craftCategory = SDL_atoi(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--bleeding") == 0 && i + 1 < argc) {
			bleedFor = SDL_atof(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--reload") == 0 && i + 1 < argc) {
			reloadAt = SDL_atof(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--rocks") == 0) {
			rocks = true;
		} else if (SDL_strcmp(argv[i], "--zoo") == 0) {
			zoo = true;
		} else if (SDL_strcmp(argv[i], "--zoom") == 0 && i + 1 < argc) {
			startZoom = SDL_atof(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--craft") == 0) {
			showPanel = true;
			showCraft = true;
		} else if (SDL_strcmp(argv[i], "--armed") == 0) {
			armed = true;
		} else if (SDL_strcmp(argv[i], "--swing") == 0 && i + 1 < argc) {
			poseSwing = SDL_atof(argv[++i]);
		} else if (SDL_strcmp(argv[i], "--bench") == 0 && i + 1 < argc) {
			// Frames drawn as fast as the machine will draw them, then the time
			// each one took: the number the whole rewrite is being judged on.
			benchFrames = SDL_atoi(argv[++i]);
		} else {
			seed = static_cast<std::uint32_t>(std::strtoul(argv[i], nullptr, 10));
		}
	}
	// Online, the island is whichever one you land on: the seed comes down the
	// wire and the same generator builds it here.
	sim::BuildSystem build;
	client::NetClient net;
	bool online = false;
	if (!connectTo.empty()) {
		std::string host = connectTo;
		std::uint16_t port = sim::net::kDefaultPort;
		const std::size_t colon = host.find(':');
		if (colon != std::string::npos) {
			port = static_cast<std::uint16_t>(SDL_atoi(host.c_str() + colon + 1));
			host = host.substr(0, colon);
		}
		std::string problem;
		if (!net.connect(host, port, playerName, problem)) {
			std::fprintf(stderr, "Could not reach %s: %s\n", connectTo.c_str(), problem.c_str());
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 1;
		}
		// The lobby: what islands are running, and which one to land on.
		std::printf("Islands on %s:\n", connectTo.c_str());
		for (const client::RoomEntry& room : net.rooms()) {
			std::printf("  %u  %s  (%d of %d)\n", room.id, room.name.c_str(), room.players,
						room.max);
		}
		if (joinRoom > 0) {
			net.joinRoom(static_cast<std::uint16_t>(joinRoom));
		} else if (newRoom) {
			net.createRoom(playerName + "'s island", 0);
		} else {
			// The lobby, on screen: every island the server is running, and
			// the choice of landing on one or starting another.
			bool choosing = true;
			while (choosing) {
				SDL_Event lobbyEvent;
				while (SDL_PollEvent(&lobbyEvent)) {
					if (lobbyEvent.type == SDL_EVENT_QUIT) {
						SDL_DestroyRenderer(renderer);
						SDL_DestroyWindow(window);
						SDL_Quit();
						return 0;
					}
					if (lobbyEvent.type != SDL_EVENT_KEY_DOWN || lobbyEvent.key.repeat) continue;
					if (lobbyEvent.key.key == SDLK_ESCAPE) {
						SDL_DestroyRenderer(renderer);
						SDL_DestroyWindow(window);
						SDL_Quit();
						return 0;
					}
					if (lobbyEvent.key.key == SDLK_N) {
						net.createRoom(playerName + "'s island", 0);
						choosing = false;
					}
					if (lobbyEvent.key.key >= SDLK_1 && lobbyEvent.key.key <= SDLK_9) {
						const std::size_t at =
							static_cast<std::size_t>(lobbyEvent.key.key - SDLK_1);
						if (at < net.rooms().size()) {
							net.joinRoom(net.rooms()[at].id);
							choosing = false;
						}
					}
				}

				int lobbyW = 0;
				int lobbyH = 0;
				SDL_GetRenderOutputSize(renderer, &lobbyW, &lobbyH);
				int windowW = 0;
				int windowH = 0;
				SDL_GetWindowSize(window, &windowW, &windowH);
				const float ui = windowW > 0 ? static_cast<float>(lobbyW) / windowW : 1.0f;
				SDL_SetRenderDrawColor(renderer, 10, 12, 10, 255);
				SDL_RenderClear(renderer);
				lettering.draw("Islands", lobbyW * 0.5f, lobbyH * 0.2f, 54 * ui,
							   client::rgb(0xefeadd), client::Face::Display,
							   client::Align::Centre);
				float row = lobbyH * 0.32f;
				for (std::size_t i = 0; i < net.rooms().size() && i < 9; ++i) {
					const client::RoomEntry& room = net.rooms()[i];
					char line[96];
					SDL_snprintf(line, sizeof(line), "%zu   %s   %d of %d", i + 1,
								 room.name.c_str(), room.players, room.max);
					lettering.draw(line, lobbyW * 0.5f - 200 * ui, row, 18 * ui,
								   client::rgb(0xefeadd));
					row += 32 * ui;
				}
				lettering.draw("N   start a new island", lobbyW * 0.5f - 200 * ui, row + 20 * ui,
							   18 * ui, client::Color{160, 160, 150, 255});
				lettering.draw("ESC   leave", lobbyW * 0.5f - 200 * ui, row + 52 * ui, 18 * ui,
							   client::Color{160, 160, 150, 255});
				SDL_RenderPresent(renderer);
				// The list keeps up with whoever is joining while you decide.
				net.poll(build, 1.0 / 30);
			}
		}
		if (!net.waitForWelcome(5.0)) {
			const std::string refused = net.takeRefusal();
			std::fprintf(stderr, "Could not join: %s\n",
						 refused.empty() ? "the server never let us on" : refused.c_str());
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 1;
		}
		online = true;
		seed = net.seed();
		std::printf("Joined as %u on island %u\n", net.id(), seed);
	}

	// Where a single-player island is kept between sittings.
	const char* home = SDL_getenv("HOME");
	const std::string savePath =
		std::string(home ? home : ".") + "/.oxide-save";

	sim::World world;
	const std::uint64_t built = SDL_GetTicks();
	world.generate(seed);
	std::printf("Oxide: island %u built in %llu ms, %zu things standing on it\n", seed,
				static_cast<unsigned long long>(SDL_GetTicks() - built), world.nodes().size());

	sim::Player player;
	dropIn(world, player, seed);
	if (online) {
		player.x = net.startX();
		player.y = net.startY();
	}
	if (atMonument >= 0 && atMonument < static_cast<int>(world.monuments().size())) {
		player.x = world.monuments()[atMonument].x;
		player.y = world.monuments()[atMonument].y;
	}
	if (startX >= 0) {
		player.x = startX;
		player.y = startY;
	}

	// Nothing dropped lands inside a wall or under a boulder: the world is
	// told once what is solid and settles every drop itself.
	sim::settleDropsAgainst(world, build);


	// Nothing grows back up through a floor: the world asks the building
	// system before it puts anything back.
	world.setRegrowthBlocked(&build, [](const void* owner, double x, double y) {
		const auto* build = static_cast<const sim::BuildSystem*>(owner);
		const int gx = static_cast<int>(std::floor(x / sim::kBuildCell));
		const int gy = static_cast<int>(std::floor(y / sim::kBuildCell));
		if (build->foundationAt(gx, gy)) return true;
		return const_cast<sim::BuildSystem*>(build)->deployableNear(x, y, sim::kDeployHalf * 2) !=
			   nullptr;
	});

	sim::NpcSystem npcs;
	npcs.populate(world, seed);
	npcs.garrison(world, seed);

	sim::Inventory inventory;
	// Noon when you arrive, so the first thing you see is the place rather
	// than the dark; a save brings its own hour with it.
	double startClock = sim::kDaySeconds * 0.5;

	// Nothing is loaded here. A launch is a new island, and the last one is
	// picked up from the title screen by pressing C, which is where a choice
	// between two islands belongs rather than in a silent default.
	//
	// A run that is measuring or photographing something wants the island as
	// generated and must not write over anybody's save on the way out.
	const bool asGenerated = startX >= 0 || atMonument >= 0 || shotPath != nullptr ||
							 benchFrames > 0 || showBase || poseDraw >= 0 || poseSwing >= 0 ||
							 showPopups || clearAround || zoo || icons || arrowShot || rocks || reloadAt >= 0;
	// Whether there is one to go back to, for the title screen to say so.
	bool hasSave = false;
	if (!online) {
		sim::World peek;
		sim::BuildSystem peekBuild;
		sim::Session peekSession;
		hasSave = sim::loadSession(savePath, peekSession, peek, peekBuild);
	}

	if (poseSwing >= 0) {
		inventory.hotbar()[0] = sim::ItemStack{sim::ItemId::Spear, 1};
		inventory.hotbar()[1] = sim::ItemStack{sim::ItemId::Hatchet, 1};
		inventory.selectSlot(1);
	}
	if (poseDraw >= 0) {
		inventory.hotbar()[1] = sim::ItemStack{sim::ItemId::Bow, 1};
		inventory.add(sim::ItemId::Arrow, 20);
		inventory.selectSlot(1);
	}
	sim::Crafting crafting;
	// A finished thing with nowhere to go lands at your feet rather than
	// holding the queue up: pack, then belt, then the ground.
	struct DropAtFeet {
		sim::World* world;
		sim::Player* player;
	};
	DropAtFeet atFeet{&world, &player};
	crafting.setOverflow(&atFeet, [](const void* owner, sim::ItemStack stack) {
		const auto* at = static_cast<const DropAtFeet*>(owner);
		at->world->dropStack(stack, at->player->x, at->player->y);
	});
	const auto fillSandbox = [&] {
		// Nothing is handed out: in sandbox the whole recipe book is open and
		// costs nothing, so anything you want is a few seconds at the bench.
		// Ammunition included, which is why it still runs out in a fight.
		crafting.setFree(true);
	};
	if (sandbox) fillSandbox();
	if (armed) {
		inventory.hotbar()[0] = sim::ItemStack{sim::ItemId::Spear, 1};
		inventory.hotbar()[1] = sim::ItemStack{sim::ItemId::Hatchet, 1};
		inventory.hotbar()[2] = sim::ItemStack{sim::ItemId::Bow, 1};
		inventory.hotbar()[3] = sim::ItemStack{sim::ItemId::Ak47, 1};
		inventory.hotbar()[4] = sim::ItemStack{sim::ItemId::PumpShotgun, 1};
		inventory.add(sim::ItemId::Arrow, 40);
		inventory.add(sim::ItemId::RifleAmmo, 120);
		inventory.add(sim::ItemId::ShotgunShell, 40);
		inventory.hotbar()[5] = sim::ItemStack{sim::ItemId::RocketLauncher, 1};
		// The rifle starts with a magazine in it, so the belt has something to
		// report about a gun that is not in your hand.
		player.loaded = sim::ItemId::Ak47;
		player.rounds = 17;
		inventory.add(sim::ItemId::Rocket, 6);
		inventory.add(sim::ItemId::C4, 3);
		inventory.selectSlot(3);
	}
	if (holdSlot >= 0) inventory.selectSlot(holdSlot);
	if (showBase) {
		// Two by two, walled in, with a doorway at the front and a door in it,
		// put up a few cells away so you can see it from outside.
		const int gx = static_cast<int>(player.x / sim::kBuildCell) + 1;
		const int gy = static_cast<int>(player.y / sim::kBuildCell);
		for (int oy = 0; oy < 2; ++oy) {
			for (int ox = 0; ox < 2; ++ox) {
				build.placeFoundation(gx + ox, gy + oy, 0, sim::BuildTier::Wood);
			}
		}
		for (int ox = 0; ox < 2; ++ox) {
			build.placeEdge(gx + ox, gy, sim::EdgeSide::North, sim::BuildKind::Wall, 0,
							sim::BuildTier::Wood);
			build.placeEdge(gx + ox, gy + 2, sim::EdgeSide::North, sim::BuildKind::Wall, 0,
							sim::BuildTier::Stone);
		}
		for (int oy = 0; oy < 2; ++oy) {
			build.placeEdge(gx, gy + oy, sim::EdgeSide::West, sim::BuildKind::Wall, 0,
							sim::BuildTier::Wood);
			build.placeEdge(gx + 2, gy + oy, sim::EdgeSide::West, sim::BuildKind::Wall, 0,
							sim::BuildTier::Metal);
		}
		sim::Structure& doorway = build.placeEdge(gx + 1, gy + 2, sim::EdgeSide::North,
												  sim::BuildKind::Doorway, 0, sim::BuildTier::Wood);
		doorway.kind = sim::BuildKind::Door;
		// And a roof on it, so the ceiling has somewhere to be looked at.
		for (int oy = 0; oy < 2; ++oy) {
			for (int ox = 0; ox < 2; ++ox) {
				build.placeCeiling(gx + ox, gy + oy, 0, sim::BuildTier::Wood);
			}
		}
		inventory.add(sim::ItemId::BuildingPlan, 1);
		// Something of everything, to see how it all sits together.
		const auto at = [&](int cx, int cy) { return (cx + 0.5) * sim::kBuildCell; };
		const auto down = [&](int cy) { return (cy + 0.5) * sim::kBuildCell; };
		const int fireId =
			build.deploy(sim::DeployKind::Campfire, at(gx, gy + 1), down(gy + 1), false, 0);
		const int furnaceId =
			build.deploy(sim::DeployKind::Furnace, at(gx + 1, gy + 1), down(gy + 1), false, 0);
		build.deploy(sim::DeployKind::WoodenBox, at(gx, gy), down(gy), false, 0);
		build.deploy(sim::DeployKind::ToolCupboard, at(gx + 1, gy), down(gy), true, 0);
		build.deploy(sim::DeployKind::SleepingBag, at(gx + 2, gy), down(gy), false, 0);
		sim::Deployable& fire = *build.deployableById(fireId);
		fire.lit = true;
		fire.container.add(sim::ItemId::Wood, 20);
		fire.container.add(sim::ItemId::MeatRaw, 3);
		sim::Deployable& furnace = *build.deployableById(furnaceId);
		furnace.lit = true;
		furnace.container.add(sim::ItemId::Wood, 20);
		furnace.container.add(sim::ItemId::MetalOre, 10);
		inventory.add(sim::ItemId::Campfire, 2);
	}

	sim::Projectiles projectiles;
	if (arrowShot) {
		// Three in the air at once, put there rather than fired, so the picture
		// is the same every time.
		world.clearNaturalIn(player.x - 500, player.y - 400, player.x + 500, player.y + 400);
		sim::Gun bowGun = sim::itemDef(sim::ItemId::Bow).gun;
		projectiles.spawn(player.x - 150, player.y - 60, 0, bowGun, bowGun.damage, true);
		projectiles.spawn(player.x - 150, player.y, 0, bowGun, bowGun.damage, true);
		sim::Gun rifleGun = sim::itemDef(sim::ItemId::Rifle).gun;
		projectiles.spawn(player.x - 150, player.y + 60, 0, rifleGun, rifleGun.damage, false);
		// And a rocket, which is the one round you are meant to see coming.
		const sim::ItemDef& tube = sim::itemDef(sim::ItemId::RocketLauncher);
		projectiles.spawnRocket(player.x - 150, player.y + 130, 0, tube.gun, tube.boom);
	}
	sim::Explosives explosives;
	client::Hud hud;
	client::Particles specks;
	client::Audio audio;
	if (!audio.open()) std::printf("No sound: %s\n", SDL_GetError());
	client::Panel panel;
	// What the buttons under the detail pane do. The screen knows what a thing
	// is and where it sits; the game knows what eating and getting dressed mean.
	{
		client::ItemActions actions;
		actions.drop = [&](sim::ItemStack stack) {
			world.dropStack(stack, player.x + SDL_cos(player.aim) * 34,
							player.y + SDL_sin(player.aim) * 34);
		};
		actions.consume = [&](sim::ItemId id) {
			if (sim::consume(player, inventory, id)) {
			}
		};
		actions.wear = [&](sim::ItemId id) {
			if (inventory.take(id, 1) < 1) return;
			const sim::ItemStack was = inventory.worn();
			inventory.worn() = sim::ItemStack{id, 1};
			if (was.id != sim::ItemId::None) inventory.add(was.id, was.count);
		};
		actions.takeOff = [&] {
			const sim::ItemStack was = inventory.worn();
			if (was.id == sim::ItemId::None) return;
			if (inventory.add(was.id, was.count) != 0) return;
			inventory.worn() = sim::ItemStack{};
		};
		panel.useActions(std::move(actions));
	}
	client::MapScreen map(renderer);
	if (clearAround) {
		world.clearNaturalIn(player.x - 400, player.y - 400, player.x + 400, player.y + 400);
	}
	if (rocks) {
		// One of each, in a line in front of you, on clear ground.
		world.clearNaturalIn(player.x - 800, player.y - 400, player.x + 800, player.y + 400);
		for (int i = 0; i < sim::kNodeKindCount; ++i) {
			const auto kind = static_cast<sim::NodeKind>(i);
			world.plant(kind, player.x - 420 + i * 170, player.y - 60,
						static_cast<std::uint32_t>(i * 7919 + 13));
		}
	}
	if (zoo) {
		// One of each, in a line in front of you, standing still.
		world.clearNaturalIn(player.x - 700, player.y - 300, player.x + 700, player.y + 300);
		npcs.mutableList().clear();
		int at = 0;
		for (int i = 0; i < 4; ++i) {
			sim::Npc npc{};
			npc.id = 9000 + i;
			npc.kind = static_cast<sim::NpcKind>(i);
			npc.x = player.x - 360 + at * 180;
			npc.y = player.y - 40;
			npc.facing = 0;
			npc.hp = sim::npcDef(npc.kind).hp;
			npc.homeX = npc.x;
			npc.homeY = npc.y;
			npc.leash = 1;
			npc.seed = static_cast<std::uint32_t>(i * 977);
			// Frozen mid-blow, so a screenshot shows what a strike looks like.
			npc.state = sim::NpcState::Attack;
			npc.attackTimer = sim::npcDef(npc.kind).attackCooldown * 0.35;
			npcs.mutableList().push_back(npc);
			++at;
		}
	}
	if (showPopups) {
		hud.say("+4 Wood", player.x - 40, player.y - 40, client::rgb(0xefeadd));
		hud.say("+12 Leather", player.x + 50, player.y + 10, client::rgb(0xefeadd));
		hud.say("-22", player.x, player.y - 70, client::rgb(0xffd9d9));
		hud.say("Killed a Wolf", player.x - 10, player.y + 60, client::rgb(0xefeadd));
	}
	if (showMap) map.toggle();
	// What the building plan would put down, cycled with B.
	sim::BuildKind buildKind = sim::BuildKind::Foundation;
	/** Whether the next thing put down goes in turned a quarter. */
	bool deployTurned = false;
	// Held open rather than clicked through: B for what to build, right click
	// on a piece for what to make it of.
	client::Wheel buildWheel;
	client::Wheel upgradeWheel;
	int upgradePiece = 0;
	if (showPanel) {
		inventory.add(sim::ItemId::Wood, 320);
		inventory.add(sim::ItemId::Stone, 180);
		inventory.add(sim::ItemId::Cloth, 60);
		inventory.add(sim::ItemId::Leather, 24);
		inventory.add(sim::ItemId::Scrap, 12);
		if (showCraft) {
			panel.toggleCraft();
			if (craftCategory >= 0) panel.chooseCategory(craftCategory);
		} else {
			panel.toggleInventory();
			panel.inspect(0);
			// What is being worn, when there is one, so the suit's own numbers
			// can be looked at rather than a rock's. After inspect, which
			// clears it.
			panel.inspectWorn(wearWhat != sim::ItemId::None);
		}
		crafting.queue(inventory, sim::recipes()[0], 0, 4);
		if (dragFrom >= 0) {
			crafting.queue(inventory, sim::recipes()[1], 0, 2);
			crafting.queue(inventory, sim::recipes()[2], 0, 1);
			int pw = 0;
			int ph = 0;
			int ww = 0;
			int wh = 0;
			SDL_GetWindowSizeInPixels(window, &pw, &ph);
			SDL_GetWindowSize(window, &ww, &wh);
			panel.showQueueDrag(window, crafting, dragFrom, dragOver, pw, ph,
								ww > 0 ? static_cast<float>(pw) / ww : 1.0f);
		}
	}

	client::Terrain terrain(renderer);
	client::Night night(renderer);
	client::Sprites sprites(renderer);
	client::Paint paint(renderer);
	paint.useText(&lettering);
	hud.useText(&lettering);

	// The system arrow is hidden and the game draws its own. An arrow points
	// up and to the left, which is a lie in a game where the thing under the
	// cursor is what you are aiming at: a dot has no direction and sits
	// exactly on the point it means.
	SDL_HideCursor();

	// Vsync off, always: the counter is there to show what the game costs, and
	// pinned to the panel's refresh it only ever shows the panel.
	if (!SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED)) {
		std::printf("could not turn vsync off: %s\n", SDL_GetError());
	}

	// The last frame's size and density, so a click knows what it landed on.
	int lastWidth = kWindowWidth;
	int lastHeight = kWindowHeight;
	double lastDensity = 1;

	/** Whether the death screen is up, and whether space has been pressed. */
	bool showHelp = false;
	bool fullscreen = false;
	/** Which fist the next bare-handed blow comes off. */
	bool punchLeft = false;
	bool paused = false;
	/** The card you land on, until you say how you want to play. */
	bool title = connectTo.empty() && !showPanel && !showMap && poseSwing < 0 &&
				 benchFrames <= 0 && !deathTest && !showPopups && (!shotPath || showTitle);
	/** Typing a line of chat, and what has been typed so far. */
	bool typing = false;
	std::string typed;
	bool dead = false;
	/** Nought while dead, a bag's number to wake in it, or -1 for a beach. */
	int wakeIn = 0;

	// The island's own clock, carried over from the last sitting.
	std::uint32_t inputSeq = 0;
	double sinceSave = 0;
	double deathClock = 0;
	bool deathAsked = false;
	bool deathSeen = false;
	std::size_t chatSeen = 0;

	double clock = startClock;
	if (startAtNight) clock = 0;

	// Where the camera is, as opposed to where the player is.
	double cameraX = player.x;
	double cameraY = player.y;

	double zoom = startZoom;
	int framesLeft = benchFrames;
	double benchTime = 0;
	double benchWorst = 0;
	bool running = true;
	// The cursor in pixels, which is what everything on screen is laid out in.
	// SDL reports it in points, and the two differ on a dense display.
	const auto cursorOnScreen = [&](float& px, float& py) {
		SDL_GetMouseState(&px, &py);
		int pw = 0;
		int ph = 0;
		int ww = 0;
		int wh = 0;
		SDL_GetWindowSizeInPixels(window, &pw, &ph);
		SDL_GetWindowSize(window, &ww, &wh);
		const float d = ww > 0 ? static_cast<float>(pw) / ww : 1.0f;
		px *= d;
		py *= d;
		return d;
	};
	if (showWheel) {
		int pw = 0;
		int ph = 0;
		SDL_GetWindowSizeInPixels(window, &pw, &ph);
		buildWheel.show(client::buildPlanOptions(), pw * 0.5f, ph * 0.5f);
	}
	std::uint64_t last = SDL_GetPerformanceCounter();
	std::vector<const sim::ResourceNode*> visible;
	double fpsClock = 0;
	int fpsFrames = 0;
	double fps = 0;

	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) running = false;
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
				// Closes whatever is open first, and only then stops the game.
				// Dead, it does nothing: a paused death screen is a game you
				// cannot get out of.
				if (dead) {
					// Nothing to do here but choose where to wake up.
				} else if (panel.open()) {
					panel.close();
				} else if (map.open()) {
					map.close();
				} else if (showHelp) {
					showHelp = false;
				} else {
					paused = !paused;
				}
			}
			if (event.type == SDL_EVENT_MOUSE_WHEEL) {
				if (inventory.held() == sim::ItemId::BuildingPlan) {
					const int step = event.wheel.y > 0 ? 1 : sim::kBuildKindCount - 1;
					buildKind = static_cast<sim::BuildKind>(
						(static_cast<int>(buildKind) + step) % sim::kBuildKindCount);
				} else {
					zoom = std::clamp(zoom * (1 + event.wheel.y * 0.1), kZoomMin, kZoomMax);
				}
			}
			if (event.type == SDL_EVENT_KEY_DOWN && panel.typingAmount() &&
				panel.typeAmount(event.key.key)) {
				// The amount box has the keyboard: a number key is a digit,
				// not a belt slot.
				continue;
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key >= SDLK_1 &&
				event.key.key <= SDLK_6) {
				inventory.selectSlot(static_cast<int>(event.key.key - SDLK_1));
			}
			if (title) {
				if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
					// Two ways in, and only one of them opens yet: enter or S
					// drops you on your island, M is the online game, which is
					// not built.
					if (event.key.key == SDLK_RETURN || event.key.key == SDLK_S) {
						sandbox = true;
						fillSandbox();
						title = false;
					}
					if (event.key.key == SDLK_C && hasSave) {
						// The island out of the save, rebuilt from its own
						// seed, with everything that happened to it on top.
						sim::Session session;
						if (sim::loadSession(savePath, session, world, build)) {
							seed = session.seed;
							player = session.player;
							inventory = session.inventory;
							clock = session.clock;
							npcs.populate(world, seed);
							npcs.garrison(world, seed);
							sandbox = true;
							fillSandbox();
							title = false;
							std::printf("Carried on from %s, %.1f hours on\n", savePath.c_str(),
										session.hoursAway);
						}
					}
					if (event.key.key == SDLK_M) titleNotice = 3.0;
					if (event.key.key == SDLK_ESCAPE) running = false;
				}
				continue;
			}
			if (typing) {
				// While typing, the keyboard belongs to the line being typed.
				if (event.type == SDL_EVENT_TEXT_INPUT) typed += event.text.text;
				if (event.type == SDL_EVENT_KEY_DOWN) {
					if (event.key.key == SDLK_BACKSPACE && !typed.empty()) typed.pop_back();
					if (event.key.key == SDLK_RETURN || event.key.key == SDLK_ESCAPE) {
						if (event.key.key == SDLK_RETURN && !typed.empty()) net.sendChat(typed);
						typed.clear();
						typing = false;
						SDL_StopTextInput(window);
					}
				}
				continue;
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN && online &&
				!event.key.repeat) {
				typing = true;
				SDL_StartTextInput(window);
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_N && online &&
				net.inviteFrom() != 0 && !event.key.repeat) {
				net.sendInviteReply(net.inviteFrom(), false);
				net.clearInvite();
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_T && online &&
				!event.key.repeat) {
				// Asked of whoever is in front of you, as an invite should be.
				std::uint16_t nearest = 0;
				double bestD = 120;
				for (const auto& [id, other] : net.others()) {
					const double d = SDL_sqrt((other.x - player.x) * (other.x - player.x) +
											  (other.y - player.y) * (other.y - player.y));
					if (d > bestD) continue;
					nearest = id;
					bestD = d;
				}
				if (nearest) {
					net.sendInvite(nearest);
				} else {
				}
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Y && online &&
				net.inviteFrom() != 0 && !event.key.repeat) {
				net.sendInviteReply(net.inviteFrom(), true);
				net.clearInvite();
			}
			if (event.type == SDL_EVENT_KEY_DOWN && sandbox && !event.key.repeat &&
				(event.key.key == SDLK_LEFTBRACKET || event.key.key == SDLK_RIGHTBRACKET)) {
				const double hours = event.key.key == SDLK_RIGHTBRACKET ? 1 : -1;
				clock += hours * sim::kDaySeconds / 24;
				if (clock < 0) clock += sim::kDaySeconds;
			}
			if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
				(event.key.key == SDLK_MINUS || event.key.key == SDLK_EQUALS)) {
				audio.setVolume(audio.volume() + (event.key.key == SDLK_EQUALS ? 0.1 : -0.1));
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F11 &&
				!event.key.repeat) {
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen);
			}
			if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
				(event.key.key == SDLK_H || event.key.key == SDLK_F1)) {
				showHelp = !showHelp;
			}
			if (event.type == SDL_EVENT_KEY_DOWN && dead && !event.key.repeat) {
				// A number picks a bag; B, space or enter takes a beach. A
				// number with no bag behind it takes the beach as well, rather
				// than leaving you pressing keys at a screen that ignores you.
				if (event.key.key >= SDLK_1 && event.key.key <= SDLK_9) {
					wakeIn = static_cast<int>(event.key.key - SDLK_1) + 1;
				}
				if (event.key.key == SDLK_B || event.key.key == SDLK_SPACE ||
					event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
					wakeIn = -1;
				}
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_M && !event.key.repeat) {
				map.toggle();
			}
			// Tab is the pack and C is the bench: two faces of one screen, so
			// either key takes you straight to its own.
			if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
				event.key.key == SDLK_TAB) {
				panel.toggleInventory();
			}
			if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
				event.key.key == SDLK_C) {
				panel.toggleCraft();
			}
			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && panel.open()) {
				const float px = event.button.x * static_cast<float>(lastDensity);
				const float py = event.button.y * static_cast<float>(lastDensity);
				if (event.button.button == SDL_BUTTON_LEFT) {
					// A press on a slot picks the stack up to carry it.
					panel.press(inventory, crafting, px, py, lastWidth, lastHeight,
								static_cast<float>(lastDensity));
				}
				if (panel.dragging().id == sim::ItemId::None) {
					panel.click(inventory, crafting, px, py,
								event.button.button == SDL_BUTTON_RIGHT, lastWidth, lastHeight,
								static_cast<float>(lastDensity));
				}
			}
			if (event.type == SDL_EVENT_MOUSE_MOTION && panel.open()) {
				// What turns a press into a drag.
				panel.motion(inventory, event.motion.x * static_cast<float>(lastDensity),
							 event.motion.y * static_cast<float>(lastDensity), lastWidth,
							 lastHeight, static_cast<float>(lastDensity));
			}
			if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && panel.open()) {
				panel.release(inventory, crafting,
							  event.button.x * static_cast<float>(lastDensity),
							  event.button.y * static_cast<float>(lastDensity), lastWidth,
							  lastHeight, static_cast<float>(lastDensity),
							  [&](sim::ItemStack stack) {
								  world.dropStack(stack,
												  player.x + SDL_cos(player.aim) * 34,
												  player.y + SDL_sin(player.aim) * 34);
							  });
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_B &&
				!event.key.repeat && inventory.held() == sim::ItemId::BuildingPlan) {
				// Held down: the ring of things you can build, chosen by
				// pushing towards one and letting go.
				float mx = 0;
				float my = 0;
				cursorOnScreen(mx, my);
				buildWheel.show(client::buildPlanOptions(), mx, my);
			}
			if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_B && buildWheel.open()) {
				float mx = 0;
				float my = 0;
				const float d = cursorOnScreen(mx, my);
				const int picked = buildWheel.release(mx, my, d);
				if (picked >= 0) buildKind = static_cast<sim::BuildKind>(picked);
			}
			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
				event.button.button == SDL_BUTTON_RIGHT && !panel.open() && !map.open() &&
				[&] {
					sim::DeployKind put = sim::DeployKind::Campfire;
					return sim::deployableOf(inventory.held(), put);
				}()) {
				// Turned a quarter, before it goes down. The same button that
				// draws a bow, because with a thing to place in your hand
				// there is no bow to draw.
				deployTurned = !deployTurned;
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Q && !event.key.repeat) {
				// The quick way round, for anyone who would rather not hold a
				// key: foundation, wall, doorway, door, ceiling, and again.
				buildKind = static_cast<sim::BuildKind>(
					(static_cast<int>(buildKind) + 1) % sim::kBuildKindCount);
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_G && !event.key.repeat) {
				sim::ItemStack& held = inventory.hotbar()[inventory.activeSlot()];
				if (held.id != sim::ItemId::None) {
					world.dropStack(held, player.x + SDL_cos(player.aim) * 36,
									player.y + SDL_sin(player.aim) * 36);
					held = sim::ItemStack{};
				}
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_R && !event.key.repeat) {
				sim::reload(player, inventory);
			}
			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_E && !event.key.repeat) {
				// A crate at a monument first: it is the thing you came for.
				sim::LootCrate* crate = nullptr;
				double crateD = sim::PlayerVitals::kInteract;
				for (sim::LootCrate& c : world.crates()) {
					if (c.looted) continue;
					const double d = SDL_sqrt((c.x - player.x) * (c.x - player.x) +
											  (c.y - player.y) * (c.y - player.y));
					if (d > crateD) continue;
					crate = &c;
					crateD = d;
				}
				if (crate) {
					panel.openContainer(&crate->container, "Crate");
				} else if (sim::Deployable* thing =
						build.deployableNear(player.x, player.y, sim::PlayerVitals::kInteract)) {
					if (thing->kind == sim::DeployKind::Campfire ||
						thing->kind == sim::DeployKind::Furnace) {
						thing->lit = !thing->lit;
						audio.build();
					}
					if (!thing->container.slots.empty()) {
						panel.openContainer(&thing->container,
											sim::itemDef(sim::itemOf(thing->kind)).name, thing);
					} else if (thing->kind == sim::DeployKind::SleepingBag) {
					}
				} else if (sim::drink(world, player)) {
					audio.pickup();
				}
				if (sim::Structure* door =
						build.nearestDoor(player.x, player.y, sim::kBuildCell * 0.9)) {
					if (door->kind == sim::BuildKind::Door) {
						if (door->locked && door->owner != 0) {
						} else if (online) {
							net.sendDoor(door->id, !door->open);
						} else {
							door->open = !door->open;
							audio.build();
						}
					}
				}
				const sim::PickResult got = sim::pickUp(world, build, player, inventory);
				if (got.picked && got.stack.count > 0) {
					audio.pluck();
					hud.say(std::string("+") + std::to_string(got.stack.count) + " " +
								sim::itemDef(got.stack.id).name,
							got.x, got.y, client::rgb(0xefeadd));
				}
			}
		}

		const std::uint64_t now = SDL_GetPerformanceCounter();
		double dt = static_cast<double>(now - last) / static_cast<double>(SDL_GetPerformanceFrequency());
		last = now;
		// Paused, the island holds still; online there is nothing to pause.
		// Online the island does not stop for you; alone it does.
		if ((paused && !online) || title) dt = 0;
		// A frame that took a second - a dragged window, a sleeping laptop -
		// must not teleport anyone across the island.
		dt = std::min(dt, 0.1);

		int width = 0;
		int height = 0;
		SDL_GetRenderOutputSize(renderer, &width, &height);
		int windowW = 0;
		int windowH = 0;
		SDL_GetWindowSize(window, &windowW, &windowH);
		// A retina display hands out twice the pixels for the same window. The
		// world is drawn in those pixels, so without this everything comes out
		// half the size it was meant to be.
		const double density = windowW > 0 ? static_cast<double>(width) / windowW : 1.0;

		// The fair view. Everybody sees the same rectangle of island, whatever
		// window or monitor they are on: the same shape and the same amount of
		// ground. A wider screen draws it bigger, not wider, and whatever is
		// left over at the sides is covered rather than given away. See
		// docs/systems/fair-view.md.
		const double pointsW = width / density;
		const double pointsH = height / density;
		double fairW = pointsW;
		double fairH = pointsW / kViewAspect;
		if (fairH > pointsH) {
			fairH = pointsH;
			fairW = pointsH * kViewAspect;
		}
		// How much bigger this window's fair rectangle is than the reference
		// one. The world is drawn that much bigger, so the ground inside it
		// stays the same.
		const double fit = std::sqrt(fairW * fairH / kViewArea);
		const double scale = zoom * fit * density;
		const float barX = static_cast<float>((pointsW - fairW) * 0.5 * density);
		const float barY = static_cast<float>((pointsH - fairH) * 0.5 * density);
		lastWidth = width;
		lastHeight = height;
		lastDensity = density;

		const bool* keys = SDL_GetKeyboardState(nullptr);
		// Dead, there is nothing to do but decide where to wake up; and while
		// a line of chat is being typed the keys are not steering anyone.
		sim::PlayerInput input;
		if (!dead && !typing) {
			if (keys[SDL_SCANCODE_W]) input.moveY -= 1;
			if (keys[SDL_SCANCODE_S]) input.moveY += 1;
			if (keys[SDL_SCANCODE_A]) input.moveX -= 1;
			if (keys[SDL_SCANCODE_D]) input.moveX += 1;
			input.sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
		}

		float mouseX = 0;
		float mouseY = 0;
		SDL_GetMouseState(&mouseX, &mouseY);
		// The mouse is in window points; the frame may be in denser pixels, and
		// the middle of the screen is the camera rather than the player.
		const double cursorX = cameraX + (mouseX * density - width * 0.5) / scale;
		const double cursorY = cameraY + (mouseY * density - height * 0.5) / scale;
		input.aim = SDL_atan2(cursorY - player.y, cursorX - player.x);

		audio.listenAt(player.x, player.y);
		sim::stepPlayer(world, build, player, input, dt);
		if (online) {
			net.poll(build, dt);
			net.sendInput(input, ++inputSeq);
			// Your own guess is pulled gently back to where the server says you
			// are: a hard snap every tick makes walking about feel like ice.
			const double dx = net.serverX() - player.x;
			const double dy = net.serverY() - player.y;
			const double off = SDL_sqrt(dx * dx + dy * dy);
			if (off > 120) {
				player.x = net.serverX();
				player.y = net.serverY();
			} else if (off > 1) {
				const double pull = std::min(1.0, dt * 6);
				player.x += dx * pull;
				player.y += dy * pull;
			}
			const std::string refused = net.takeRefusal();
		}
		world.update(dt);
		hud.update(dt);
		const std::size_t roundsBefore = projectiles.list().size();
		const sim::NpcEvents animals = npcs.update(world, build, projectiles, dt, player);
		if (zoo) {
			// Held mid-blow, every frame, because update would otherwise put
			// them straight back to wandering and the picture would show a
			// line of animals standing about.
			for (sim::Npc& show : npcs.mutableList()) {
				show.state = sim::NpcState::Attack;
				show.attackTimer = sim::npcDef(show.kind).attackCooldown * 0.35;
			}
		}
		if (projectiles.list().size() > roundsBefore) {
			// Somebody else's gunfire: further off than any other sound, and
			// quieter at its own spot than your own gun is.
			const sim::Bullet& shot = projectiles.list().back();
			if (const client::GunSound* report = client::gunSoundOf(sim::ItemId::Rifle)) {
				audio.from(shot.x, shot.y, [&] { audio.gunshot(*report, 0.75); }, 1400);
			}
		}
		// Nothing warms you yet: the campfire arrives with the deployables.
		build.updateDeployables(world, dt);
		specks.update(dt);
		if (!online && !asGenerated) {
			sinceSave += dt;
			if (sinceSave > 20) {
				sinceSave = 0;
				sim::Session session;
				session.seed = seed;
				session.clock = clock;
				session.player = player;
				session.inventory = inventory;
				sim::saveSession(savePath, session, world, build);
			}
		}
		// Embers off anything burning, so a fire looks alight from across a field.
		for (const sim::Deployable& thing : build.deployables()) {
			if (!thing.lit) continue;
			if (SDL_randf() < dt * 20) specks.ember(thing.x, thing.y - 4);
		}
		if (deathTest) {
			// Dies, waits, asks for a beach, and says whether it worked.
			deathClock += dt;
			if (deathClock > 0.4 && player.alive && !dead && !deathSeen) {
				sim::hurtPlayer(player, inventory, 999);
			}
			if (dead) deathSeen = true;
			if (deathClock > 1.0 && dead && !deathAsked) {
				deathAsked = true;
				SDL_Event press{};
				press.type = SDL_EVENT_KEY_DOWN;
				press.key.key = SDLK_B;
				press.key.repeat = 0;
				SDL_PushEvent(&press);
			}
			if (deathClock > 2.0) {
				std::printf("death test: died %s, asked %s, now %s, %s, at %.0f,%.0f\n",
							deathSeen ? "yes" : "NO", deathAsked ? "yes" : "NO",
							dead ? "STILL ON THE DEATH SCREEN" : "back in the game",
							player.alive ? "alive" : "DEAD", player.x, player.y);
				running = false;
			}
		}
		clock += dt;
		const double dark = sim::darkness(clock);
		sim::updateSurvival(world, player, inventory, dt, sim::nightness(clock),
							build.warmthAt(player.x, player.y));
		sim::updateUse(player, inventory, dt, player.sprinting);

		if (!player.alive && !dead) {
			// The map remembers where, because everything you were carrying is
			// about to be lying there on a thirty minute clock.
			map.markDeath(player.x, player.y);
			// Everything you were carrying falls where you did.
			for (sim::ItemStack& stack : inventory.hotbar()) {
				if (stack.id != sim::ItemId::None) {
					world.dropStack(stack, player.x + SDL_randf() * 20 - 10,
									player.y + SDL_randf() * 20 - 10);
					stack = sim::ItemStack{};
				}
			}
			for (sim::ItemStack& stack : inventory.pack()) {
				if (stack.id != sim::ItemId::None) {
					world.dropStack(stack, player.x + SDL_randf() * 20 - 10,
									player.y + SDL_randf() * 20 - 10);
					stack = sim::ItemStack{};
				}
			}
			// Whatever was on the bench goes too: a half made hatchet is not
			// yours any more, and what it was costing falls with the rest.
			for (const sim::ItemStack& owed : crafting.abandon()) {
				world.dropStack(owed, player.x + SDL_randf() * 20 - 10,
								player.y + SDL_randf() * 20 - 10);
			}
			// Dead: the screen waits for you rather than snatching you back,
			// and everything you were carrying stays where it fell.
			dead = true;
			panel.close();
			map.close();
		}
		if (dead && wakeIn != 0) {
			// Your things stay where you fell; you wake in the bag you chose,
			// or on a beach with nothing.
			const sim::Deployable* bag = nullptr;
			if (wakeIn > 0) {
				int seen = 0;
				for (const sim::Deployable& thing : build.deployables()) {
					if (thing.kind != sim::DeployKind::SleepingBag || thing.owner != 0) continue;
					if (++seen == wakeIn) bag = &thing;
				}
			}
			if (wakeIn > 0 && !bag) {
				// Asked for a bag that is not there: a beach, then.
				wakeIn = -1;
				bag = nullptr;
			}
			{
				dead = false;
				dropIn(world, player, static_cast<std::uint32_t>(SDL_GetTicks()));
				if (bag) {
					player.x = bag->x;
					player.y = bag->y;
				}
				inventory = sim::Inventory();
				wakeIn = 0;
			}
		}
		if (animals.playerDamage > 0 && player.alive && !dead) {
			sim::hurtPlayer(player, inventory, animals.playerDamage);
			audio.playerHurt();
			specks.burst(player.x, player.y, 8, client::rgb(0xc22b2b), 160, 0.45, 2.8);
			specks.shake(6, 0.25);
			hud.say("-" + std::to_string(static_cast<int>(animals.playerDamage)), player.x,
					player.y - 22, client::rgb(0xd8483a));
		}

		if (poseDraw >= 0) {
			player.bowDraw = sim::kBowDrawSeconds * poseDraw;
			player.aim = 0;
		}
		if (wearWhat != sim::ItemId::None) inventory.worn() = sim::ItemStack{wearWhat, 1};
		if (bleedFor > 0) player.bleeding = bleedFor;
		if (reloadAt >= 0) {
			// Held part way through a magazine change, for a screenshot.
			const sim::Gun& show = sim::itemDef(inventory.held()).gun;
			player.loaded = inventory.held();
			player.reloadTotal = show.reloadSeconds;
			player.reloadLeft = show.reloadSeconds * (1 - reloadAt);
		}
		if (poseSwing >= 0) {
			player.swingLength = 0.34;
			player.swingAnim = 0.34 * (1 - poseSwing);
			player.aim = 0;
		}

		{
			// A reload, in the beats it actually has. Measured across the tick
			// rather than before it: the clock has to be read on both sides of
			// tickReload or nothing is ever seen to pass a mark.
			const sim::Gun& loading = sim::itemDef(player.loaded).gun;
			const double was = player.reloadLeft;
			const double total = player.reloadTotal;
			const int roundsWas = player.rounds;
			sim::tickReload(player, inventory, dt);
			const double now = player.reloadLeft;
			if (loading.singly) {
				// A shell at a time: one noise each as it goes in.
				if (player.rounds > roundsWas) audio.magIn();
			} else if (loading.magazine > 1 && total > 0) {
				// The old one out and on the floor, the new one home, the bolt
				// let go. As fractions of this gun's own reload, so a slow one
				// takes its time over all three rather than snapping at the end.
				const auto passed = [&](double at) {
					const double mark = total * (1 - at);
					return was > mark && now <= mark;
				};
				if (passed(0.12)) {
					audio.magOut();
					specks.spentMagazine(player.x + SDL_cos(player.aim) * 10,
										 player.y + SDL_sin(player.aim) * 10, player.aim);
				}
				if (passed(0.62)) audio.magIn();
				if (passed(0.92)) audio.boltRelease();
			}
		}
		{
			// A thing finished on the bench says so.
			const std::size_t was = crafting.jobs().size();
			crafting.update(dt, inventory);
			if (crafting.jobs().size() < was) audio.craft();
		}
		panel.update(dt, inventory);
		// Walking away from an open box, or being shut out of it, closes it.
		if (panel.container()) {
			const sim::Deployable* thing = nullptr;
			for (const sim::Deployable& d : build.deployables()) {
				if (&d.container == panel.container()) thing = &d;
			}
			double away = 0;
			double atX = 0;
			double atY = 0;
			bool found = false;
			if (thing) {
				atX = thing->x;
				atY = thing->y;
				found = true;
			} else {
				for (const sim::LootCrate& crate : world.crates()) {
					if (&crate.container != panel.container()) continue;
					atX = crate.x;
					atY = crate.y;
					found = true;
				}
			}
			if (found) {
				away = SDL_sqrt((atX - player.x) * (atX - player.x) +
								(atY - player.y) * (atY - player.y));
				if (away > sim::PlayerVitals::kInteract + sim::PlayerVitals::kContainerSlack ||
					!build.canReach(player.x, player.y, atX, atY, 0)) {
					panel.close();
				}
			}
		}
		build.update(dt);

		// Held down: a blow or a shot goes out whenever the last one has come
		// round. A bow is the exception: it draws while held and looses when
		// the button comes up.
		const SDL_MouseButtonFlags buttons =
			panel.open() || map.open() || dead ? 0 : SDL_GetMouseState(nullptr, nullptr);
		const sim::Gun& heldGun = sim::itemDef(inventory.held()).gun;
		// isRanged, not damage: a rocket launcher does no damage of its own,
		// so asking after its damage said it was not a gun and the left button
		// went to the swing that is not there either. Nothing fired.
		const bool gun = sim::isRanged(heldGun);
		// The left button is the trigger of everything that has one; the right
		// draws a bow.
		const bool trigger = (buttons & SDL_BUTTON_LMASK) != 0;
		const bool drawing = (buttons & SDL_BUTTON_RMASK) != 0;
		if (gun) {
			const sim::FireResult shot =
				sim::fire(player, inventory, projectiles, trigger, drawing, dt);
			if (shot.fired) {
				if (const client::GunSound* report = client::gunSoundOf(shot.gun)) {
					audio.gunshot(*report);
				} else {
					audio.hit();
				}
				// The flash at the muzzle, and a nudge on the camera for the
				// bigger guns.
				const double mx = player.x + SDL_cos(shot.angle) * 20;
				const double my = player.y + SDL_sin(shot.angle) * 20;
				specks.burst(mx, my, 6, client::rgb(0xffd98a), 220, 0.12, 2.6, 0, shot.angle, 0.7);
				specks.shake(sim::itemDef(shot.gun).gun.damage > 50 ? 3.5 : 2.0, 0.12);
			}
			// A weapon with nothing in it makes no noise at all. The belt
			// already reads nought rounds, and a click every time you press
			// the button is a nag rather than information.
			(void)shot.empty;
		}
		explosives.update(world, build, npcs, player, inventory, dt);
		for (const sim::BulletHit& hit : projectiles.update(world, npcs, build, dt, &player)) {
			if (hit.npc) {
				specks.burst(hit.x, hit.y, 7, client::rgb(0x8c1f1f), 170, 0.4, 2.6);
				audio.from(hit.x, hit.y, [&] { audio.hitFlesh(); });
				if (hit.killed) audio.from(hit.x, hit.y, [&] { audio.enemyDie(); });
			}
			if (hit.node || hit.built) {
				specks.burst(hit.x, hit.y, 5, client::rgb(0x6b7a5c), 120, 0.3, 2.0, 180);
				audio.from(hit.x, hit.y, [&] { audio.hit(); });
			}
			if (hit.rocket) {
				specks.burst(hit.x, hit.y, 46, client::rgb(0xff8c2e), 300, 1.0, 5.0);
				specks.burst(hit.x, hit.y, 24, client::rgb(0xffd98a), 200, 0.7, 3.6);
				specks.shake(14, 0.5);
				// A blast carries further than anything else does.
				audio.from(hit.x, hit.y, [&] { audio.roar(); }, 2200);
			}
			if (hit.rocket) {
				// A rocket takes the piece it struck and everything round it.
				explosives.detonate(world, build, npcs, player, inventory, hit.x, hit.y,
									hit.blastRadius, hit.blastDamage, hit.builtId, true);
			}
			if (hit.player && player.alive && !dead) {
				sim::hurtPlayer(player, inventory, hit.damage);
				audio.playerHurt();
				specks.burst(player.x, player.y, 8, client::rgb(0xc22b2b), 160, 0.45, 2.8);
				specks.shake(5, 0.2);
				hud.say("-" + std::to_string(static_cast<int>(hit.damage)), player.x, player.y - 22,
						client::rgb(0xd8483a));
			}
			if (hit.npc && hit.killed) {
				hud.say(std::string("Killed a ") + sim::npcDef(hit.npcKind).name, hit.x, hit.y - 22,
						client::rgb(0xefeadd));
			}
		}
		const sim::ItemId inHand = inventory.held();
		// Running keeps your hands busy. Medicine is the exception: you can
		// always patch yourself up on the move.
		// Whether there is a roof over you, which is what decides whether a
		// tool reaches the floor you are standing on or the roof above it.
		const bool underRoof =
			build.ceilingAt(static_cast<int>(SDL_floor(player.x / sim::kBuildCell)),
							static_cast<int>(SDL_floor(player.y / sim::kBuildCell))) != nullptr;
		const bool handsFree =
			!player.sprinting ||
			sim::itemDef(inHand).category == sim::ItemCategory::Consumable;
		sim::DeployKind deployKind = sim::DeployKind::Campfire;
		const bool deploying = sim::deployableOf(inHand, deployKind);
		const bool planning = inHand == sim::ItemId::BuildingPlan;
		client::BuildTarget target = client::targetAt(cursorX, cursorY, buildKind);
		const char* refusal = nullptr;
		if (planning) {
			refusal = buildKind == sim::BuildKind::Foundation
						  ? build.refuseFoundation(world, target.gx, target.gy, 0)
					  : buildKind == sim::BuildKind::Ceiling
						  ? build.refuseCeiling(world, target.gx, target.gy, 0)
						  : build.refuseEdge(world, target.gx, target.gy, target.side, buildKind, 0);
			// Out of reach is a refusal like any other: you build what you can
			// put a hand on.
			const double reach = sim::PlayerVitals::kBuildReach;
			if (!refusal && SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
									 (cursorY - player.y) * (cursorY - player.y)) > reach) {
				refusal = "Too far away";
			}
			const sim::TierDef& twig = sim::tierDef(sim::BuildTier::Twig);
			if (!refusal && inventory.count(twig.cost.id) < twig.cost.count) {
				refusal = "Not enough wood";
			}
		}

		// Free on the fine grid rather than one to a cell: the cursor is where
		// the middle of the thing goes, snapped to whatever squares its own
		// footprint needs. Right click turns it a quarter before it goes down.
		double deployX = cursorX;
		double deployY = cursorY;
		sim::BuildSystem::snapDeploy(deployKind, deployTurned, deployX, deployY);
		const char* deployRefusal = nullptr;
		if (deploying) {
			deployRefusal =
				build.refuseDeploy(world, deployX, deployY, deployKind, deployTurned, 0);
			if (!deployRefusal &&
				SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
						 (cursorY - player.y) * (cursorY - player.y)) >
					sim::PlayerVitals::kDeployReach) {
				deployRefusal = "Too far away";
			}
			if (handsFree && (buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
				if (deployRefusal) {
				} else if (inventory.take(inHand, 1) > 0) {
					build.deploy(deployKind, deployX, deployY, deployTurned, 0);
					audio.build();
				}
				player.attackTimer = 0.4;
			}
		}

		if (planning && !refusal && handsFree && (buttons & SDL_BUTTON_LMASK) != 0 &&
			player.attackTimer <= 0) {
			const sim::TierDef& twig = sim::tierDef(sim::BuildTier::Twig);
			inventory.take(twig.cost.id, twig.cost.count);
			if (online) {
				// The server decides: a wall exists once everybody has been
				// told about it, not the moment you clicked.
				net.sendBuild(buildKind, target.gx, target.gy, target.side);
			} else if (buildKind == sim::BuildKind::Foundation) {
				build.placeFoundation(target.gx, target.gy, 0);
				// The cell is cleared as it is laid: nettles and saplings do
				// not survive a floor going down on them.
				world.clearNaturalIn(target.gx * sim::kBuildCell, target.gy * sim::kBuildCell,
									 (target.gx + 1) * sim::kBuildCell,
									 (target.gy + 1) * sim::kBuildCell);
			} else if (buildKind == sim::BuildKind::Ceiling) {
				build.placeCeiling(target.gx, target.gy, 0);
			} else if (buildKind == sim::BuildKind::Door) {
				// A door goes into the doorway that is already there.
				if (sim::Structure* doorway = build.edgeAt(target.gx, target.gy, target.side)) {
					doorway->kind = sim::BuildKind::Door;
					doorway->open = false;
				}
			} else {
				build.placeEdge(target.gx, target.gy, target.side, buildKind, 0,
								sim::BuildTier::Twig, player.x, player.y);
			}
			audio.build();
			player.attackTimer = 0.25;
		} else if (planning && refusal && (buttons & SDL_BUTTON_LMASK) != 0 &&
				   player.attackTimer <= 0) {
			player.attackTimer = 0.4;
		}

		if (inHand == sim::ItemId::Lock && (buttons & SDL_BUTTON_LMASK) != 0 &&
			player.attackTimer <= 0) {
			// A lock goes on a door of your own, and on nothing else.
			sim::Structure* door = build.nearest(cursorX, cursorY, sim::kBuildCell * 0.6);
			if (door && door->kind == sim::BuildKind::Door && door->owner == 0 && !door->locked &&
				inventory.take(sim::ItemId::Lock, 1) > 0) {
				door->locked = true;
			} else {
			}
			player.attackTimer = 0.4;
		}

		if (inHand == sim::ItemId::Hammer && handsFree && (buttons & SDL_BUTTON_RMASK) != 0 &&
			!upgradeWheel.open() && player.attackTimer <= 0) {
			// Right click on your own piece: the ring of what it could be made
			// of, held open while the button is down.
			sim::Structure* piece = build.nearest(cursorX, cursorY, 30, underRoof);
			if (piece && piece->owner == 0) {
				float mx = 0;
				float my = 0;
				cursorOnScreen(mx, my);
				upgradePiece = piece->id;
				upgradeWheel.show(client::tierOptions(*piece, inventory), mx, my);
			}
			player.attackTimer = 0.2;
		}
		if (upgradeWheel.open() && (buttons & SDL_BUTTON_RMASK) == 0) {
			float mx = 0;
			float my = 0;
			const float d = cursorOnScreen(mx, my);
			const int picked = upgradeWheel.release(mx, my, d);
			if (picked != -1) {
				for (sim::Structure& piece : build.mutableList()) {
					if (piece.id != upgradePiece) continue;
					if (picked == client::kWheelWreck) {
						if (build.demolish(piece, 0)) {
							audio.build();
							specks.burst(cursorX, cursorY, 14, client::rgb(0xc9b08a), 120, 0.5, 3);
						}
					} else if (build.upgradeTo(piece, static_cast<sim::BuildTier>(picked),
											   inventory)) {
						audio.build();
						specks.burst(cursorX, cursorY, 12, client::rgb(0x9aa8b4), 110, 0.5, 3);
					}
					break;
				}
			}
		}

		if (inHand == sim::ItemId::Hammer && handsFree && (buttons & SDL_BUTTON_LMASK) != 0 &&
			player.attackTimer <= 0) {
			// The hammer mends what is damaged and puts what is whole up a
			// tier. Only your own, and only within arm's length of you.
			sim::Structure* piece = build.nearest(cursorX, cursorY, 30, underRoof);
			if (piece && piece->owner != 0) piece = nullptr;
			if (piece &&
				SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
						 (cursorY - player.y) * (cursorY - player.y)) > 140) {
			} else if (piece && piece->hp < piece->maxHp) {
				// Mending costs wood by how much of it is gone.
				const int cost = std::max(1, static_cast<int>(
												 SDL_ceil((piece->maxHp - piece->hp) * 0.06)));
				if (inventory.count(sim::ItemId::Wood) < cost) {
				} else {
					inventory.take(sim::ItemId::Wood, cost);
					piece->hp = piece->maxHp;
					audio.build();
					specks.burst(cursorX, cursorY, 8, client::rgb(0xc9e08a), 90, 0.4, 2.2);
				}
			}
			// The left button mends, and does nothing else. Upgrading and
			// taking a piece down both live on the ring, because both are
			// decisions and a click is not.
			player.attackTimer = 0.35;
		}

		if (inHand == sim::ItemId::None && inventory.worn().id != sim::ItemId::None &&
			(buttons & SDL_BUTTON_RMASK) != 0 && player.attackTimer <= 0) {
			// An empty hand and the right button takes off what you are wearing.
			const sim::ItemStack was = inventory.worn();
			if (inventory.add(was.id, was.count) == 0) {
				inventory.worn() = sim::ItemStack{};
			}
			player.attackTimer = 0.4;
		}

		if (sim::itemDef(inHand).category == sim::ItemCategory::Clothing &&
			(buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
			// Worn rather than held: what was on before goes back in the pack.
			const sim::ItemStack was = inventory.worn();
			if (inventory.take(inHand, 1) > 0) {
				inventory.worn() = sim::ItemStack{inHand, 1};
				if (was.id != sim::ItemId::None) inventory.add(was.id, was.count);
			}
			player.attackTimer = 0.4;
		}

		if (sim::itemDef(inHand).category == sim::ItemCategory::Explosive &&
			(buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
			// Placed against whatever is nearest where you clicked, which is
			// what makes a charge a raiding tool rather than a grenade.
			const double reach = sim::PlayerVitals::kDeployReach;
			if (SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
						 (cursorY - player.y) * (cursorY - player.y)) > reach) {
			} else if (inventory.take(inHand, 1) > 0) {
				explosives.throwAt(inHand, player.x, player.y, cursorX, cursorY);
				char fuse[64];
				SDL_snprintf(fuse, sizeof(fuse), "%s thrown. %.1fs.", sim::itemDef(inHand).name,
							 sim::itemDef(inHand).boom.fuse);
			}
			player.attackTimer = 0.5;
		}

		const bool eating = sim::itemDef(inHand).category == sim::ItemCategory::Consumable;
		if (eating && (buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
			if (sim::consume(player, inventory, inHand)) {
				audio.craft();
			}
		}

		if (!gun && !planning && !eating && handsFree && inHand != sim::ItemId::Hammer &&
			(buttons & SDL_BUTTON_LMASK) != 0) {
			const sim::SwingResult blow = sim::swing(world, npcs, build, player, inventory);
			// One fist then the other. Flipped as the blow is thrown rather
			// than as it lands, or a punch that hits nothing never alternates.
			if (blow.swung) punchLeft = !punchLeft;
			if (blow.landed && blow.gained.count > 0) {
				// Off the top of the thing struck, not out of its middle: a
				// tree stands four radii above its foot, a rock about one.
				const double top =
					blow.y - blow.radius * (blow.kind == sim::NodeKind::Tree ? 3.4 : 1.4) - 6;
				hud.say(std::string("+") + std::to_string(blow.gained.count) + " " +
							sim::itemDef(blow.gained.id).name,
						blow.x, top, client::rgb(0xefeadd));
				if (blow.alsoGained.count > 0) {
					hud.say(std::string("+") + std::to_string(blow.alsoGained.count) + " " +
								sim::itemDef(blow.alsoGained.id).name,
							blow.x, top - 14, client::rgb(0xefeadd));
				}
			}
			if (blow.landed && !blow.hitNpc && !blow.built) {
				specks.burst(blow.x, blow.y - 4, 6, client::nodeColor(blow.kind), 130, 0.4, 2.4,
							 220);
				audio.from(blow.x, blow.y, [&] {
					switch (blow.kind) {
						case sim::NodeKind::Tree: audio.chopWood(); break;
						case sim::NodeKind::Metal: audio.hitMetal(); break;
						case sim::NodeKind::Sulfur: audio.hitSulfur(); break;
						case sim::NodeKind::Barrel: audio.hitStructureMetal(); break;
						default: audio.hitStone(); break;
					}
				});
				if (blow.broke) audio.from(blow.x, blow.y, [&] { audio.treeFall(); });
			}
			if (blow.built) {
				audio.from(blow.x, blow.y, [&] { audio.hitStructureWood(); });
			}
			if (blow.hitNpc) {
				specks.burst(blow.x, blow.y, 6, client::rgb(0x8c1f1f), 150, 0.35, 2.5);
				audio.from(blow.x, blow.y, [&] { audio.hitFlesh(); });
				if (blow.killed) audio.from(blow.x, blow.y, [&] { audio.enemyDie(); });
			}
			if (blow.swung && !blow.landed) audio.hit();
			if (blow.hitNpc && blow.killed) {
				hud.say(std::string("Killed a ") + sim::npcDef(blow.npcKind).name, blow.x, blow.y - 22,
						client::rgb(0xefeadd));
			} else if (blow.hitNpc) {
				hud.say("-" + std::to_string(static_cast<int>(blow.damage)), blow.x, blow.y - 16,
						client::rgb(0xffd9d9));
			}
		}

		SDL_SetRenderDrawColor(renderer, client::kVoid.r, client::kVoid.g, client::kVoid.b, 255);
		SDL_RenderClear(renderer);
		// Everything from here to the interface is drawn in the world, so the
		// pen goes with it: ink round a thing is part of the thing.
		paint.useWorldScale(static_cast<float>(scale));
		{
			// Eased after the player, at a rate that is the same however fast
			// the frames come.
			const double t = 1 - SDL_pow(0.001, dt);
			cameraX += (player.x - cameraX) * t;
			cameraY += (player.y - cameraY) * t;
			const double halfViewW = width / (2 * scale);
			const double halfViewH = height / (2 * scale);
			cameraX = sim::kWorldWidth > halfViewW * 2
						  ? std::clamp(cameraX, halfViewW, sim::kWorldWidth - halfViewW)
						  : sim::kWorldWidth / 2.0;
			cameraY = sim::kWorldHeight > halfViewH * 2
						  ? std::clamp(cameraY, halfViewH, sim::kWorldHeight - halfViewH)
						  : sim::kWorldHeight / 2.0;
		}
		double shakeX = 0;
		double shakeY = 0;
		specks.shakeOffset(shakeX, shakeY);
		const double camX = cameraX + shakeX;
		const double camY = cameraY + shakeY;
		terrain.draw(world, camX, camY, scale, width, height);
		terrain.drawScreen(camX, camY, scale, width, height);
		// The swell and the ink along the roads: neither fits in a baked tile.
		terrain.drawOver(paint, world, camX, camY, scale, width, height, clock);

		// The looting places are ground: they go down before anything stands
		// on them.
		for (const sim::Monument& monument : world.monuments()) {
			if (SDL_fabs(monument.x - player.x) > width / (2 * scale) + monument.radius) continue;
			if (SDL_fabs(monument.y - player.y) > height / (2 * scale) + monument.radius) continue;
			client::drawMonument(paint, monument, camX, camY, scale, width, height);
		}

		// Everything standing, back to front, so what is nearer the camera is
		// painted over what is behind it.
		const double halfW = width / (2 * scale);
		const double halfH = height / (2 * scale);
		// Reached past the edges of the screen: a tree is drawn well above its
		// own foot, and would otherwise pop in as its trunk crossed the top.
		const double margin = 220;
		world.nodesInRect(player.x - halfW - margin, player.y - halfH - margin,
						  player.x + halfW + margin, player.y + halfH + margin, visible);
		std::sort(visible.begin(), visible.end(),
				  [](const sim::ResourceNode* a, const sim::ResourceNode* b) { return a->y < b->y; });


		for (const sim::Structure& piece : build.list()) {
			if (piece.kind != sim::BuildKind::Foundation) continue;
			client::drawBuilt(paint, piece, camX, camY, scale, width, height);
		}

		const int myRegion = build.regionAt(player.x, player.y);
		// What is sealed in a room you are not in is out of sight, roof and all.
		const auto hidden = [&](double wx, double wy) {
			const int region = build.regionAt(wx, wy);
			return region != 0 && region != myRegion;
		};

		for (const sim::Dropped& drop : world.drops()) {
			const float sx = static_cast<float>((drop.x - camX) * scale) + width * 0.5f;
			const float sy = static_cast<float>((drop.y - camY) * scale) + height * 0.5f;
			if (sx < -40 || sy < -40 || sx > width + 40 || sy > height + 40) continue;
			if (hidden(drop.x, drop.y)) continue;
			// Riding a little above where it fell, so it reads as lying loose
			// rather than planted.
			const float bob = static_cast<float>(SDL_sin(drop.age * 2.4 + drop.id) * 2 * scale);
			client::drawItemIcon(paint, drop.stack.id, sx,
								 sy - 2 * static_cast<float>(scale) + bob,
								 static_cast<float>(22 * scale));
		}
		for (const sim::Deployable& thing : build.deployables()) {
			if (thing.kind != sim::DeployKind::SleepingBag) continue;
			const float sx = static_cast<float>((thing.x - camX) * scale) + width * 0.5f;
			const float sy = static_cast<float>((thing.y - camY) * scale) + height * 0.5f;
			client::drawDeployable(paint, thing, sx, sy, static_cast<float>(scale));
		}

		for (sim::LootCrate& crate : world.crates()) {
			if (crate.looted) continue;
			const float sx = static_cast<float>((crate.x - camX) * scale) + width * 0.5f;
			const float sy = static_cast<float>((crate.y - camY) * scale) + height * 0.5f;
			if (sx < -60 || sy < -60 || sx > width + 60 || sy > height + 60) continue;
			client::drawCrate(paint, crate, sx, sy, static_cast<float>(scale));
		}

		static std::vector<const sim::Npc*> animalsNear;
		npcs.inRect(player.x - halfW - margin, player.y - halfH - margin, player.x + halfW + margin,
					player.y + halfH + margin, animalsNear);

		bool playerDrawn = false;
		const auto drawPlayer = [&] {
			client::HumanLook look;
			// Drawn from the camera like everything else, so a shake moves the
			// whole picture rather than sliding the world out from under you.
			look.x = static_cast<float>((player.x - camX) * scale) + width * 0.5f;
			look.y = static_cast<float>((player.y - camY) * scale) + height * 0.5f;
			look.facing = static_cast<float>(player.aim);
			look.phase = static_cast<float>(player.walkPhase);
			look.radius = static_cast<float>(sim::PlayerRules::kRadius * scale);
			look.swimming = player.swimming;
			// Struck, you flash: the one thing that says a blow landed on you
			// rather than near you.
			if (player.hurtFlash > 0) look.hurt = client::rgb(0xff9a9a);
			// What you are wearing, and nothing if you are wearing nothing: a
			// survivor with an empty wear slot washes up bare.
			switch (inventory.worn().id) {
				case sim::ItemId::Clothing:
					look.shirt = client::rgb(0x8a6134);
					look.legs = client::rgb(0x6a4a28);
					break;
				case sim::ItemId::RadSuit:
					// Sealed, hood and all: a radiation suit with a bare head
					// keeps nothing out.
					look.shirt = client::rgb(0xe8c73c);
					look.legs = client::rgb(0xc0a220);
					look.helmet = client::rgb(0xe8c73c);
					look.gloves = client::rgb(0xc0a220);
					look.boots = client::rgb(0xc0a220);
					break;
				case sim::ItemId::MetalSuit:
					// Road signs over hide: a grey plate on a brown body.
					look.shirt = client::rgb(0xb9b2a4);
					look.legs = client::rgb(0x6a4a28);
					break;
				case sim::ItemId::HeavyMetalSuit:
					// Plate over everything, legs and head included.
					look.shirt = client::rgb(0xa9b6c0);
					look.legs = client::rgb(0x76828c);
					look.helmet = client::rgb(0x9aa7b2);
					look.gloves = client::rgb(0x8d9aa4);
					look.boots = client::rgb(0x8d9aa4);
					break;
				default:
					look.shirt = look.skin;
					look.legs = look.skin;
					break;
			}
			// Bare unless the case above said otherwise, and set every frame,
			// or taking a suit off leaves the helmet on.
			if (inventory.worn().id != sim::ItemId::RadSuit &&
				inventory.worn().id != sim::ItemId::HeavyMetalSuit) {
				look.helmet = client::Color{0, 0, 0, 0};
				look.gloves = client::Color{0, 0, 0, 0};
				look.boots = client::Color{0, 0, 0, 0};
			}
			look.held = inventory.held();
			look.bowDraw = static_cast<float>(player.bowDraw / sim::kBowDrawSeconds);
			// Where the swing has got to, as a fraction of its own length.
			look.punchLeft = punchLeft;
			// Every gun, not just the ones with a magazine: a reload is a
			// reload and the hands do the same thing whatever is in them.
			look.reloadFed = sim::itemDef(player.loaded).gun.magazine > 1 &&
							 !sim::itemDef(player.loaded).gun.singly;
			look.reloading = player.reloadTotal > 0 && player.reloadLeft > 0
								 ? static_cast<float>(1 - player.reloadLeft / player.reloadTotal)
								 : -1.0f;
			look.swingT = player.swingAnim > 0 && player.swingLength > 0
							  ? static_cast<float>(1 - player.swingAnim / player.swingLength)
							  : -1;
			client::drawHuman(paint, look);
			playerDrawn = true;
		};

		std::size_t nextAnimal = 0;
		std::sort(animalsNear.begin(), animalsNear.end(),
				  [](const sim::Npc* a, const sim::Npc* b) { return a->y < b->y; });
		const auto drawAnimalsUpTo = [&](double y) {
			while (nextAnimal < animalsNear.size() && animalsNear[nextAnimal]->y <= y) {
				const sim::Npc* animal = animalsNear[nextAnimal++];
				if (!playerDrawn && animal->y > player.y) drawPlayer();
				if (hidden(animal->x, animal->y)) continue;
				const float ax = static_cast<float>((animal->x - camX) * scale) + width * 0.5f;
				const float ay = static_cast<float>((animal->y - camY) * scale) + height * 0.5f;
				const sim::NpcDef& def = sim::npcDef(animal->kind);
				if (def.human) {
					// A scientist and a soldier are people, and go through the
					// one routine that draws a person.
					client::HumanLook look;
					look.x = ax;
					look.y = ay;
					look.facing = static_cast<float>(animal->facing);
					look.phase = static_cast<float>(animal->animPhase);
					look.radius = static_cast<float>(def.radius * scale);
					look.stride = static_cast<float>(
						std::min(1.0, std::hypot(animal->vx, animal->vy) / 90));
					look.shirt = animal->kind == sim::NpcKind::Soldier ? client::rgb(0x6f7a52)
																	  : client::rgb(0xe6ebf0);
					look.legs = animal->kind == sim::NpcKind::Soldier ? client::rgb(0x454d31)
																	 : client::rgb(0x758494);
					look.held = animal->kind == sim::NpcKind::Soldier ? sim::ItemId::Ak47
																	 : sim::ItemId::Revolver;
					client::drawHuman(paint, look);
				} else {
					client::drawAnimal(paint, *animal, ax, ay, static_cast<float>(scale));
				}
				client::drawAnimalTag(paint, *animal, ax, ay, static_cast<float>(scale));
			}
		};

		for (const sim::ResourceNode* node : visible) {
			drawAnimalsUpTo(node->y);
			// Felled, broken or picked: gone from the island until it grows
			// back, rather than standing there at nought health.
			if (node->hp <= 0) {
				// What is left where it stood: a hole in the ground the size of
				// its own foot, which closes over as it grows back.
				const float hx = static_cast<float>((node->x - camX) * scale) + width * 0.5f;
				const float hy = static_cast<float>((node->y - camY) * scale) + height * 0.5f;
				if (hx < -80 || hy < -80 || hx > width + 80 || hy > height + 80) continue;
				const float rx = static_cast<float>(node->radius * scale) * 0.6f;
				const float ry = static_cast<float>(node->radius * scale) * 0.35f;
				// Inked like everything else standing on the island, or it read
				// as a smudge on the grass rather than a hole in it.
				paint.fillPoly(client::ellipsePoints(hx, hy, rx, ry),
							   client::Color{22, 24, 20, 210});
				paint.outlinePoly(client::ellipsePoints(hx, hy, rx, ry), client::kInkWidth,
								  client::kInk);
				continue;
			}
			if (!playerDrawn && node->y > player.y) drawPlayer();
			const float sx = static_cast<float>((node->x - camX) * scale) + width * 0.5f;
			const float sy = static_cast<float>((node->y - camY) * scale) + height * 0.5f;
			const sim::Biome under = world.biomeAt(node->x, node->y);
			const bool snowy = under == sim::Biome::Snow;
			// The lighter tree of the open grassland; the pines keep to the
			// forest and the snow.
			const bool broadleaf = node->kind == sim::NodeKind::Tree && under == sim::Biome::Grass;
			// A tree with something living behind it goes see-through. It is
			// not only about not losing yourself under one: a bear stalking you
			// from behind a trunk was invisible until it was on top of you.
			float alpha = 1.0f;
			if (node->kind == sim::NodeKind::Tree) {
				const auto behind = [&](double ax, double ay) {
					const double dx = ax - node->x;
					const double dy = ay - node->y;
					return std::abs(dx) < node->radius * 1.8 && dy < node->radius * 0.6 &&
						   dy > -node->radius * 3.5;
				};
				if (behind(player.x, player.y)) {
					alpha = 0.45f;
				} else {
					for (const sim::Npc* animal : animalsNear) {
						if (!behind(animal->x, animal->y)) continue;
						alpha = 0.55f;
						break;
					}
				}
			}
			// Struck, it shudders where it stands.
			const float shakeAt =
				node->shake > 0
					? static_cast<float>(SDL_sin(node->shake * 90) * node->shake * 14 * scale)
					: 0.0f;
			sprites.draw(*node, sx + shakeAt, sy, static_cast<float>(scale), snowy, broadleaf,
						 alpha);
			if (sim::showsHealth(*node)) {
				// White in black, which reads on snow and on grass alike.
				// Measured in world units, so the bar grows and shrinks with
				// the thing it belongs to. Half the old height: five points
				// read as a plank, not as a gauge.
				const float w = 32 * static_cast<float>(scale);
				const float h = 2.5f * static_cast<float>(scale);
				const float edge = static_cast<float>(scale);
				const float bx = sx - w / 2;
				const float by = sy + static_cast<float>(node->radius * scale) * 0.9f;
				paint.fillRect(bx - edge, by - edge, w + edge * 2, h + edge * 2, client::kInk);
				paint.fillRect(bx, by, w * node->hp / node->maxHp, h, client::rgb(0xffffff));
			}
		}
		drawAnimalsUpTo(1e9);
		for (const auto& [id, other] : net.others()) {
			if (!other.alive) continue;
			const float ox = static_cast<float>((other.drawX - camX) * scale) + width * 0.5f;
			const float oy = static_cast<float>((other.drawY - camY) * scale) + height * 0.5f;
			if (ox < -80 || oy < -80 || ox > width + 80 || oy > height + 80) continue;
			if (hidden(other.drawX, other.drawY)) continue;
			client::HumanLook look;
			look.x = ox;
			look.y = oy;
			look.facing = static_cast<float>(other.aim);
			look.phase = static_cast<float>(other.walkPhase);
			look.radius = static_cast<float>(sim::PlayerRules::kRadius * scale);
			look.swimming = other.swimming;
			look.stride = 0.7f;
			client::drawHuman(paint, look);
			// A mate is marked, and nobody else is: finding the rest is the game.
			if (other.team != 0 && other.team == net.team()) {
				paint.inkedCircle(ox, oy - 26 * static_cast<float>(scale),
								  5 * static_cast<float>(scale), client::rgb(0x5fb85f),
								  client::kInkFine);
			}
		}
		if (!playerDrawn) drawPlayer();

		// The one progress indicator, and it sits beside you rather than over
		// the belt: a timer you have to look away from is a timer you do not
		// watch. A bandage, a syringe and a magazine change all use it, always
		// the same size, so a three second job visibly runs faster than a five
		// second one instead of the two looking identical.
		{
			double progress = -1;
			client::Color arc = client::rgb(0xffffff);
			if (player.useTotal > 0 && player.useLeft > 0) {
				progress = 1 - player.useLeft / player.useTotal;
			} else if (player.reloadTotal > 0 && player.reloadLeft > 0) {
				progress = 1 - player.reloadLeft / player.reloadTotal;
			} else if (player.bowDraw > 0) {
				progress = std::min(1.0, player.bowDraw / sim::kBowDrawSeconds);
			}
			if (progress >= 0) {
				const float px = static_cast<float>((player.x - camX) * scale) + width * 0.5f;
				const float py = static_cast<float>((player.y - camY) * scale) + height * 0.5f;
				const float rr = 20 * static_cast<float>(scale);
				const float from = -0.8f;
				const float to = 0.8f;
				const auto sweep = [&](float upTo, float thick, client::Color color) {
					std::vector<client::Point> along;
					for (int i = 0; i <= 16; ++i) {
						const float a = from + (upTo - from) * (static_cast<float>(i) / 16);
						along.push_back({px + std::cos(a) * rr, py + std::sin(a) * rr});
					}
					paint.outlinePoly(along, thick, color, false);
				};
				sweep(to, 4, client::Color{8, 10, 12, 140});
				sweep(from + (to - from) * static_cast<float>(progress), 2.5f, arc);
			}
		}

		for (const sim::Deployable& thing : build.deployables()) {
			if (thing.kind == sim::DeployKind::SleepingBag) continue;
			if (hidden(thing.x, thing.y)) continue;
			const float sx = static_cast<float>((thing.x - camX) * scale) + width * 0.5f;
			const float sy = static_cast<float>((thing.y - camY) * scale) + height * 0.5f;
			client::drawDeployable(paint, thing, sx, sy, static_cast<float>(scale),
								   static_cast<float>(clock));
			if (sim::showsHealth(thing)) {
				const float w = 36 * static_cast<float>(scale);
				const float h = 2.5f * static_cast<float>(scale);
				const float edge = static_cast<float>(scale);
				paint.fillRect(sx - w / 2 - edge, sy + 20 * static_cast<float>(scale) - edge,
							   w + edge * 2, h + edge * 2, client::kInk);
				paint.fillRect(sx - w / 2, sy + 20 * static_cast<float>(scale),
							   w * thing.hp / thing.maxHp, h, client::rgb(0xffffff));
			}
		}

		// The ceilings you have built, over the top of what is under them. Not
		// the one you are standing under: from above you are looking down
		// through the cell you occupy, which is how a roofed room is a room
		// rather than a lid.
		{
			const int myGx = static_cast<int>(SDL_floor(player.x / sim::kBuildCell));
			const int myGy = static_cast<int>(SDL_floor(player.y / sim::kBuildCell));
			const auto& sealed = build.enclosedCells();
			for (const sim::Structure& piece : build.list()) {
				if (piece.kind != sim::BuildKind::Ceiling) continue;
				// The whole room goes faint, not the one square you stand on,
				// and faint rather than gone: you can see what is under it and
				// you can still see that it is there.
				bool inside = piece.gx == myGx && piece.gy == myGy;
				if (!inside && myRegion != 0) {
					const auto it = sealed.find((static_cast<std::uint64_t>(
													 static_cast<std::uint32_t>(piece.gx))
												 << 32) |
												static_cast<std::uint32_t>(piece.gy));
					inside = it != sealed.end() && it->second == myRegion;
				}
				if (inside) {
					client::drawBuiltFaint(paint, piece, camX, camY, scale, width, height);
				} else {
					client::drawBuilt(paint, piece, camX, camY, scale, width, height);
				}
			}
		}

		// A roof over every sealed room but the one you are in: a base is a
		// thing you cannot see into, which is most of what makes one worth
		// building.
		for (const auto& [key, region] : build.enclosedCells()) {
			if (region == myRegion) continue;
			const int gx = static_cast<int>(static_cast<std::int32_t>(key >> 32));
			const int gy = static_cast<int>(static_cast<std::uint32_t>(key));
			const float rx = static_cast<float>((gx * sim::kBuildCell - camX) * scale) +
							 width * 0.5f;
			const float ry = static_cast<float>((gy * sim::kBuildCell - camY) * scale) +
							 height * 0.5f;
			const float size = static_cast<float>(sim::kBuildCell * scale) + 1;
			if (rx < -size || ry < -size || rx > width || ry > height) continue;
			paint.fillRect(rx, ry, size, size, client::rgb(0x2b2721));
			// Shingle hatching, so it reads as a roof rather than a hole.
			for (int i = 1; i < 4; ++i) {
				const float t = i / 4.0f;
				paint.line(rx, ry + size * t, rx + size, ry + size * t, client::kInkFine,
						   client::Color{0, 0, 0, 56});
			}
		}

		for (const sim::Structure& piece : build.list()) {
			if (piece.kind == sim::BuildKind::Foundation) continue;
			// A ceiling is over your head, so it is drawn after everything
			// that stands under it, and not at all over the cell you are on.
			if (piece.kind == sim::BuildKind::Ceiling) continue;
			client::drawBuilt(paint, piece, camX, camY, scale, width, height);
			if (sim::showsHealth(piece)) {
				double cx = 0;
				double cy = 0;
				if (piece.kind == sim::BuildKind::Foundation) {
					cx = (piece.gx + 0.5) * sim::kBuildCell;
					cy = (piece.gy + 0.5) * sim::kBuildCell;
				} else {
					double x0 = 0;
					double y0 = 0;
					double x1 = 0;
					double y1 = 0;
					sim::edgeSegment(piece.gx, piece.gy, piece.side, x0, y0, x1, y1);
					cx = (x0 + x1) * 0.5;
					cy = (y0 + y1) * 0.5;
				}
				const float w = 40 * static_cast<float>(scale);
				const float h = 2.5f * static_cast<float>(scale);
				const float edge = static_cast<float>(scale);
				const float bx = static_cast<float>((cx - camX) * scale) + width * 0.5f - w / 2;
				const float by = static_cast<float>((cy - camY) * scale) + height * 0.5f;
				paint.fillRect(bx - edge, by - edge, w + edge * 2, h + edge * 2, client::kInk);
				paint.fillRect(bx, by, w * piece.hp / piece.maxHp, h, client::rgb(0xffffff));
			}
		}
		if (planning) {
			client::drawGhost(paint, target, sim::BuildTier::Twig, refusal == nullptr, camX, camY,
							  scale, width, height);
		}
		if (deploying) {
			// The floor it would take, at its own size and its own turn, with
			// the fine grid drawn inside it: what matters is the footprint,
			// and a whole cell lit up said nothing about a two by one bench.
			double halfWide = 0;
			double halfDeep = 0;
			sim::BuildSystem::deployBounds(deployKind, deployTurned, deployX, deployY, halfWide,
										   halfDeep);
			const float gx =
				static_cast<float>((deployX - halfWide - camX) * scale) + width * 0.5f;
			const float gy =
				static_cast<float>((deployY - halfDeep - camY) * scale) + height * 0.5f;
			const float w = static_cast<float>(halfWide * 2 * scale);
			const float h = static_cast<float>(halfDeep * 2 * scale);
			const client::Color tint = deployRefusal ? client::Color{224, 80, 60, 90}
													 : client::Color{124, 200, 255, 90};
			paint.fillRect(gx, gy, w, h, tint);
			paint.outlineRoundRect(gx, gy, w, h, 2 * static_cast<float>(scale),
								   1.5f * static_cast<float>(scale), client::kInk);
			const float step = static_cast<float>(sim::kDeployCell * scale);
			for (float lx = gx + step; lx < gx + w - 0.5f; lx += step) {
				paint.line(lx, gy, lx, gy + h, static_cast<float>(scale),
						   client::Color{20, 17, 13, 70});
			}
			for (float ly = gy + step; ly < gy + h - 0.5f; ly += step) {
				paint.line(gx, ly, gx + w, ly, static_cast<float>(scale),
						   client::Color{20, 17, 13, 70});
			}
		}

		for (const sim::Bullet& bullet : projectiles.list()) {
			const float bx = static_cast<float>((bullet.x - camX) * scale) + width * 0.5f;
			const float by = static_cast<float>((bullet.y - camY) * scale) + height * 0.5f;
			const double speed = SDL_sqrt(bullet.vx * bullet.vx + bullet.vy * bullet.vy);
			const float ux = static_cast<float>(bullet.vx / speed);
			const float uy = static_cast<float>(bullet.vy / speed);
			// A heavier round is drawn heavier: an AK's 78 reads thicker and
			// longer than a revolver's 42.
			const float width = 1.4f + static_cast<float>(bullet.damage) * 0.03f;
			// How far the streak reaches back, on screen, so it scales with the
			// view like any other length. The WIDTHS below do not: Paint scales
			// every stroke by the view itself, and doing it here as well drew
			// the rounds at half again the size they were meant to be.
			const float tail = static_cast<float>(speed * 0.02 * scale) *
							   (1 + static_cast<float>(bullet.damage) * 0.006f);
			// And it thins and fades over its last moments in the air.
			const float left = static_cast<float>(
				std::clamp(bullet.left / (std::max(1.0, speed) * 0.2), 0.0, 1.0));
			if (bullet.arrow) {
				// A knapped head, a thin shaft and a forked nock: the arrow is
				// mostly stick, so the stick is thin and the head is what you
				// see. Laid out along its flight, nock behind and point ahead.
				const float len = 13.0f;
				const float px = ux;
				const float py = uy;
				// Across the arrow, for the head's barbs and the nock's fork.
				const float ax = -uy;
				const float ay = ux;
				const auto at = [&](float along, float across) {
					return client::Point{bx + px * along * static_cast<float>(scale) +
											 ax * across * static_cast<float>(scale),
										 by + py * along * static_cast<float>(scale) +
											 ay * across * static_cast<float>(scale)};
				};
				// The shaft, which is most of it: the head is about a fifth of
				// an arrow's length, not a third.
				const client::Point tailAt = at(-len, 0);
				const client::Point neck = at(len * 0.6f, 0);
				paint.line(tailAt.x, tailAt.y, neck.x, neck.y, 1.1f + client::kInkFine,
						   client::kInk);
				paint.line(tailAt.x, tailAt.y, neck.x, neck.y, 1.1f, client::rgb(0x9a7346));
				// The fork at the back, where the string sits. No binding round
				// the joint: a band across the shaft at this size is a lump.
				paint.fillPoly({at(-len, -1.6f), at(-len * 0.74f, -0.5f), at(-len * 0.74f, 0.5f),
								at(-len, 1.6f), at(-len * 0.86f, 0)},
							   client::rgb(0xb08a55));
				paint.outlinePoly({at(-len, -1.6f), at(-len * 0.74f, -0.5f),
								   at(-len * 0.74f, 0.5f), at(-len, 1.6f), at(-len * 0.86f, 0)},
								  client::kInkFine, client::kInk);
				// The stone head: barbed at the shoulders, tapering to a point.
				const std::vector<client::Point> head{
					at(len, 0),            at(len * 0.72f, -1.5f), at(len * 0.6f, -1.9f),
					at(len * 0.66f, -0.6f), at(len * 0.66f, 0.6f), at(len * 0.6f, 1.9f),
					at(len * 0.72f, 1.5f)};
				paint.inkedPoly(head, client::rgb(0xb8b2a6), client::kInkFine);
				// One flake off the face of it, which is what says knapped.
				paint.line(at(len * 0.9f, -0.35f).x, at(len * 0.9f, -0.35f).y,
						   at(len * 0.68f, -0.9f).x, at(len * 0.68f, -0.9f).y,
						   client::kInkFine * 0.7f, client::rgb(0x7f7a70));
			} else if (bullet.rocket) {
				// A rocket is a thing you can see coming, not a tracer: a
				// body, a nose cone, fins at the tail and the flame behind it.
				const float len = 11.0f;
				const float ax = -uy;
				const float ay = ux;
				const auto at = [&](float along, float across) {
					return client::Point{bx + ux * along * static_cast<float>(scale) +
											 ax * across * static_cast<float>(scale),
										 by + uy * along * static_cast<float>(scale) +
											 ay * across * static_cast<float>(scale)};
				};
				// The flame, flickering off the tail on the round's own clock.
				const float flare =
					1.0f + 0.35f * static_cast<float>(SDL_sin(bullet.left * 70));
				paint.fillPoly({at(-len * 0.62f, -2.2f), at(-len * 0.62f, 2.2f),
								at(-len * (1.5f + flare * 0.6f), 0)},
							   client::rgb(0xff8a3a));
				paint.fillPoly({at(-len * 0.62f, -1.2f), at(-len * 0.62f, 1.2f),
								at(-len * (1.0f + flare * 0.45f), 0)},
							   client::rgb(0xffe08a));
				// The fins, one either side of the tail.
				paint.inkedPoly({at(-len * 0.55f, -1.8f), at(-len * 0.2f, -1.8f),
								 at(-len * 0.5f, -4.0f)},
								client::rgb(0x6c727a), client::kInkFine);
				paint.inkedPoly({at(-len * 0.55f, 1.8f), at(-len * 0.2f, 1.8f),
								 at(-len * 0.5f, 4.0f)},
								client::rgb(0x6c727a), client::kInkFine);
				// The body, and the warhead in front of it.
				paint.inkedPoly({at(-len * 0.62f, -1.9f), at(len * 0.3f, -1.9f),
								 at(len * 0.3f, 1.9f), at(-len * 0.62f, 1.9f)},
								client::rgb(0x8d949c), client::kInkFine);
				paint.inkedPoly({at(len * 0.3f, -1.9f), at(len, 0), at(len * 0.3f, 1.9f)},
								client::rgb(0xc2452f), client::kInkFine);
			} else {
				const float core = width * (0.4f + 0.6f * left);
				const std::uint8_t fade = static_cast<std::uint8_t>(255 * left);
				// The black line round the core, so a tracer reads on snow.
				paint.line(bx - ux * tail, by - uy * tail, bx, by, core + 1.6f,
						   client::Color{20, 17, 13, fade});
				const client::Color hot = bullet.fromPlayer ? client::rgb(0xfff1a8)
															: client::rgb(0xff9a6a);
				paint.line(bx - ux * tail, by - uy * tail, bx, by, core,
						   client::Color{hot.r, hot.g, hot.b, fade});
			}
		}

		// Where a wall was blown out: a scorch on the ground that nobody may
		// build on until it fades, so the hole stays a hole long enough to be
		// walked through. It fades as its own timer runs down, which is the
		// only notice it gives, and the only one it needs.
		for (const sim::BuildSystem::Scorch& mark : build.scorches()) {
			const double cx = (mark.gx + 0.5) * sim::kBuildCell;
			const double cy = (mark.gy + 0.5) * sim::kBuildCell;
			const float sx = static_cast<float>((cx - camX) * scale) + width * 0.5f;
			const float sy = static_cast<float>((cy - camY) * scale) + height * 0.5f;
			const float half = static_cast<float>(sim::kBuildCell * scale) * 0.5f;
			const std::uint8_t fade =
				static_cast<std::uint8_t>(150 * std::clamp(mark.left / 30.0, 0.0, 1.0));
			if (mark.foundation) {
				paint.fillRect(sx - half, sy - half, half * 2, half * 2,
							   client::Color{28, 22, 18, fade});
			} else {
				// On an edge: a bar along the side the wall stood on. A cell
				// owns its north and its west edge; the other two belong to
				// its neighbours.
				const float thick = half * 0.22f;
				if (mark.side == sim::EdgeSide::North) {
					paint.fillRect(sx - half, sy - half, half * 2, thick,
								   client::Color{28, 22, 18, fade});
				} else {
					paint.fillRect(sx - half, sy - half, thick, half * 2,
								   client::Color{28, 22, 18, fade});
				}
			}
		}

		for (const sim::Charge& charge : explosives.list()) {
			const float cx = static_cast<float>((charge.x - camX) * scale) + width * 0.5f;
			const float cy = static_cast<float>((charge.y - camY) * scale) + height * 0.5f;
			const float r = 9 * static_cast<float>(scale);
			paint.inkedCircle(cx, cy, r, client::rgb(0x8a7a5a), client::kInkWidth);
			// The fuse, blinking faster as it runs down.
			const bool lit = SDL_fmod(charge.fuse * 6, 1.0) > 0.5;
			paint.fillCircle(cx, cy - r * 0.6f, r * 0.35f,
							 lit ? client::rgb(0xff6b4a) : client::rgb(0x3a3733));
		}

		if (dark > 0.02) {
			// The night is a veil over the whole screen and every fire erases
			// part of it, rather than painting light on top of the world: a
			// light that is added washes out everything under it, and what you
			// get is a glowing smudge with no ground inside it.
			night.begin(width, height, client::rgb(0x080c14), static_cast<float>(dark) * 0.97f);
			const auto glow = [&](double wx, double wy, double radius, float strength) {
				const float sx = static_cast<float>((wx - camX) * scale) + width * 0.5f;
				const float sy = static_cast<float>((wy - camY) * scale) + height * 0.5f;
				night.cut(sx, sy, static_cast<float>(radius * scale), strength);
			};
			// A survivor casts no light of their own. A torch in the hand does,
			// which is the whole point of carrying one.
			if (player.alive && inventory.held() == sim::ItemId::Torch) {
				glow(player.x, player.y, 340, 0.95f);
			}
			for (const sim::Deployable& thing : build.deployables()) {
				if (thing.lit) glow(thing.x, thing.y, 400, 1.0f);
			}
			night.present();
			// The fire's own colour back over the hole it cut, gently.
			if (player.alive && inventory.held() == sim::ItemId::Torch) {
				const float sx = static_cast<float>((player.x - camX) * scale) + width * 0.5f;
				const float sy = static_cast<float>((player.y - camY) * scale) + height * 0.5f;
				night.warm(sx, sy, static_cast<float>(340 * scale), client::rgb(0xff9a3c),
						   static_cast<float>(dark) * 0.22f);
			}
			for (const sim::Deployable& thing : build.deployables()) {
				if (!thing.lit) continue;
				const float sx = static_cast<float>((thing.x - camX) * scale) + width * 0.5f;
				const float sy = static_cast<float>((thing.y - camY) * scale) + height * 0.5f;
				night.warm(sx, sy, static_cast<float>(400 * scale), client::rgb(0xff8a30),
						   static_cast<float>(dark) * 0.26f);
			}
		}

		specks.draw(paint, camX, camY, scale, width, height);

		hud.drawPopups(renderer, camX, camY, scale, width, height);

		// What the key under your finger would do, if anything.
		char prompt[96] = {0};
		if (deploying) {
			SDL_snprintf(prompt, sizeof(prompt), "%s   %s", sim::itemDef(inHand).name,
						 deployRefusal ? deployRefusal : "click to put it down");
		} else if (planning) {
			static const char* kNames[4] = {"Foundation", "Wall", "Doorway", "Door"};
			SDL_snprintf(prompt, sizeof(prompt), "%s   %s   (B to change)",
						 kNames[static_cast<int>(buildKind)], refusal ? refusal : "click to place");
		} else {
			const sim::Dropped* nearest = nullptr;
			double best = sim::PlayerVitals::kInteract;
			for (const sim::Dropped& drop : world.drops()) {
				const double d = SDL_sqrt((drop.x - player.x) * (drop.x - player.x) +
										  (drop.y - player.y) * (drop.y - player.y));
				if (d > best) continue;
				nearest = &drop;
				best = d;
			}
			if (nearest) {
				SDL_snprintf(prompt, sizeof(prompt), "E   Pick up %d %s", nearest->stack.count,
							 sim::itemDef(nearest->stack.id).name);
			} else {
				world.nodesInRect(player.x - sim::PlayerVitals::kInteract, player.y - sim::PlayerVitals::kInteract,
								  player.x + sim::PlayerVitals::kInteract, player.y + sim::PlayerVitals::kInteract, visible);
				for (const sim::ResourceNode* node : visible) {
					if (node->kind != sim::NodeKind::Nettle || node->hp <= 0) continue;
					SDL_snprintf(prompt, sizeof(prompt), "E   Pick nettle");
					break;
				}
			}
			if (!prompt[0]) {
				for (const sim::LootCrate& crate : world.crates()) {
					if (crate.looted) continue;
					if (SDL_sqrt((crate.x - player.x) * (crate.x - player.x) +
								 (crate.y - player.y) * (crate.y - player.y)) >
						sim::PlayerVitals::kInteract) {
						continue;
					}
					SDL_snprintf(prompt, sizeof(prompt), "E   Open crate");
					break;
				}
			}
			if (!prompt[0]) {
				if (const sim::Deployable* thing =
						build.deployableNear(player.x, player.y, sim::PlayerVitals::kInteract)) {
					SDL_snprintf(prompt, sizeof(prompt), "E   %s",
								 sim::itemDef(sim::itemOf(thing->kind)).name);
				}
			}
			if (!prompt[0]) {
				// A door says what E would do to it, and says when it will not
				// open for you.
				if (sim::Structure* door =
						build.nearestDoor(player.x, player.y, sim::kBuildCell * 0.9)) {
					if (door->kind == sim::BuildKind::Door) {
						if (door->locked && door->owner != 0) {
							SDL_snprintf(prompt, sizeof(prompt), "Locked");
						} else {
							SDL_snprintf(prompt, sizeof(prompt), "E   %s",
										 door->open ? "Close" : "Open");
						}
					}
				}
			}
			// Fresh water is the last thing offered, being the one you are
			// standing in rather than standing at.
			if (!prompt[0] && world.freshAt(player.x, player.y)) {
				SDL_snprintf(prompt, sizeof(prompt), "E   Drink");
			}
		}
		hud.setVitals(player.calories, player.hydration, player.temperature, player.radiation,
					  player.bleeding,
					  player.useTotal > 0 && player.useLeft > 0 ? player.useLeft / player.useTotal
																: 0);
		// Whichever gun holds rounds says so on its own slot, held or not.
		hud.setMending(player.healOverTime);
		hud.setLoadedGun(player.loaded, player.rounds);
		hud.setClock(clock);
		hud.setAmmo(sim::isRanged(sim::itemDef(inventory.held()).gun)
						? sim::roundsCarried(player, inventory)
						: -1,
					player.loaded == inventory.held() ? player.rounds : 0,
					player.reloadTotal > 0 ? player.reloadLeft / player.reloadTotal : 0,
					player.bowDraw / sim::kBowDrawSeconds);
		// A crate emptied is a crate looted, and it fills again in its own time.
		for (sim::LootCrate& crate : world.crates()) {
			if (crate.looted) continue;
			bool empty = true;
			for (const sim::ItemStack& slot : crate.container.slots) {
				if (slot.id != sim::ItemId::None) empty = false;
			}
			if (!empty) continue;
			crate.looted = true;
			crate.respawn = sim::kCrateRespawnSeconds;
			if (panel.container() == &crate.container) panel.close();
		}

		for (const auto& [id, other] : net.others()) {
			if (other.team != 0 && other.team == net.team()) map.addMate(other.x, other.y);
		}
		// The interface is not in the world: its lines keep their own weight.
		paint.useWorldScale(1);

		// What is outside the fair view is covered. The world under it was
		// drawn and is thrown away, which costs a few thousand pixels of fill
		// and buys the guarantee that a wider monitor is not a better one.
		if (barX > 0.5f || barY > 0.5f) {
			const client::Color bar = client::kVoid;
			if (barX > 0.5f) {
				paint.fillRect(0, 0, barX, static_cast<float>(height), bar);
				paint.fillRect(width - barX, 0, barX, static_cast<float>(height), bar);
			}
			if (barY > 0.5f) {
				paint.fillRect(0, 0, static_cast<float>(width), barY, bar);
				paint.fillRect(0, height - barY, static_cast<float>(width), barY, bar);
			}
		}
		map.draw(paint, world, build, player, width, height, static_cast<float>(density));
		panel.setBench(sandbox ? 3 : build.benchTierAt(player.x, player.y, 0));
		panel.setSandbox(sandbox);
		// Asked so the flag does not pile up. Nothing is said about it: a full
		// queue is already eight chips and a count that reads 8/8.
		(void)panel.takeQueueFull();
		// What the hammer is over: what it is, what it is made of, how much of
		// it is left, and whether it is still young enough to take down. Only
		// with the hammer in hand, so it is something you ask for rather than
		// something the screen keeps telling you.
		if (inventory.held() == sim::ItemId::Hammer && !panel.open() && !map.open()) {
			if (const sim::Structure* piece = build.nearest(cursorX, cursorY, 30, underRoof)) {
				const float scaleUi = static_cast<float>(density);
				float mx = 0;
				float my = 0;
				cursorOnScreen(mx, my);
				const char* what = piece->kind == sim::BuildKind::Foundation ? "Foundation"
								   : piece->kind == sim::BuildKind::Ceiling  ? "Ceiling"
								   : piece->kind == sim::BuildKind::Doorway  ? "Doorway"
								   : piece->kind == sim::BuildKind::Door     ? "Door"
																			: "Wall";
				char made[64];
				SDL_snprintf(made, sizeof(made), "%s   %d / %d", sim::tierDef(piece->tier).name,
							 piece->hp, piece->maxHp);
				const double left = sim::BuildSystem::kFreeDemolishSeconds - piece->age;
				char note[64];
				if (piece->owner != 0) {
					SDL_snprintf(note, sizeof(note), "someone else's");
				} else if (left > 0) {
					SDL_snprintf(note, sizeof(note), "%dm%02ds to take it down",
								 static_cast<int>(left) / 60, static_cast<int>(left) % 60);
				} else {
					SDL_snprintf(note, sizeof(note), "too old to take down");
				}
				const float w = 186 * scaleUi;
				const float h = 62 * scaleUi;
				const float cx = mx + 18 * scaleUi;
				const float cy = my + 18 * scaleUi;
				paint.fillRoundRect(cx, cy, w, h, client::ui::kRadiusMedium, client::ui::kSurface);
				paint.outlineRoundRect(cx, cy, w, h, client::ui::kRadiusMedium, 1.5f * scaleUi,
									   client::kInk);
				lettering.draw(what, cx + 10 * scaleUi, cy + 8 * scaleUi, 14 * scaleUi, client::ui::kInk,
							   client::Face::BodyBold);
				lettering.draw(made, cx + 10 * scaleUi, cy + 26 * scaleUi, 11 * scaleUi, client::ui::kSubtle);
				lettering.draw(note, cx + 10 * scaleUi, cy + 42 * scaleUi, 10 * scaleUi,
							   piece->owner != 0 || left <= 0 ? client::ui::kWarn
															  : client::ui::kFaint);
			}
		}

		panel.draw(paint, inventory, crafting, width, height, static_cast<float>(density));

		// The rings, over everything: they are held open under your hand.
		{
			float mx = 0;
			float my = 0;
			const float d = cursorOnScreen(mx, my);
			buildWheel.draw(paint, mx, my, d);
			upgradeWheel.draw(paint, mx, my, d);
		}
		if (typing) {
			const std::string line = "say: " + typed + "_";
			paint.fillRect(0, height - 150 * static_cast<float>(density),
						   static_cast<float>(width), 30 * static_cast<float>(density),
						   client::Color{20, 17, 13, 200});
			lettering.draw(line, 20 * density, height - 146 * density, 16 * density,
						   client::rgb(0xefeadd));
		}

		if (icons) {
			// Every picture there is, laid out big, so they can be judged
			// against each other rather than one at a time on a belt.
			paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
						   client::rgb(0x2a2a28));
			const int cols = 10;
			const float cell = static_cast<float>(width) / (cols + 1);
			for (int i = 1; i < sim::kItemCount; ++i) {
				const int at = i - 1;
				const float cx = cell * (0.8f + at % cols);
				const float cy = cell * (0.8f + at / cols);
				paint.fillRect(cx - cell * 0.42f, cy - cell * 0.42f, cell * 0.84f, cell * 0.84f,
							   client::rgb(0x3a3a36));
				client::drawItemIcon(paint, static_cast<sim::ItemId>(i), cx, cy, cell * 0.62f);
				lettering.draw(sim::itemDef(static_cast<sim::ItemId>(i)).name, cx,
							   cy + cell * 0.3f, 9 * density, client::rgb(0xcfcfc8),
							   client::Face::Body, client::Align::Centre);
			}
		}

		if (title) {
			// Drawn over the island itself rather than a blank panel: the first
			// thing you see should be the place you are about to be dropped on.
			paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
						   client::Color{10, 12, 10, 190});
			lettering.draw("Oxide", width * 0.5f, height * 0.26f, 110 * density,
						   client::rgb(0xefeadd), client::Face::Display, client::Align::Centre);
			lettering.draw("an island, a rock, and whatever you make of them", width * 0.5f,
						   height * 0.45f, 20 * density, client::Color{160, 160, 150, 255},
						   client::Face::Body, client::Align::Centre);
			// Two modes, in a column of their own so the eye runs down them.
			// Multiplayer is listed and not yet built, which is what the grey
			// and the line under it say.
			struct Mode {
				const char* key;
				const char* what;
				bool ready;
			};
			const Mode kModes[] = {
				{"S", "Sandbox: a new island, every recipe open", true},
				{"C", hasSave ? "Carry on where you left off" : "Nothing saved yet", hasSave},
				{"M", "Multiplayer: not built yet", false},
				{"ESC", "Leave", true},
			};
			for (int i = 0; i < 4; ++i) {
				const float y = height * 0.55f + i * 36 * density;
				const client::Color ink = kModes[i].ready ? client::rgb(0xefeadd)
														  : client::Color{120, 120, 112, 255};
				lettering.draw(kModes[i].key, width * 0.5f - 260 * density, y, 20 * density, ink,
							   client::Face::Display);
				lettering.draw(kModes[i].what, width * 0.5f - 190 * density, y, 18 * density, ink);
			}
			if (titleNotice > 0) {
				titleNotice -= 1.0 / 60;
				lettering.draw("Multiplayer is not in this build yet.", width * 0.5f,
							   height * 0.55f + 4 * 36 * density + 14 * density, 16 * density,
							   client::rgb(0xd8483a), client::Face::Body, client::Align::Centre);
			}
		}

		if (paused) {
			// Settings, which for now is every binding in one place: it is
			// where a player looks for the controls.
			paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
						   client::Color{0, 0, 0, 170});
			lettering.draw(online ? "Settings" : "Paused", width * 0.5f, height * 0.13f,
						   54 * density, client::rgb(0xefeadd), client::Face::Display,
						   client::Align::Centre);
			lettering.draw("esc to carry on", width * 0.5f, height * 0.2f, 18 * density,
						   client::Color{160, 160, 150, 255}, client::Face::Body,
						   client::Align::Centre);

			struct Binding {
				const char* group;
				const char* key;
				const char* what;
			};
			static const Binding kBindings[] = {
				{"Moving", "WASD", "walk"},
				{nullptr, "SHIFT", "run"},
				{nullptr, "MOUSE", "aim"},
				{nullptr, "E", "use what you face"},
				{"Your hands", "1-6", "belt slot"},
				{nullptr, "LEFT CLICK", "swing or fire"},
				{nullptr, "RIGHT CLICK", "draw a bow, take off a coat"},
				{nullptr, "R", "reload"},
				{nullptr, "G", "drop the held stack"},
				{"Making and building", "TAB", "pack, bench and containers"},
				{nullptr, "Q", "cycle piece, holding a plan"},
				{nullptr, "CLICK", "move a stack"},
				{"The island", "M", "the map"},
				{nullptr, "T", "ask a nearby player to team up"},
				{nullptr, "Y / N", "accept or refuse"},
				{nullptr, "ENTER", "say something, online"},
				{nullptr, "H", "how the island works"},
				{nullptr, "- / =", "quieter, louder"},
				{nullptr, "F11", "full screen"},
				{nullptr, "ESC", "back out"},
			};
			float row = height * 0.26f;
			for (const Binding& binding : kBindings) {
				if (binding.group) {
					row += 12 * density;
					lettering.draw(binding.group, width * 0.5f - 300 * density, row, 20 * density,
								   client::Color{160, 160, 150, 255}, client::Face::Display);
					row += 28 * density;
				}
				lettering.draw(binding.key, width * 0.5f - 260 * density, row, 15 * density,
							   client::rgb(0xefeadd), client::Face::BodyBold);
				lettering.draw(binding.what, width * 0.5f - 60 * density, row, 15 * density,
							   client::rgb(0xefeadd));
				row += 24 * density;
			}
		}

		if (showHelp) {
			static const char* kLines[] = {
				"WASD  move        SHIFT  run",
				"LEFT CLICK  hit, fire, place     RIGHT CLICK  draw a bow",
				"1-6  belt         R  reload       E  use what is in front of you",
				"TAB  pack and bench               M  map",
				"B  what the plan puts down        H  close this",
			};
			paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
						   client::Color{0, 0, 0, 170});
			for (int i = 0; i < 5; ++i) {
				lettering.draw(kLines[i], width * 0.5f - 300 * density,
							   height * 0.35f + i * 32 * density, 18 * density,
							   client::rgb(0xefeadd));
			}
		}

		if (dead) {
			paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
						   client::Color{40, 8, 8, 170});
			lettering.draw("You died", width * 0.5f, height * 0.36f, 72 * density,
						   client::rgb(0xefeadd), client::Face::Display, client::Align::Centre);
			char line[96];
			SDL_snprintf(line, sizeof(line),
						 "Your things are on the ground where you fell.   Day %d.",
						 1 + static_cast<int>(clock / sim::kDaySeconds));
			lettering.draw(line, width * 0.5f, height * 0.4f + 60 * density, 18 * density,
						   client::rgb(0xefeadd), client::Face::Body, client::Align::Centre);
			// Every bag you have put down, with how far off it lies and which
			// way, so waking up is a decision rather than a button.
			int listed = 0;
			for (const sim::Deployable& thing : build.deployables()) {
				if (thing.kind != sim::DeployKind::SleepingBag || thing.owner != 0) continue;
				if (++listed > 9) break;
				const double dx = thing.x - player.x;
				const double dy = thing.y - player.y;
				const char* ns = dy < 0 ? "north" : "south";
				const char* ew = dx < 0 ? "west" : "east";
				SDL_snprintf(line, sizeof(line), "%d   Sleeping bag, %d cells %s%s", listed,
							 static_cast<int>(SDL_sqrt(dx * dx + dy * dy) / sim::kBuildCell), ns,
							 ew);
				lettering.draw(line, width * 0.5f, height * 0.4f + (100 + listed * 32) * density,
							   18 * density, client::rgb(0xefeadd), client::Face::Body,
							   client::Align::Centre);
			}
			lettering.draw("B   A beach, with nothing", width * 0.5f,
						   height * 0.4f + (132 + listed * 32) * density, 18 * density,
						   client::rgb(0xd8483a), client::Face::Body, client::Align::Centre);
		}

		if (!title)
			hud.draw(paint, inventory, static_cast<int>(std::lround(player.health)), width, height, panel.open() ? "" : prompt, static_cast<float>(density));

		fpsClock += dt;
		++fpsFrames;
		if (fpsClock >= 0.5) {
			fps = fpsFrames / fpsClock;
			fpsClock = 0;
			fpsFrames = 0;
		}
		if (!title) {
			// The day and the hour, top right, in the same quiet hand as the
			// counter opposite: no plate behind it and no word for night, which
			// the sky already says.
			const int day = 1 + static_cast<int>(clock / sim::kDaySeconds);
			const int hour24 = static_cast<int>(sim::dayFraction(clock) * 24);
			const int minute = static_cast<int>(SDL_fmod(sim::dayFraction(clock) * 24 * 60, 60));
			const int hour = hour24 % 12 == 0 ? 12 : hour24 % 12;
			char top[96];
			SDL_snprintf(top, sizeof(top), "Day %d   %d:%02d %s%s", day, hour, minute,
						 hour24 < 12 ? "am" : "pm", online ? "   online" : "");
			lettering.drawInked(top, width - 10 * density, 6 * density, 12 * density,
								client::Color{255, 255, 255, 190}, client::Face::Body,
								client::Align::Right, 1);
		}

		// The counter, small and out of the way: it is for me, not for playing.
		char counter[128];
		if (map.open()) {
			// Where you are is map business: on the map screen it is how you
			// read the map, and in play it is a number nobody needs.
			SDL_snprintf(counter, sizeof(counter), "%.0f fps   %.0f, %.0f   zoom %.2f", fps,
						 player.x, player.y, zoom);
		} else {
			SDL_snprintf(counter, sizeof(counter), "%.0f fps   zoom %.2f", fps, zoom);
		}
		lettering.drawInked(counter, 10 * density, 6 * density, 12 * density,
							client::Color{255, 255, 255, 190}, client::Face::Body,
							client::Align::Left, 1);

		if (benchFrames > 0) {
			// The time it takes to build a frame, which is what the rewrite is
			// judged on. Waiting for the display is not work, and on this Mac
			// the present is pinned to the panel's 120 Hz whatever vsync is
			// set to, so counting it would measure the monitor.
			const double built = static_cast<double>(SDL_GetPerformanceCounter() - now) /
								 static_cast<double>(SDL_GetPerformanceFrequency());
			benchTime += built;
			benchWorst = std::max(benchWorst, built);
			if (--framesLeft <= 0) {
				std::printf("bench: %d frames, %.2f ms to build each, worst %.2f ms\n", benchFrames,
							benchTime / benchFrames * 1000, benchWorst * 1000);
				running = false;
			}
		}

		if (shotPath) {
			SDL_Surface* frame = SDL_RenderReadPixels(renderer, nullptr);
			if (frame) {
				SDL_SaveBMP(frame, shotPath);
				SDL_DestroySurface(frame);
			}
			running = false;
		}

		{
			// The cursor, over everything: a dot with the pen round it, so it
			// reads on snow and on grass and on a dark panel alike.
			float mx = 0;
			float my = 0;
			SDL_GetMouseState(&mx, &my);
			const float cx = mx * static_cast<float>(density);
			const float cy = my * static_cast<float>(density);
			const float dot = 3.0f * static_cast<float>(density);
			paint.useWorldScale(1);
			paint.fillCircle(cx, cy, dot + 1.5f * static_cast<float>(density), client::kInk);
			paint.fillCircle(cx, cy, dot, client::rgb(0xf4f1e6));
		}

		lettering.endFrame();
		SDL_RenderPresent(renderer);
	}

	if (!online && !asGenerated) {
		// On the way out, so quitting never costs you the last twenty seconds.
		sim::Session session;
		session.seed = seed;
		session.clock = clock;
		session.player = player;
		session.inventory = inventory;
		sim::saveSession(savePath, session, world, build);
	}

	audio.close();
	lettering.close();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
