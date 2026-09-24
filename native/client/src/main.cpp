#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "animal.hpp"
#include "built.hpp"
#include "deploy_draw.hpp"
#include "monument_draw.hpp"
#include "held.hpp"
#include "hud.hpp"
#include "map_screen.hpp"
#include "net_client.hpp"
#include "panel.hpp"
#include "human.hpp"
#include "paint.hpp"
#include "particles.hpp"
#include "palette.hpp"
#include "sim/action.hpp"
#include "sim/blast.hpp"
#include "sim/build.hpp"
#include "sim/craft.hpp"
#include "sim/save.hpp"
#include "sim/daylight.hpp"
#include "sim/deployable.hpp"
#include "sim/survival.hpp"
#include "sim/inventory.hpp"
#include "sim/npcs.hpp"
#include "sim/projectile.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"
#include "sprites.hpp"
#include "terrain.hpp"

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
constexpr double kZoomMin = 0.82;
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

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Oxide", kWindowWidth, kWindowHeight,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window,
                                     &renderer)) {
        std::fprintf(stderr, "No window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // Wait for the display rather than spinning: the old game capped itself at
    // 120 by hand because the browser gave it no say in this.
    SDL_SetRenderVSync(renderer, 1);

    std::uint32_t seed = 12345u;
    // A frame straight to a file and then out again, so the look of the game
    // can be checked without a pair of eyes at the window.
    const char* shotPath = nullptr;
    double startX = -1;
    double startY = -1;
    int benchFrames = 0;
    // A swing frozen part way through, for checking how a blow is drawn.
    double poseSwing = -1;
    /** Armed to the teeth, for trying the guns before crafting exists. */
    bool armed = false;
    /** The pack open with something in it, for a look at the screen itself. */
    bool showPanel = false;
    /** The island's own map, opened for a look at it. */
    bool showMap = false;
    /** Dropped in at midnight, for a look at the dark. */
    bool startAtNight = false;
    /** Nothing to worry about and everything to hand, for trying things out. */
    bool sandbox = false;
    /** Where to play: nowhere is this machine, a host is somebody's island. */
    std::string connectTo;
    std::string playerName = "survivor";
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
        } else if (SDL_strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            playerName = argv[++i];
        } else if (SDL_strcmp(argv[i], "--sandbox") == 0) {
            sandbox = true;
        } else if (SDL_strcmp(argv[i], "--night") == 0) {
            startAtNight = true;
        } else if (SDL_strcmp(argv[i], "--map") == 0) {
            showMap = true;
        } else if (SDL_strcmp(argv[i], "--panel") == 0) {
            showPanel = true;
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
    // Online, the island is whichever one the server is running: the seed
    // comes down the wire and the same generator builds it here.
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
            std::fprintf(stderr, "Could not join %s: %s\n", connectTo.c_str(), problem.c_str());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        online = true;
        seed = net.seed();
        std::printf("Joined %s as %u on island %u\n", connectTo.c_str(), net.id(), seed);
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

    sim::BuildSystem build;
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

    sim::NpcSystem npcs;
    npcs.populate(world, seed);
    npcs.garrison(world, seed);

    sim::Inventory inventory;
    // Noon when you arrive, so the first thing you see is the place rather
    // than the dark; a save brings its own hour with it.
    double startClock = sim::kDaySeconds * 0.5;

    // Picking up where the last sitting left off, unless this is an island
    // somebody else is running.
    bool loaded = false;
    if (!online && !sandbox) {
        sim::Session session;
        if (sim::loadSession(savePath, session, world, build) && session.seed == seed) {
            player = session.player;
            inventory = session.inventory;
            startClock = session.clock;
            loaded = true;
            std::printf("Carried on from %s\n", savePath.c_str());
        }
    }

    if (poseSwing >= 0) {
        inventory.hotbar()[1] = sim::ItemStack{sim::ItemId::Hatchet, 1};
        inventory.selectSlot(1);
    }
    if (sandbox) {
        // Every bench to hand and a bag of everything, so the whole game can
        // be looked at without playing through it first.
        inventory.add(sim::ItemId::Workbench3, 1);
        inventory.add(sim::ItemId::BuildingPlan, 1);
        inventory.add(sim::ItemId::Hatchet, 1);
        inventory.add(sim::ItemId::Wood, 5000);
        inventory.add(sim::ItemId::Stone, 5000);
        inventory.add(sim::ItemId::Metal, 5000);
        inventory.add(sim::ItemId::Scrap, 2000);
        inventory.add(sim::ItemId::Cloth, 1000);
        inventory.add(sim::ItemId::Gunpowder, 2000);
    }
    if (armed) {
        inventory.hotbar()[1] = sim::ItemStack{sim::ItemId::Hatchet, 1};
        inventory.hotbar()[2] = sim::ItemStack{sim::ItemId::Bow, 1};
        inventory.hotbar()[3] = sim::ItemStack{sim::ItemId::Ak47, 1};
        inventory.hotbar()[4] = sim::ItemStack{sim::ItemId::PumpShotgun, 1};
        inventory.add(sim::ItemId::Arrow, 40);
        inventory.add(sim::ItemId::RifleAmmo, 120);
        inventory.add(sim::ItemId::ShotgunShell, 40);
        inventory.hotbar()[5] = sim::ItemStack{sim::ItemId::RocketLauncher, 1};
        inventory.add(sim::ItemId::Rocket, 6);
        inventory.add(sim::ItemId::C4, 3);
        inventory.selectSlot(3);
    }
    if (showBase) {
        // Two by two, walled in, with a doorway at the front and a door in it,
        // put up a few cells away so you can see it from outside.
        const int gx = static_cast<int>(player.x / sim::kBuildCell) + 3;
        const int gy = static_cast<int>(player.y / sim::kBuildCell) + 2;
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
        inventory.add(sim::ItemId::BuildingPlan, 1);
        // Something of everything, to see how it all sits together.
        const int fireId = build.deploy(sim::DeployKind::Campfire, gx, gy + 1, 0);
        const int furnaceId = build.deploy(sim::DeployKind::Furnace, gx + 1, gy + 1, 0);
        build.deploy(sim::DeployKind::WoodenBox, gx, gy, 0);
        build.deploy(sim::DeployKind::ToolCupboard, gx + 1, gy, 0);
        build.deploy(sim::DeployKind::SleepingBag, gx + 2, gy, 0);
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
    sim::Explosives explosives;
    sim::Crafting crafting;
    client::Hud hud;
    client::Particles specks;
    client::Panel panel;
    client::MapScreen map(renderer);
    if (loaded) hud.notify("Carried on where you left off.");
    if (showMap) map.toggle();
    // What the building plan would put down, cycled with B.
    sim::BuildKind buildKind = sim::BuildKind::Foundation;
    if (showPanel) {
        inventory.add(sim::ItemId::Wood, 320);
        inventory.add(sim::ItemId::Stone, 180);
        inventory.add(sim::ItemId::Cloth, 60);
        inventory.add(sim::ItemId::Leather, 24);
        inventory.add(sim::ItemId::Scrap, 12);
        panel.toggle();
        crafting.queue(inventory, sim::recipes()[0], 0);
    }

    client::Terrain terrain(renderer);
    client::Sprites sprites(renderer);
    client::Paint paint(renderer);

    if (benchFrames > 0 && !SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED)) {
        std::printf("bench: could not turn vsync off: %s\n", SDL_GetError());
    }

    // The last frame's size and density, so a click knows what it landed on.
    int lastWidth = kWindowWidth;
    int lastHeight = kWindowHeight;
    double lastDensity = 1;

    /** Whether the death screen is up, and whether space has been pressed. */
    bool showHelp = false;
    /** Typing a line of chat, and what has been typed so far. */
    bool typing = false;
    std::string typed;
    bool dead = false;
    bool wantsRespawn = false;

    // The island's own clock, carried over from the last sitting.
    std::uint32_t inputSeq = 0;
    double sinceSave = 0;
    std::size_t chatSeen = 0;

    double clock = startClock;
    if (startAtNight) clock = 0;

    double zoom = 1.0;
    int framesLeft = benchFrames;
    double benchTime = 0;
    double benchWorst = 0;
    bool running = true;
    std::uint64_t last = SDL_GetPerformanceCounter();
    std::vector<const sim::ResourceNode*> visible;
    double fpsClock = 0;
    int fpsFrames = 0;
    double fps = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running = false;
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                zoom = std::clamp(zoom * (1 + event.wheel.y * 0.1), kZoomMin, kZoomMax);
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key >= SDLK_1 &&
                event.key.key <= SDLK_6) {
                inventory.selectSlot(static_cast<int>(event.key.key - SDLK_1));
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
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_T && online &&
                !event.key.repeat) {
                typing = true;
                SDL_StartTextInput(window);
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F && online &&
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
                    hud.notify("Asked them to team up");
                } else {
                    hud.notify("Nobody near enough to ask");
                }
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Y && online &&
                net.inviteFrom() != 0 && !event.key.repeat) {
                net.sendInviteReply(net.inviteFrom(), true);
                net.clearInvite();
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_H && !event.key.repeat) {
                showHelp = !showHelp;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE && dead) {
                wantsRespawn = true;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_M && !event.key.repeat) {
                map.toggle();
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB &&
                !event.key.repeat) {
                panel.toggle();
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && panel.open()) {
                panel.click(inventory, crafting, event.button.x * static_cast<float>(lastDensity),
                            event.button.y * static_cast<float>(lastDensity),
                            event.button.button == SDL_BUTTON_RIGHT, lastWidth, lastHeight,
                            static_cast<float>(lastDensity));
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_B && !event.key.repeat) {
                // Foundation, wall, doorway, door, and round again.
                buildKind = static_cast<sim::BuildKind>(
                    (static_cast<int>(buildKind) + 1) % 4);
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
                        hud.say(thing->lit ? "Lit" : "Out", thing->x, thing->y - 20,
                                client::rgb(0xffd98a));
                    }
                    if (!thing->container.slots.empty()) {
                        panel.openContainer(&thing->container,
                                            sim::itemDef(sim::itemOf(thing->kind)).name, thing);
                    } else if (thing->kind == sim::DeployKind::SleepingBag) {
                        hud.say("Your bag", thing->x, thing->y - 20, client::rgb(0xefeadd));
                    }
                } else if (sim::drink(world, player)) {
                    hud.say("drank", player.x, player.y - 26, client::rgb(0x5aa8d8));
                }
                if (sim::Structure* door = build.nearest(player.x, player.y, sim::kBuildCell * 0.9)) {
                    if (door->kind == sim::BuildKind::Door) {
                        if (door->locked && door->owner != 0) {
                            hud.say("Locked. You will have to break it.", player.x, player.y - 26,
                                    client::rgb(0xd8483a));
                        } else if (online) {
                            net.sendDoor(door->id, !door->open);
                        } else {
                            door->open = !door->open;
                        }
                    }
                }
                const sim::PickResult got = sim::pickUp(world, player, inventory);
                if (got.picked && got.stack.count > 0) {
                    hud.say(std::string("+") + std::to_string(got.stack.count) + " " +
                                sim::itemDef(got.stack.id).name,
                            got.x, got.y, client::rgb(0xefeadd));
                }
            }
        }

        const std::uint64_t now = SDL_GetPerformanceCounter();
        double dt = static_cast<double>(now - last) / static_cast<double>(SDL_GetPerformanceFrequency());
        last = now;
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
        const double scale = zoom * density;
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
        // The mouse is in window points; the frame may be in denser pixels.
        input.aim = SDL_atan2(mouseY * density - height * 0.5, mouseX * density - width * 0.5);
        const double cursorX = player.x + (mouseX * density - width * 0.5) / scale;
        const double cursorY = player.y + (mouseY * density - height * 0.5) / scale;

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
            if (!refused.empty()) hud.notify(refused);
            while (chatSeen < net.chat().size()) hud.notify(net.chat()[chatSeen++]);
        }
        world.update(dt);
        hud.update(dt);
        const sim::NpcEvents animals = npcs.update(world, build, projectiles, dt, player);
        // Nothing warms you yet: the campfire arrives with the deployables.
        build.updateDeployables(dt);
        specks.update(dt);
        if (!online && !sandbox) {
            sinceSave += dt;
            if (sinceSave > 20) {
                sinceSave = 0;
                sim::Session session{seed, clock, player, inventory};
                sim::saveSession(savePath, session, world, build);
            }
        }
        // Embers off anything burning, so a fire looks alight from across a field.
        for (const sim::Deployable& thing : build.deployables()) {
            if (!thing.lit) continue;
            if (SDL_randf() < dt * 20) specks.ember(thing.x, thing.y - 4);
        }
        clock += dt;
        const double dark = sim::darkness(clock);
        sim::updateSurvival(world, player, inventory, dt, sim::nightness(clock),
                            build.warmthAt(player.x, player.y));
        if (sandbox) {
            // Topped up every tick, so the bars read normally and nothing else
            // in the game has to know this mode exists.
            player.health = sim::PlayerVitals::kMaxHealth;
            player.calories = sim::PlayerVitals::kMaxCalories;
            player.hydration = sim::PlayerVitals::kMaxHydration;
            player.temperature = sim::PlayerVitals::kComfortTemp;
            player.radiation = 0;
            player.bleeding = 0;
            player.alive = true;
        }
        sim::updateUse(player, inventory, dt, player.sprinting);

        if (!player.alive && !dead) {
            // Dead: the screen waits for you rather than snatching you back,
            // and everything you were carrying stays where it fell.
            dead = true;
            panel.close();
            map.close();
        }
        if (dead && wantsRespawn) {
            wantsRespawn = false;
            dead = false;
            // Your own bag if you put one down, a beach if you did not.
            const sim::Deployable* bag = nullptr;
            for (const sim::Deployable& thing : build.deployables()) {
                if (thing.kind == sim::DeployKind::SleepingBag && thing.owner == 0) bag = &thing;
            }
            dropIn(world, player, static_cast<std::uint32_t>(SDL_GetTicks()));
            if (bag) {
                player.x = bag->x;
                player.y = bag->y;
            }
            inventory = sim::Inventory();
            hud.notify(bag ? "Woke up in your bag." : "Woke up on the beach with nothing.");
        }
        if (animals.playerDamage > 0) {
            sim::hurtPlayer(player, inventory, animals.playerDamage);
            specks.burst(player.x, player.y, 8, client::rgb(0xc22b2b), 160, 0.45, 2.8);
            specks.shake(6, 0.25);
            hud.say("-" + std::to_string(static_cast<int>(animals.playerDamage)), player.x,
                    player.y - 22, client::rgb(0xd8483a));
        }

        if (poseSwing >= 0) {
            player.swingLength = 0.34;
            player.swingAnim = 0.34 * (1 - poseSwing);
            player.aim = 0;
        }

        sim::tickReload(player, inventory, dt);
        crafting.update(dt, inventory);
        build.update(dt);

        // Held down: a blow or a shot goes out whenever the last one has come
        // round. A bow is the exception: it draws while held and looses when
        // the button comes up.
        const SDL_MouseButtonFlags buttons =
            panel.open() || map.open() || dead ? 0 : SDL_GetMouseState(nullptr, nullptr);
        const sim::Gun& heldGun = sim::itemDef(inventory.held()).gun;
        const bool gun = heldGun.damage > 0;
        // A bow is drawn with the right button, as it is in Rust: the left one
        // is the trigger of everything that has one.
        const bool bow = gun && heldGun.magazine <= 0;
        const bool trigger = (buttons & (bow ? SDL_BUTTON_RMASK : SDL_BUTTON_LMASK)) != 0;
        if (gun) {
            const sim::FireResult shot = sim::fire(player, inventory, projectiles, trigger, dt);
            if (shot.fired) {
                // The flash at the muzzle, and a nudge on the camera for the
                // bigger guns.
                const double mx = player.x + SDL_cos(shot.angle) * 20;
                const double my = player.y + SDL_sin(shot.angle) * 20;
                specks.burst(mx, my, 6, client::rgb(0xffd98a), 220, 0.12, 2.6, 0, shot.angle, 0.7);
                specks.shake(sim::itemDef(shot.gun).gun.damage > 50 ? 3.5 : 2.0, 0.12);
            }
            if (shot.empty && trigger) {
                hud.say("Reload  (R)", player.x, player.y - 26, client::rgb(0xd8483a));
            }
        }
        explosives.update(world, build, npcs, player, inventory, dt);
        for (const sim::BulletHit& hit : projectiles.update(world, npcs, build, dt, &player)) {
            if (hit.npc) specks.burst(hit.x, hit.y, 7, client::rgb(0x8c1f1f), 170, 0.4, 2.6);
            if (hit.node || hit.built) {
                specks.burst(hit.x, hit.y, 5, client::rgb(0x6b7a5c), 120, 0.3, 2.0, 180);
            }
            if (hit.rocket) {
                specks.burst(hit.x, hit.y, 46, client::rgb(0xff8c2e), 300, 1.0, 5.0);
                specks.burst(hit.x, hit.y, 24, client::rgb(0xffd98a), 200, 0.7, 3.6);
                specks.shake(14, 0.5);
            }
            if (hit.rocket) {
                // A rocket takes the piece it struck and everything round it.
                explosives.detonate(world, build, npcs, player, inventory, hit.x, hit.y,
                                    hit.blastRadius, hit.blastDamage, hit.builtId, true);
            }
            if (hit.player) {
                sim::hurtPlayer(player, inventory, hit.damage);
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
        sim::DeployKind deployKind = sim::DeployKind::Campfire;
        const bool deploying = sim::deployableOf(inHand, deployKind);
        const bool planning = inHand == sim::ItemId::BuildingPlan;
        client::BuildTarget target = client::targetAt(cursorX, cursorY, buildKind);
        const char* refusal = nullptr;
        if (planning) {
            refusal = buildKind == sim::BuildKind::Foundation
                          ? build.refuseFoundation(world, target.gx, target.gy, 0)
                          : build.refuseEdge(world, target.gx, target.gy, target.side, buildKind, 0);
            // Out of reach is a refusal like any other: you build what you can
            // put a hand on.
            const double reach = sim::kBuildCell * 1.8;
            if (!refusal && SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
                                     (cursorY - player.y) * (cursorY - player.y)) > reach) {
                refusal = "Too far away";
            }
            const sim::TierDef& twig = sim::tierDef(sim::BuildTier::Twig);
            if (!refusal && inventory.count(twig.cost.id) < twig.cost.count) {
                refusal = "Not enough wood";
            }
        }

        const int deployGx = static_cast<int>(SDL_floor(cursorX / sim::kBuildCell));
        const int deployGy = static_cast<int>(SDL_floor(cursorY / sim::kBuildCell));
        const char* deployRefusal = nullptr;
        if (deploying) {
            deployRefusal = build.refuseDeploy(world, deployGx, deployGy, deployKind, 0);
            if (!deployRefusal &&
                SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
                         (cursorY - player.y) * (cursorY - player.y)) > sim::kBuildCell * 1.8) {
                deployRefusal = "Too far away";
            }
            if ((buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
                if (deployRefusal) {
                    hud.say(deployRefusal, player.x, player.y - 26, client::rgb(0xd8483a));
                } else if (inventory.take(inHand, 1) > 0) {
                    build.deploy(deployKind, deployGx, deployGy, 0);
                }
                player.attackTimer = 0.4;
            }
        }

        if (planning && !refusal && (buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
            const sim::TierDef& twig = sim::tierDef(sim::BuildTier::Twig);
            inventory.take(twig.cost.id, twig.cost.count);
            if (online) {
                // The server decides: a wall exists once everybody has been
                // told about it, not the moment you clicked.
                net.sendBuild(buildKind, target.gx, target.gy, target.side);
            } else if (buildKind == sim::BuildKind::Foundation) {
                build.placeFoundation(target.gx, target.gy, 0);
            } else if (buildKind == sim::BuildKind::Door) {
                // A door goes into the doorway that is already there.
                if (sim::Structure* doorway = build.edgeAt(target.gx, target.gy, target.side)) {
                    doorway->kind = sim::BuildKind::Door;
                    doorway->open = false;
                }
            } else {
                build.placeEdge(target.gx, target.gy, target.side, buildKind, 0);
            }
            player.attackTimer = 0.25;
        } else if (planning && refusal && (buttons & SDL_BUTTON_LMASK) != 0 &&
                   player.attackTimer <= 0) {
            hud.say(refusal, player.x, player.y - 26, client::rgb(0xd8483a));
            player.attackTimer = 0.4;
        }

        if (inHand == sim::ItemId::Lock && (buttons & SDL_BUTTON_LMASK) != 0 &&
            player.attackTimer <= 0) {
            // A lock goes on a door of your own, and on nothing else.
            sim::Structure* door = build.nearest(cursorX, cursorY, sim::kBuildCell * 0.6);
            if (door && door->kind == sim::BuildKind::Door && door->owner == 0 && !door->locked &&
                inventory.take(sim::ItemId::Lock, 1) > 0) {
                door->locked = true;
                hud.say("Locked", cursorX, cursorY, client::rgb(0xefeadd));
            } else {
                hud.say("Locks go on your own doors", player.x, player.y - 26,
                        client::rgb(0xd8483a));
            }
            player.attackTimer = 0.4;
        }

        if (inHand == sim::ItemId::Hammer && (buttons & SDL_BUTTON_LMASK) != 0 &&
            player.attackTimer <= 0) {
            // The hammer takes a piece up a tier rather than knocking it down.
            if (sim::Structure* piece = build.nearest(cursorX, cursorY, sim::kBuildCell * 0.6)) {
                sim::BuildTier up = sim::BuildTier::Twig;
                if (!build.nextTier(*piece, up)) {
                    hud.say("Already sheet metal", player.x, player.y - 26, client::rgb(0xd8483a));
                } else if (build.upgrade(*piece, inventory)) {
                    hud.say(sim::tierDef(up).name, cursorX, cursorY, client::rgb(0xefeadd));
                } else {
                    const sim::Cost& cost = sim::tierDef(up).cost;
                    hud.say(std::string("Needs ") + std::to_string(cost.count) + " " +
                                sim::itemDef(cost.id).name,
                            player.x, player.y - 26, client::rgb(0xd8483a));
                }
                player.attackTimer = 0.5;
            }
        }

        if (sim::itemDef(inHand).category == sim::ItemCategory::Clothing &&
            (buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
            // Worn rather than held: what was on before goes back in the pack.
            const sim::ItemStack was = inventory.worn();
            if (inventory.take(inHand, 1) > 0) {
                inventory.worn() = sim::ItemStack{inHand, 1};
                if (was.id != sim::ItemId::None) inventory.add(was.id, was.count);
                hud.say(std::string("Wearing ") + sim::itemDef(inHand).name, player.x,
                        player.y - 26, client::rgb(0xefeadd));
            }
            player.attackTimer = 0.4;
        }

        if (sim::itemDef(inHand).category == sim::ItemCategory::Explosive &&
            (buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
            // Placed against whatever is nearest where you clicked, which is
            // what makes a charge a raiding tool rather than a grenade.
            const double reach = sim::kBuildCell * 1.6;
            if (SDL_sqrt((cursorX - player.x) * (cursorX - player.x) +
                         (cursorY - player.y) * (cursorY - player.y)) > reach) {
                hud.say("Too far away", player.x, player.y - 26, client::rgb(0xd8483a));
            } else if (inventory.take(inHand, 1) > 0) {
                const sim::Structure* against = build.nearest(cursorX, cursorY, 30);
                explosives.place(inHand, cursorX, cursorY, against ? against->id : 0);
                hud.say(sim::itemDef(inHand).name, cursorX, cursorY, client::rgb(0xff6b4a));
            }
            player.attackTimer = 0.5;
        }

        const bool eating = sim::itemDef(inHand).category == sim::ItemCategory::Consumable;
        if (eating && (buttons & SDL_BUTTON_LMASK) != 0 && player.attackTimer <= 0) {
            if (sim::consume(player, inventory, inHand)) {
                hud.say(sim::itemDef(inHand).name, player.x, player.y - 26, client::rgb(0x8cf08c));
            }
        }

        if (!gun && !planning && !eating && inHand != sim::ItemId::Hammer &&
            (buttons & SDL_BUTTON_LMASK) != 0) {
            const sim::SwingResult blow = sim::swing(world, npcs, player, inventory);
            if (blow.landed && blow.gained.count > 0) {
                hud.say(std::string("+") + std::to_string(blow.gained.count) + " " +
                            sim::itemDef(blow.gained.id).name,
                        blow.x, blow.y - 10, client::rgb(0xefeadd));
            }
            if (blow.landed && !blow.hitNpc) {
                specks.burst(blow.x, blow.y - 4, 6, client::nodeColor(blow.kind), 130, 0.4, 2.4,
                             220);
            }
            if (blow.hitNpc) {
                specks.burst(blow.x, blow.y, 6, client::rgb(0x8c1f1f), 150, 0.35, 2.5);
            }
            if (blow.hitNpc && blow.killed) {
                hud.say(std::string("Killed a ") + sim::npcDef(blow.npcKind).name, blow.x, blow.y - 22,
                        client::rgb(0xefeadd));
            } else if (blow.hitNpc) {
                hud.say("-" + std::to_string(static_cast<int>(blow.damage)), blow.x, blow.y - 16,
                        client::rgb(0xffd9d9));
            }
            if (blow.packFull) hud.say("Pack full", player.x, player.y - 24, client::rgb(0xd8483a));
        }

        SDL_SetRenderDrawColor(renderer, client::kVoid.r, client::kVoid.g, client::kVoid.b, 255);
        SDL_RenderClear(renderer);
        double shakeX = 0;
        double shakeY = 0;
        specks.shakeOffset(shakeX, shakeY);
        const double camX = player.x + shakeX;
        const double camY = player.y + shakeY;
        terrain.draw(world, camX, camY, scale, width, height);

        // The looting places are ground: they go down before anything stands
        // on them.
        for (const sim::Monument& monument : world.monuments()) {
            if (SDL_fabs(monument.x - player.x) > width / (2 * scale) + monument.radius) continue;
            if (SDL_fabs(monument.y - player.y) > height / (2 * scale) + monument.radius) continue;
            client::drawMonument(paint, monument, camX, camY, scale, width, height,
                                 static_cast<float>(density));
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

        for (const sim::Dropped& drop : world.drops()) {
            const float sx = static_cast<float>((drop.x - camX) * scale) + width * 0.5f;
            const float sy = static_cast<float>((drop.y - camY) * scale) + height * 0.5f;
            if (sx < -40 || sy < -40 || sx > width + 40 || sy > height + 40) continue;
            client::drawItemIcon(paint, drop.stack.id, sx, sy, static_cast<float>(22 * scale));
        }

        for (const sim::Structure& piece : build.list()) {
            if (piece.kind != sim::BuildKind::Foundation) continue;
            client::drawBuilt(paint, piece, camX, camY, scale, width, height);
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
            look.held = inventory.held();
            // Where the swing has got to, as a fraction of its own length.
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
                client::drawAnimalTag(paint, *animal, ax, ay, static_cast<float>(scale),
                                      static_cast<float>(density));
            }
        };

        for (const sim::ResourceNode* node : visible) {
            drawAnimalsUpTo(node->y);
            if (!playerDrawn && node->y > player.y) drawPlayer();
            const float sx = static_cast<float>((node->x - camX) * scale) + width * 0.5f;
            const float sy = static_cast<float>((node->y - camY) * scale) + height * 0.5f;
            const sim::Biome under = world.biomeAt(node->x, node->y);
            const bool snowy = under == sim::Biome::Snow;
            // The lighter tree of the open grassland; the pines keep to the
            // forest and the snow.
            const bool broadleaf = node->kind == sim::NodeKind::Tree && under == sim::Biome::Grass;
            // A tree you are standing behind goes see-through, so you are not
            // lost under one.
            float alpha = 1.0f;
            if (node->kind == sim::NodeKind::Tree) {
                const double dx = player.x - node->x;
                const double dy = player.y - node->y;
                if (std::abs(dx) < node->radius * 1.8 && dy < node->radius * 0.6 &&
                    dy > -node->radius * 3.5) {
                    alpha = 0.45f;
                }
            }
            sprites.draw(*node, sx, sy, static_cast<float>(scale), snowy, broadleaf, alpha);
        }
        drawAnimalsUpTo(1e9);
        for (const auto& [id, other] : net.others()) {
            if (!other.alive) continue;
            const float ox = static_cast<float>((other.drawX - camX) * scale) + width * 0.5f;
            const float oy = static_cast<float>((other.drawY - camY) * scale) + height * 0.5f;
            if (ox < -80 || oy < -80 || ox > width + 80 || oy > height + 80) continue;
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
                                  5 * static_cast<float>(density), client::rgb(0x5fb85f),
                                  client::kInkFine);
            }
        }
        if (!playerDrawn) drawPlayer();

        for (const sim::Deployable& thing : build.deployables()) {
            if (thing.kind == sim::DeployKind::SleepingBag) continue;
            const float sx = static_cast<float>((thing.x - camX) * scale) + width * 0.5f;
            const float sy = static_cast<float>((thing.y - camY) * scale) + height * 0.5f;
            client::drawDeployable(paint, thing, sx, sy, static_cast<float>(scale));
        }

        // A roof over every sealed room but the one you are in: a base is a
        // thing you cannot see into, which is most of what makes one worth
        // building.
        const int myRegion = build.regionAt(player.x, player.y);
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
            client::drawBuilt(paint, piece, camX, camY, scale, width, height);
        }
        if (planning) {
            client::drawGhost(paint, target, sim::BuildTier::Twig, refusal == nullptr, camX, camY,
                              scale, width, height);
        }
        if (deploying) {
            // The cell it would sit on, rather than a ghost of the thing: what
            // matters is which square it takes.
            const float gx =
                static_cast<float>((deployGx * sim::kBuildCell - camX) * scale) + width * 0.5f;
            const float gy =
                static_cast<float>((deployGy * sim::kBuildCell - camY) * scale) + height * 0.5f;
            const float size = static_cast<float>(sim::kBuildCell * scale);
            paint.fillRect(gx, gy, size, size,
                           deployRefusal ? client::Color{224, 80, 60, 90}
                                         : client::Color{124, 200, 255, 90});
        }

        for (const sim::Bullet& bullet : projectiles.list()) {
            const float bx = static_cast<float>((bullet.x - camX) * scale) + width * 0.5f;
            const float by = static_cast<float>((bullet.y - camY) * scale) + height * 0.5f;
            const double speed = SDL_sqrt(bullet.vx * bullet.vx + bullet.vy * bullet.vy);
            // A streak behind the round, as long as it travels in a blink.
            const float tail = static_cast<float>(speed * 0.02 * scale);
            const float ux = static_cast<float>(bullet.vx / speed);
            const float uy = static_cast<float>(bullet.vy / speed);
            if (bullet.arrow) {
                paint.line(bx - ux * tail, by - uy * tail, bx, by, 2.2f * static_cast<float>(scale),
                           client::rgb(0x8a5a2e));
            } else {
                // Fading out at the back, so it reads as a thing in flight
                // rather than a stick lying across the ground.
                paint.line(bx - ux * tail, by - uy * tail, bx - ux * tail * 0.4f,
                           by - uy * tail * 0.4f, 1.6f * static_cast<float>(scale),
                           client::Color{255, 241, 168, 90});
                paint.line(bx - ux * tail * 0.4f, by - uy * tail * 0.4f, bx, by,
                           2.0f * static_cast<float>(scale), client::rgb(0xfff1a8));
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
            // Night is laid over the world, and every fire cuts a hole in it.
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MUL);
            SDL_SetRenderDrawColorFloat(renderer, 1.0f - static_cast<float>(dark) * 0.94f,
                                        1.0f - static_cast<float>(dark) * 0.9f,
                                        1.0f - static_cast<float>(dark) * 0.78f, 1.0f);
            const SDL_FRect all{0, 0, static_cast<float>(width), static_cast<float>(height)};
            SDL_RenderFillRect(renderer, &all);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            const auto glow = [&](double wx, double wy, double radius) {
                const float sx = static_cast<float>((wx - camX) * scale) + width * 0.5f;
                const float sy = static_cast<float>((wy - camY) * scale) + height * 0.5f;
                const float r = static_cast<float>(radius * scale);
                // Rings rather than a gradient: SDL has no radial fill, and a
                // dozen fading rings read as one soft light.
                for (int i = 18; i >= 1; --i) {
                    const float t = i / 18.0f;
                    const std::uint8_t a =
                        static_cast<std::uint8_t>(13 * (1 - t) * (1 - t) * dark * 2.2);
                    paint.fillCircle(sx, sy, r * t, client::Color{255, 220, 165, a});
                }
            };
            if (player.alive) glow(player.x, player.y, 170);
            for (const sim::Deployable& thing : build.deployables()) {
                if (thing.lit) glow(thing.x, thing.y, 250);
            }
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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
            double best = sim::kPickReach;
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
                world.nodesInRect(player.x - sim::kPickReach, player.y - sim::kPickReach,
                                  player.x + sim::kPickReach, player.y + sim::kPickReach, visible);
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
            // Fresh water is the last thing offered, being the one you are
            // standing in rather than standing at.
            if (!prompt[0] && world.freshAt(player.x, player.y)) {
                SDL_snprintf(prompt, sizeof(prompt), "E   Drink");
            }
        }
        hud.setVitals(player.calories, player.hydration, player.temperature, player.radiation,
                      player.bleeding > 0,
                      player.useTotal > 0 && player.useLeft > 0 ? player.useLeft / player.useTotal
                                                                : 0);
        hud.setAmmo(sim::itemDef(inventory.held()).gun.damage > 0
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

        map.draw(paint, world, build, player, width, height, static_cast<float>(density));
        panel.setBench(sandbox ? 3 : build.benchTierAt(player.x, player.y, 0));
        panel.draw(paint, inventory, crafting, width, height, static_cast<float>(density));
        if (typing) {
            const float size = 1.8f * static_cast<float>(density);
            const std::string line = "say: " + typed + "_";
            paint.fillRect(0, height - 150 * static_cast<float>(density),
                           static_cast<float>(width), 30 * static_cast<float>(density),
                           client::Color{20, 17, 13, 200});
            SDL_SetRenderScale(renderer, size, size);
            SDL_SetRenderDrawColor(renderer, 232, 226, 212, 255);
            SDL_RenderDebugText(renderer, (20 * density) / size,
                                (height - 142 * density) / size, line.c_str());
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        }

        if (showHelp) {
            static const char* kLines[] = {
                "WASD  move        SHIFT  run",
                "LEFT CLICK  hit, fire, place     RIGHT CLICK  draw a bow",
                "1-6  belt         R  reload       E  use what is in front of you",
                "TAB  pack and bench               M  map",
                "B  what the plan puts down        H  close this",
            };
            const float size = 1.8f * static_cast<float>(density);
            paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
                           client::Color{0, 0, 0, 170});
            SDL_SetRenderScale(renderer, size, size);
            SDL_SetRenderDrawColor(renderer, 232, 226, 212, 255);
            for (int i = 0; i < 5; ++i) {
                SDL_RenderDebugText(renderer, (width * 0.5f - 300 * density) / size,
                                    (height * 0.35f + i * 30 * density) / size, kLines[i]);
            }
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        }

        if (dead) {
            paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
                           client::Color{40, 8, 8, 170});
            const float size = 4.0f * static_cast<float>(density);
            const char* line = "YOU DIED";
            const float textW = static_cast<float>(SDL_strlen(line)) * 8 * size;
            SDL_SetRenderScale(renderer, size, size);
            SDL_SetRenderDrawColor(renderer, 232, 226, 212, 255);
            SDL_RenderDebugText(renderer, (width - textW) * 0.5f / size,
                                (height * 0.4f) / size, line);
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);

            bool hasBag = false;
            for (const sim::Deployable& thing : build.deployables()) {
                if (thing.kind == sim::DeployKind::SleepingBag && thing.owner == 0) hasBag = true;
            }
            const char* how = hasBag ? "SPACE to wake up in your bag"
                                     : "SPACE to wake up on a beach with nothing";
            const float small = 1.8f * static_cast<float>(density);
            const float howW = static_cast<float>(SDL_strlen(how)) * 8 * small;
            SDL_SetRenderScale(renderer, small, small);
            SDL_SetRenderDrawColor(renderer, 216, 72, 58, 255);
            SDL_RenderDebugText(renderer, (width - howW) * 0.5f / small,
                                (height * 0.4f + 70 * static_cast<float>(density)) / small, how);
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        }

        hud.draw(paint, inventory, static_cast<int>(std::lround(player.health)), width, height, panel.open() ? "" : prompt, static_cast<float>(density));

        fpsClock += dt;
        ++fpsFrames;
        if (fpsClock >= 0.5) {
            fps = fpsFrames / fpsClock;
            fpsClock = 0;
            fpsFrames = 0;
        }
        SDL_SetRenderScale(renderer, static_cast<float>(density), static_cast<float>(density));
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugTextFormat(renderer, 10, 10, "%.0f fps  %zu drawn  %.0f, %.0f  zoom %.2f  %02d:%02d",
                                  fps, visible.size(), player.x, player.y, zoom,
                                  static_cast<int>(sim::dayFraction(clock) * 24),
                                  static_cast<int>(SDL_fmod(sim::dayFraction(clock) * 24 * 60, 60)));
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);

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

        SDL_RenderPresent(renderer);
    }

    if (!online && !sandbox && benchFrames <= 0 && !shotPath) {
        // On the way out, so quitting never costs you the last twenty seconds.
        sim::Session session{seed, clock, player, inventory};
        sim::saveSession(savePath, session, world, build);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
