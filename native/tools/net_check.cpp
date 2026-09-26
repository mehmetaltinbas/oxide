// Two players on one island, with nobody watching: both connect, both walk,
// and each is checked for whether the other showed up in their snapshots and
// whether a wall one of them put up reached the other.
#include <enet/enet.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "sim/net/protocol.hpp"

using namespace sim::net;

namespace {

struct Fake {
    ENetHost* host = nullptr;
    std::uint16_t room = 0;
    ENetPeer* peer = nullptr;
    std::uint16_t id = 0;
    std::uint32_t seed = 0;
    double x = 0;
    double y = 0;
    int sawOthers = 0;
    int pieces = 0;
    std::string refusal;
};

void send(Fake& fake, const Writer& out, bool reliable) {
    ENetPacket* packet = enet_packet_create(out.bytes().data(), out.size(),
                                            reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(fake.peer, reliable ? 1 : 0, packet);
}

bool open(Fake& fake, std::uint16_t port, const char* name) {
    fake.host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!fake.host) return false;
    ENetAddress address{};
    enet_address_set_host(&address, "127.0.0.1");
    address.port = port;
    fake.peer = enet_host_connect(fake.host, &address, 2, 0);
    ENetEvent event;
    if (enet_host_service(fake.host, &event, 2000) <= 0 || event.type != ENET_EVENT_TYPE_CONNECT) {
        return false;
    }
    Writer hello;
    hello.u8(static_cast<std::uint8_t>(ClientMessage::Hello));
    hello.u16(kProtocolVersion);
    hello.text(name);
    send(fake, hello, true);
    enet_host_flush(fake.host);
    return true;
}

/** The lobby's answer, and landing on the first island it lists. */
void joinFirst(Fake& fake) {
    Writer join;
    join.u8(static_cast<std::uint8_t>(ClientMessage::Join));
    join.u16(fake.room);
    send(fake, join, true);
    enet_host_flush(fake.host);
}

void read(Fake& fake) {
    ENetEvent event;
    while (enet_host_service(fake.host, &event, 1) > 0) {
        if (event.type != ENET_EVENT_TYPE_RECEIVE) continue;
        Reader in(event.packet->data, event.packet->dataLength);
        switch (static_cast<ServerMessage>(in.u8())) {
            case ServerMessage::Welcome:
                fake.id = in.u16();
                fake.seed = in.u32();
                fake.x = in.f32();
                fake.y = in.f32();
                break;
            case ServerMessage::Snapshot: {
                in.u32();  // the tick it belongs to
                in.u32();
                fake.x = in.f32();
                fake.y = in.f32();
                in.u8();
                fake.sawOthers += in.u16();
                break;
            }
            case ServerMessage::Piece: ++fake.pieces; break;
            case ServerMessage::RoomList: {
                const std::uint8_t count = in.u8();
                if (count > 0) {
                    fake.room = in.u16();
                    in.text();
                    in.u8();
                    in.u8();
                }
                break;
            }
            case ServerMessage::Refused: fake.refusal = in.text(); break;
            default: break;
        }
        enet_packet_destroy(event.packet);
    }
}

void walk(Fake& fake, std::uint8_t bits, std::uint32_t seq) {
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Input));
    out.u8(bits);
    out.f32(0);
    out.u32(seq);
    send(fake, out, false);
    enet_host_flush(fake.host);
}

}  // namespace

int main(int argc, char** argv) {
    const std::uint16_t port =
        argc > 1 ? static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10)) : kDefaultPort;
    if (enet_initialize() != 0) {
        std::printf("net: ENet would not start\n");
        return 1;
    }
    Fake a;
    Fake b;
    if (!open(a, port, "ada") || !open(b, port, "bo")) {
        std::printf("net: no server on %u\n", port);
        return 1;
    }

    // The lobby answers, and both land on the island it lists.
    for (int frame = 0; frame < 60; ++frame) {
        read(a);
        read(b);
        enet_host_service(a.host, nullptr, 8);
    }
    joinFirst(a);
    joinFirst(b);

    // Settle, so both know where they woke up.
    for (int frame = 0; frame < 30; ++frame) {
        walk(a, 0, frame);
        walk(b, 0, frame);
        read(a);
        read(b);
    }

    // Then each walks at the other until they are within sight of one another.
    // The test knows where both are; neither of them does, which is the point.
    const auto toward = [](const Fake& from, const Fake& to) {
        std::uint8_t bits = InputBits::kRun;
        if (to.x > from.x + 20) bits |= InputBits::kRight;
        if (to.x < from.x - 20) bits |= InputBits::kLeft;
        if (to.y > from.y + 20) bits |= InputBits::kDown;
        if (to.y < from.y - 20) bits |= InputBits::kUp;
        return bits;
    };
    // Paced at roughly the server's own tick: sending faster than it steps
    // only fills its queue, and the walk still takes as long as it takes.
    for (int frame = 0; frame < 9000; ++frame) {
        walk(a, toward(a, b), frame);
        walk(b, toward(b, a), frame);
        read(a);
        read(b);
        enet_host_service(a.host, nullptr, 8);
        if (a.sawOthers > 30 && b.sawOthers > 30) break;
    }

    const int gx = static_cast<int>(a.x / 64);
    const int gy = static_cast<int>(a.y / 64);
    Writer build;
    build.u8(static_cast<std::uint8_t>(ClientMessage::Build));
    build.u8(0);  // a foundation
    build.i32(gx);
    build.i32(gy);
    build.u8(0);
    send(a, build, true);
    enet_host_flush(a.host);
    for (int frame = 0; frame < 60; ++frame) {
        read(a);
        read(b);
    }

    std::printf("net: island %u, ada %u at %.0f,%.0f, %.0f apart, saw %d, bo saw %d, pieces a %d b %d%s%s\n",
                a.seed, a.id, a.x, a.y, std::hypot(a.x - b.x, a.y - b.y), a.sawOthers, b.sawOthers,
                a.pieces, b.pieces,
                a.refusal.empty() ? "" : ", refused: ", a.refusal.c_str());
    const bool good =
        a.seed != 0 && a.id != 0 && a.pieces > 0 && b.pieces > 0 && a.sawOthers > 0 && b.sawOthers > 0;
    enet_host_destroy(a.host);
    enet_host_destroy(b.host);
    enet_deinitialize();
    return good ? 0 : 1;
}
