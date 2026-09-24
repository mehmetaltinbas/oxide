#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "animal.hpp"
#include "held.hpp"
#include "hud.hpp"
#include "human.hpp"
#include "paint.hpp"
#include "palette.hpp"
#include "sim/action.hpp"
#include "sim/inventory.hpp"
#include "sim/npcs.hpp"
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
    player.health = 100;
    player.attackTimer = 0;
    player.swingAnim = 0;
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
    for (int i = 1; i < argc; ++i) {
        if (SDL_strcmp(argv[i], "--shot") == 0 && i + 1 < argc) {
            shotPath = argv[++i];
        } else if (SDL_strcmp(argv[i], "--at") == 0 && i + 2 < argc) {
            startX = SDL_atof(argv[++i]);
            startY = SDL_atof(argv[++i]);
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
    sim::World world;
    const std::uint64_t built = SDL_GetTicks();
    world.generate(seed);
    std::printf("Oxide: island %u built in %llu ms, %zu things standing on it\n", seed,
                static_cast<unsigned long long>(SDL_GetTicks() - built), world.nodes().size());

    sim::Player player;
    dropIn(world, player, seed);
    if (startX >= 0) {
        player.x = startX;
        player.y = startY;
    }

    sim::NpcSystem npcs;
    npcs.populate(world, seed);

    sim::Inventory inventory;
    if (poseSwing >= 0) {
        inventory.hotbar()[1] = sim::ItemStack{sim::ItemId::Hatchet, 1};
        inventory.selectSlot(1);
    }
    client::Hud hud;

    client::Terrain terrain(renderer);
    client::Sprites sprites(renderer);
    client::Paint paint(renderer);

    if (benchFrames > 0 && !SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED)) {
        std::printf("bench: could not turn vsync off: %s\n", SDL_GetError());
    }

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
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_E && !event.key.repeat) {
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

        const bool* keys = SDL_GetKeyboardState(nullptr);
        sim::PlayerInput input;
        if (keys[SDL_SCANCODE_W]) input.moveY -= 1;
        if (keys[SDL_SCANCODE_S]) input.moveY += 1;
        if (keys[SDL_SCANCODE_A]) input.moveX -= 1;
        if (keys[SDL_SCANCODE_D]) input.moveX += 1;
        input.sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

        float mouseX = 0;
        float mouseY = 0;
        SDL_GetMouseState(&mouseX, &mouseY);
        // The mouse is in window points; the frame may be in denser pixels.
        input.aim = SDL_atan2(mouseY * density - height * 0.5, mouseX * density - width * 0.5);

        sim::stepPlayer(world, player, input, dt);
        world.update(dt);
        hud.update(dt);
        const sim::NpcEvents animals = npcs.update(world, dt, player);
        if (player.health <= 0) {
            // Dead: you wake on a beach with a rock, and everything you were
            // carrying is gone, exactly as the other game had it.
            dropIn(world, player, static_cast<std::uint32_t>(SDL_GetTicks()));
            inventory = sim::Inventory();
            hud.say("Woke up on the beach with nothing.", player.x, player.y - 30,
                    client::rgb(0xd8483a));
        }
        if (animals.playerDamage > 0) {
            player.health = std::max(0, player.health - static_cast<int>(animals.playerDamage));
            hud.say("-" + std::to_string(static_cast<int>(animals.playerDamage)), player.x,
                    player.y - 22, client::rgb(0xd8483a));
        }

        if (poseSwing >= 0) {
            player.swingLength = 0.34;
            player.swingAnim = 0.34 * (1 - poseSwing);
            player.aim = 0;
        }

        // Held down: a swing goes out whenever the last one has come round.
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
        if ((buttons & SDL_BUTTON_LMASK) != 0) {
            const sim::SwingResult blow = sim::swing(world, npcs, player, inventory);
            if (blow.landed && blow.gained.count > 0) {
                hud.say(std::string("+") + std::to_string(blow.gained.count) + " " +
                            sim::itemDef(blow.gained.id).name,
                        blow.x, blow.y - 10, client::rgb(0xefeadd));
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
        terrain.draw(world, player.x, player.y, scale, width, height);

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
            const float sx = static_cast<float>((drop.x - player.x) * scale) + width * 0.5f;
            const float sy = static_cast<float>((drop.y - player.y) * scale) + height * 0.5f;
            if (sx < -40 || sy < -40 || sx > width + 40 || sy > height + 40) continue;
            client::drawItemIcon(paint, drop.stack.id, sx, sy, static_cast<float>(22 * scale));
        }

        static std::vector<const sim::Npc*> animalsNear;
        npcs.inRect(player.x - halfW - margin, player.y - halfH - margin, player.x + halfW + margin,
                    player.y + halfH + margin, animalsNear);

        bool playerDrawn = false;
        const auto drawPlayer = [&] {
            client::HumanLook look;
            look.x = width * 0.5f;
            look.y = height * 0.5f;
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
                const float ax = static_cast<float>((animal->x - player.x) * scale) + width * 0.5f;
                const float ay = static_cast<float>((animal->y - player.y) * scale) + height * 0.5f;
                client::drawAnimal(paint, *animal, ax, ay, static_cast<float>(scale));
                client::drawAnimalTag(paint, *animal, ax, ay, static_cast<float>(scale),
                                      static_cast<float>(density));
            }
        };

        for (const sim::ResourceNode* node : visible) {
            drawAnimalsUpTo(node->y);
            if (!playerDrawn && node->y > player.y) drawPlayer();
            const float sx = static_cast<float>((node->x - player.x) * scale) + width * 0.5f;
            const float sy = static_cast<float>((node->y - player.y) * scale) + height * 0.5f;
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
        if (!playerDrawn) drawPlayer();

        hud.drawPopups(renderer, player.x, player.y, scale, width, height);

        // What the key under your finger would do, if anything.
        char prompt[96] = {0};
        {
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
        }
        hud.draw(paint, inventory, player.health, width, height, prompt, static_cast<float>(density));

        fpsClock += dt;
        ++fpsFrames;
        if (fpsClock >= 0.5) {
            fps = fpsFrames / fpsClock;
            fpsClock = 0;
            fpsFrames = 0;
        }
        SDL_SetRenderScale(renderer, static_cast<float>(density), static_cast<float>(density));
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugTextFormat(renderer, 10, 10, "%.0f fps  %zu drawn  %.0f, %.0f  zoom %.2f",
                                  fps, visible.size(), player.x, player.y, zoom);
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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
