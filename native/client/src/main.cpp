#include <SDL3/SDL.h>

#include <cstdio>

#include "sim/rng.hpp"
#include "sim/world_size.hpp"

/**
 * The skeleton of the client: a window, an event loop and a frame clock.
 *
 * The island, the drawing and the rest arrive on top of this. What is here is
 * the shape everything else hangs off, and the proof that the three pieces
 * (this, the shared rules, SDL) build and link into one binary on both Mac and
 * Windows.
 */
namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

}  // namespace

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL would not start: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Oxide", kWindowWidth, kWindowHeight,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &window, &renderer)) {
        std::fprintf(stderr, "No window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // Wait for the display rather than spinning: the old game capped itself at
    // 120 by hand because the browser gave it no say in this.
    SDL_SetRenderVSync(renderer, 1);

    // Proof the shared rules are linked in and give the same answers they will
    // give on the server: one island's worth of numbers from one seed.
    sim::Rng rng(20736);
    std::printf("Oxide: island %d x %d, %d x %d biome tiles, first roll %.6f\n", sim::kWorldWidth,
                sim::kWorldHeight, sim::kBiomeCols, sim::kBiomeRows, rng.unit());

    bool running = true;
    std::uint64_t last = SDL_GetPerformanceCounter();
    double clock = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running = false;
        }

        const std::uint64_t now = SDL_GetPerformanceCounter();
        const double dt =
            static_cast<double>(now - last) / static_cast<double>(SDL_GetPerformanceFrequency());
        last = now;
        clock += dt;

        // Placeholder: the island's own green, and something moving on it so a
        // dropped frame is visible at a glance.
        SDL_SetRenderDrawColor(renderer, 0x6c, 0x9c, 0x3b, 0xff);
        SDL_RenderClear(renderer);

        int width = 0;
        int height = 0;
        SDL_GetRenderOutputSize(renderer, &width, &height);
        const float size = 60.0f;
        const SDL_FRect box{
            static_cast<float>(width) * 0.5f + static_cast<float>(SDL_cos(clock)) * 200.0f - size / 2,
            static_cast<float>(height) * 0.5f + static_cast<float>(SDL_sin(clock)) * 120.0f - size / 2,
            size, size};
        SDL_SetRenderDrawColor(renderer, 0x14, 0x11, 0x0d, 0xff);
        SDL_RenderFillRect(renderer, &box);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
