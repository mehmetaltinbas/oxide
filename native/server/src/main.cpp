#include <enet/enet.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "sim/rng.hpp"
#include "sim/world_size.hpp"

/**
 * The skeleton of the server: a socket, a fixed tick, and a count of who is
 * connected.
 *
 * ENet rather than WebSocket and JSON, because a game wants unreliable
 * channels for the things it repeats (where everyone is) and reliable ones for
 * the things it says once (a wall went up). The old server sent every player's
 * position to every player as text thirty times a second, which is the wall
 * this rewrite exists to get past.
 */
namespace {

constexpr int kPort = 8787;
constexpr int kTickHz = 30;
constexpr int kMaxPlayers = 128;
/** Where everyone is, and the things said once: two channels, as ENet counts them. */
constexpr int kChannels = 2;

}  // namespace

int main(int, char**) {
    // A line at a time, so the log says something useful while it runs rather
    // than only when it stops. It will be read through docker or systemd.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    if (enet_initialize() != 0) {
        std::fprintf(stderr, "ENet would not start\n");
        return 1;
    }

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = kPort;
    ENetHost* host = enet_host_create(&address, kMaxPlayers, kChannels, 0, 0);
    if (host == nullptr) {
        std::fprintf(stderr, "Nothing listening on %d: is it already in use?\n", kPort);
        enet_deinitialize();
        return 1;
    }

    sim::Rng rng(20736);
    std::printf("Oxide server on %d, %d Hz, island %d x %d, first roll %.6f\n", kPort, kTickHz,
                sim::kWorldWidth, sim::kWorldHeight, rng.unit());

    const auto step = std::chrono::microseconds(1000000 / kTickHz);
    auto next = std::chrono::steady_clock::now();
    int players = 0;
    bool running = true;
    while (running) {
        ENetEvent event;
        // Drain the socket without blocking: the tick sets the pace, not the
        // network.
        while (enet_host_service(host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    ++players;
                    std::printf("A player connected, %d on the island\n", players);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    --players;
                    std::printf("A player left, %d on the island\n", players);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            }
        }

        // One step of the world goes here.

        next += step;
        std::this_thread::sleep_until(next);
    }

    enet_host_destroy(host);
    enet_deinitialize();
    return 0;
}
