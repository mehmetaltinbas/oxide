#include <enet/enet.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "sim/build.hpp"
#include "sim/net/protocol.hpp"
#include "sim/npcs.hpp"
#include "sim/player.hpp"
#include "sim/projectile.hpp"
#include "sim/world.hpp"

/**
 * The authority.
 *
 * One island, generated from a seed the clients are told, so nobody is sent a
 * map. Everyone's movement is worked out here with the same code the client
 * runs, which is the whole point of the shared library: the two cannot drift.
 * What is built goes through here as well, so a wall exists only once everyone
 * has been told about it.
 *
 * ENet rather than WebSocket and JSON: unreliable channels for the things that
 * repeat (where everyone is) and reliable ones for the things said once (a wall
 * went up). Each player is sent only what is near them, which is what makes a
 * hundred of them affordable where the old server sent everybody everything.
 */
namespace {

using namespace sim::net;

constexpr int kChannels = 2;
/** Where everyone is, thirty times a second, and never mind if one is lost. */
constexpr int kChannelState = 0;
/** What is said once and must arrive: a wall, a door, a word. */
constexpr int kChannelEvents = 1;

/** One player, as the server knows them. */
struct Client {
    std::uint16_t id = 0;
    ENetPeer* peer = nullptr;
    std::string name = "survivor";
    sim::Player player;
    sim::PlayerInput input;
    /** The last input number they sent, echoed back so they can drop what we have. */
    std::uint32_t ack = 0;
    /** Nought is nobody's team; everyone else on the same number is a mate. */
    std::uint8_t team = 0;
    /** Which island they are on, or nought while they are still choosing. */
    std::uint16_t room = 0;
    bool greeted = false;
};

/**
 * One island, and everyone on it.
 *
 * A server runs several: the lobby lists them, and joining one is what gets
 * you a seed and a place to stand. They are generated on demand rather than up
 * front, because an island nobody asked for is a few megabytes of nothing.
 */
struct Room {
    std::uint16_t id = 0;
    std::string name = "Island";
    std::uint32_t seed = 0;
    sim::World world;
    sim::BuildSystem build;
    sim::NpcSystem npcs;
    sim::Projectiles projectiles;
    int players = 0;
    /** How long it has stood empty, so an abandoned one is given up. */
    double idle = 0;
};

void send(ENetPeer* peer, int channel, const Writer& writer, bool reliable) {
    ENetPacket* packet = enet_packet_create(writer.bytes().data(), writer.size(),
                                            reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(peer, static_cast<enet_uint8>(channel), packet);
}

/** How close you must be to ask someone to team up: speaking distance. */
constexpr double kInviteRange = 120;

}  // namespace

int main(int argc, char** argv) {
    // A line at a time, so the log says something useful while it runs rather
    // than only when it stops. It will be read through docker or systemd.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    std::uint16_t port = kDefaultPort;
    std::uint32_t seed = 12345;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        }
    }

    if (enet_initialize() != 0) {
        std::fprintf(stderr, "ENet would not start\n");
        return 1;
    }

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;
    ENetHost* host = enet_host_create(&address, kMaxPlayers, kChannels, 0, 0);
    if (!host) {
        std::fprintf(stderr, "Nothing listening on %u: is it already in use?\n", port);
        enet_deinitialize();
        return 1;
    }

    std::unordered_map<std::uint16_t, Room> rooms;
    std::uint16_t nextRoom = 1;
    std::unordered_map<ENetPeer*, Client> clients;
    std::uint16_t nextId = 1;
    std::uint8_t nextTeam = 1;

    // The island everyone lands on unless they start another: a server with
    // nothing running is a lobby with nothing in it.
    const auto openRoom = [&](const std::string& name, std::uint32_t withSeed) -> Room* {
        if (rooms.size() >= static_cast<std::size_t>(kMaxRooms)) return nullptr;
        const std::uint16_t id = nextRoom++;
        Room& room = rooms[id];
        room.id = id;
        room.name = name;
        room.seed = withSeed;
        room.world.generate(withSeed);
        room.npcs.populate(room.world, withSeed);
        room.npcs.garrison(room.world, withSeed);
        std::printf("Island %u \"%s\" from seed %u: %zu things on it\n", id, name.c_str(),
                    withSeed, room.world.nodes().size());
        return &room;
    };
    openRoom("Ashvale", seed);

    std::printf("Oxide server: listening on %u\n", port);

    /** Everyone on one island, which is as far as anything here carries. */
    const auto everyone = [&](std::uint16_t room, const Writer& writer, ENetPeer* except) {
        for (auto& [peer, client] : clients) {
            if (peer == except || client.room != room) continue;
            send(peer, kChannelEvents, writer, true);
        }
    };

    const auto roomOf = [&](const Client& client) -> Room* {
        const auto it = rooms.find(client.room);
        return it == rooms.end() ? nullptr : &it->second;
    };

    /** What is running, for the lobby to list. */
    const auto sendRooms = [&](ENetPeer* peer) {
        Writer out;
        out.u8(static_cast<std::uint8_t>(ServerMessage::RoomList));
        out.u8(static_cast<std::uint8_t>(rooms.size()));
        for (const auto& [id, room] : rooms) {
            out.u16(id);
            out.text(room.name);
            out.u8(static_cast<std::uint8_t>(room.players));
            out.u8(static_cast<std::uint8_t>(kMaxPlayersPerRoom));
        }
        send(peer, kChannelEvents, out, true);
    };

    /** Putting somebody on an island: a seed, a beach, and what is built. */
    const auto joinRoom = [&](Client& client, Room& room) {
        client.room = room.id;
        ++room.players;
        room.idle = 0;
        room.world.beachSpawn(client.id * 7919u + room.seed, client.player.x, client.player.y);

        Writer welcome;
        welcome.u8(static_cast<std::uint8_t>(ServerMessage::Welcome));
        welcome.u16(client.id);
        welcome.u32(room.seed);
        welcome.f32(static_cast<float>(client.player.x));
        welcome.f32(static_cast<float>(client.player.y));
        send(client.peer, kChannelEvents, welcome, true);

        for (const sim::Structure& piece : room.build.list()) {
            Writer out;
            out.u8(static_cast<std::uint8_t>(ServerMessage::Piece));
            out.u32(static_cast<std::uint32_t>(piece.id));
            out.u8(static_cast<std::uint8_t>(piece.kind));
            out.u8(static_cast<std::uint8_t>(piece.tier));
            out.i32(piece.gx);
            out.i32(piece.gy);
            out.u8(static_cast<std::uint8_t>(piece.side));
            out.u16(static_cast<std::uint16_t>(piece.owner));
            send(client.peer, kChannelEvents, out, true);
        }

        Writer joined;
        joined.u8(static_cast<std::uint8_t>(ServerMessage::Joined));
        joined.u16(client.id);
        joined.text(client.name);
        everyone(room.id, joined, client.peer);
        for (const auto& [peer, other] : clients) {
            if (peer == client.peer || other.room != room.id) continue;
            Writer who;
            who.u8(static_cast<std::uint8_t>(ServerMessage::Joined));
            who.u16(other.id);
            who.text(other.name);
            send(client.peer, kChannelEvents, who, true);
        }
        std::printf("%s joined island %u\n", client.name.c_str(), room.id);
    };

    const auto leaveRoom = [&](Client& client) {
        Room* room = roomOf(client);
        if (!room) return;
        --room->players;
        Writer out;
        out.u8(static_cast<std::uint8_t>(ServerMessage::Left));
        out.u16(client.id);
        const std::uint16_t was = client.room;
        client.room = 0;
        client.team = 0;
        everyone(was, out, nullptr);
    };

    const auto describePiece = [](const sim::Structure& piece) {
        Writer out;
        out.u8(static_cast<std::uint8_t>(ServerMessage::Piece));
        out.u32(static_cast<std::uint32_t>(piece.id));
        out.u8(static_cast<std::uint8_t>(piece.kind));
        out.u8(static_cast<std::uint8_t>(piece.tier));
        out.i32(piece.gx);
        out.i32(piece.gy);
        out.u8(static_cast<std::uint8_t>(piece.side));
        out.u16(static_cast<std::uint16_t>(piece.owner));
        return out;
    };

    const double step = 1.0 / kTickHz;
    auto next = std::chrono::steady_clock::now();
    bool running = true;
    double clock = 0;
    std::uint32_t tick = 0;

    while (running) {
        ENetEvent event;
        while (enet_host_service(host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    Client client;
                    client.id = nextId++;
                    client.peer = event.peer;
                    clients[event.peer] = client;
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    const auto it = clients.find(event.peer);
                    if (it != clients.end()) {
                        leaveRoom(it->second);
                        clients.erase(it);
                    }
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE: {
                    const auto it = clients.find(event.peer);
                    if (it == clients.end()) {
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    Client& client = it->second;
                    Reader in(event.packet->data, event.packet->dataLength);
                    const ClientMessage kind = static_cast<ClientMessage>(in.u8());
                    switch (kind) {
                        case ClientMessage::Hello: {
                            const std::uint16_t version = in.u16();
                            client.name = in.text();
                            if (client.name.empty()) client.name = "survivor";
                            if (version != kProtocolVersion) {
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Refused));
                                out.text("Client and server versions do not match.");
                                send(event.peer, kChannelEvents, out, true);
                                enet_peer_disconnect_later(event.peer, 0);
                                break;
                            }
                            client.greeted = true;
                            // What is running, so they can choose.
                            sendRooms(event.peer);
                            break;
                        }
                        case ClientMessage::Rooms: {
                            sendRooms(event.peer);
                            break;
                        }
                        case ClientMessage::Create: {
                            if (!client.greeted || client.room != 0) break;
                            std::string name = in.text();
                            if (name.empty()) name = "Island";
                            // A different island each time, unless they gave a
                            // seed of their own.
                            const std::uint32_t withSeed = in.u32();
                            Room* room = openRoom(
                                name, withSeed != 0 ? withSeed
                                                    : static_cast<std::uint32_t>(enet_time_get()) *
                                                          2654435761u);
                            if (!room) {
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Refused));
                                out.text("This server is running as many islands as it will.");
                                send(event.peer, kChannelEvents, out, true);
                                break;
                            }
                            joinRoom(client, *room);
                            break;
                        }
                        case ClientMessage::Join: {
                            if (!client.greeted || client.room != 0) break;
                            const std::uint16_t want = in.u16();
                            const auto it = rooms.find(want);
                            if (it == rooms.end()) {
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Refused));
                                out.text("That island is gone.");
                                send(event.peer, kChannelEvents, out, true);
                                break;
                            }
                            if (it->second.players >= kMaxPlayersPerRoom) {
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Refused));
                                out.text("That island is full.");
                                send(event.peer, kChannelEvents, out, true);
                                break;
                            }
                            joinRoom(client, it->second);
                            break;
                        }
                        case ClientMessage::Leave: {
                            leaveRoom(client);
                            Writer out;
                            out.u8(static_cast<std::uint8_t>(ServerMessage::Left_Room));
                            send(event.peer, kChannelEvents, out, true);
                            sendRooms(event.peer);
                            break;
                        }
                        case ClientMessage::Input: {
                            const std::uint8_t bits = in.u8();
                            client.input.moveX = ((bits & InputBits::kRight) ? 1 : 0) -
                                                 ((bits & InputBits::kLeft) ? 1 : 0);
                            client.input.moveY = ((bits & InputBits::kDown) ? 1 : 0) -
                                                 ((bits & InputBits::kUp) ? 1 : 0);
                            client.input.sprint = (bits & InputBits::kRun) != 0;
                            client.input.aim = in.f32();
                            client.ack = in.u32();
                            break;
                        }
                        case ClientMessage::Build: {
                            Room* room = roomOf(client);
                            if (!room) break;
                            const auto kindByte = static_cast<sim::BuildKind>(in.u8());
                            const int gx = in.i32();
                            const int gy = in.i32();
                            const auto side = static_cast<sim::EdgeSide>(in.u8());
                            const char* refused =
                                kindByte == sim::BuildKind::Foundation
                                    ? room->build.refuseFoundation(room->world, gx, gy, client.id)
                                    : room->build.refuseEdge(room->world, gx, gy, side, kindByte,
                                                             client.id);
                            if (refused) {
                                // The client put this up the moment they
                                // clicked, so it has to be told to take it back
                                // rather than left with a wall nobody else sees.
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Refused));
                                out.text(refused);
                                send(event.peer, kChannelEvents, out, true);
                                break;
                            }
                            const sim::Structure& piece =
                                kindByte == sim::BuildKind::Foundation
                                    ? room->build.placeFoundation(gx, gy, client.id)
                                    : room->build.placeEdge(gx, gy, side, kindByte, client.id,
                                                            sim::BuildTier::Twig, client.player.x,
                                                            client.player.y);
                            everyone(room->id, describePiece(piece), nullptr);
                            break;
                        }
                        case ClientMessage::Door: {
                            Room* room = roomOf(client);
                            if (!room) break;
                            const int id = in.i32();
                            const bool open = in.u8() != 0;
                            for (sim::Structure& piece : room->build.list2()) {
                                if (piece.id != id || piece.kind != sim::BuildKind::Door) continue;
                                piece.open = open;
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Door));
                                out.i32(id);
                                out.u8(open ? 1 : 0);
                                everyone(room->id, out, nullptr);
                                break;
                            }
                            break;
                        }
                        case ClientMessage::Damage: {
                            Room* room = roomOf(client);
                            if (!room) break;
                            const int id = in.i32();
                            // Clamped, because the amount is a client's word for
                            // how hard it hit: combat is not settled here yet,
                            // so one message must not be able to flatten a base.
                            const double amount = std::clamp(static_cast<double>(in.f32()), 0.0, 500.0);
                            for (sim::Structure& piece : room->build.list2()) {
                                if (piece.id != id) continue;
                                if (room->build.damage(piece, amount)) {
                                    Writer out;
                                    out.u8(static_cast<std::uint8_t>(ServerMessage::Destroyed));
                                    out.i32(id);
                                    everyone(room->id, out, nullptr);
                                }
                                break;
                            }
                            break;
                        }
                        case ClientMessage::Chat: {
                            const std::string text = in.text();
                            if (text.empty() || client.room == 0) break;
                            Writer out;
                            out.u8(static_cast<std::uint8_t>(ServerMessage::Chat));
                            out.text(client.name);
                            out.text(text);
                            everyone(client.room, out, nullptr);
                            send(event.peer, kChannelEvents, out, true);
                            break;
                        }
                        case ClientMessage::Invite: {
                            const std::uint16_t to = in.u16();
                            for (auto& [peer, other] : clients) {
                                if (other.id != to || other.room != client.room) continue;
                                // An invite is offered to the person in front of
                                // you, not shouted across the island.
                                if (std::hypot(client.player.x - other.player.x,
                                               client.player.y - other.player.y) > kInviteRange) {
                                    break;
                                }
                                Writer out;
                                out.u8(static_cast<std::uint8_t>(ServerMessage::Invited));
                                out.u16(client.id);
                                out.text(client.name);
                                send(peer, kChannelEvents, out, true);
                                break;
                            }
                            break;
                        }
                        case ClientMessage::InviteReply: {
                            const std::uint16_t from = in.u16();
                            const bool yes = in.u8() != 0;
                            if (!yes) break;
                            for (auto& [peer, other] : clients) {
                                if (other.id != from) continue;
                                // Whoever has a team keeps it; otherwise a new
                                // one is started for the pair of them.
                                std::uint8_t team = other.team ? other.team : client.team;
                                if (!team) team = nextTeam++;
                                other.team = team;
                                client.team = team;
                                for (ENetPeer* who : {peer, event.peer}) {
                                    Writer out;
                                    out.u8(static_cast<std::uint8_t>(ServerMessage::Team));
                                    out.u8(team);
                                    send(who, kChannelEvents, out, true);
                                }
                                break;
                            }
                            break;
                        }
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
                default: break;
            }
        }

        // One step of each island, for everybody on it, with the same code the
        // client runs on its own copy.
        for (auto& [peer, client] : clients) {
            Room* room = roomOf(client);
            if (!room) continue;
            sim::stepPlayer(room->world, room->build, client.player, client.input, step);
        }
        for (auto& [id, room] : rooms) {
            room.world.update(step);
            room.build.update(step);
            room.build.updateDeployables(room.world, step);
            if (room.players > 0) {
                room.idle = 0;
            } else {
                room.idle += step;
            }
        }
        // An island nobody has been on for five minutes is given up, unless it
        // is the one the server started with.
        for (auto it = rooms.begin(); it != rooms.end();) {
            if (it->first != 1 && it->second.players <= 0 && it->second.idle > kIdleRoomSeconds) {
                std::printf("Island %u \"%s\" given up\n", it->first, it->second.name.c_str());
                it = rooms.erase(it);
            } else {
                ++it;
            }
        }
        clock += step;
        ++tick;

        // And what each of them can see of it. Only what is near: a player on
        // the far side of the island is not their business, and not sending it
        // is what makes a crowd affordable.
        for (auto& [peer, client] : clients) {
            if (client.room == 0) continue;
            Writer out;
            out.u8(static_cast<std::uint8_t>(ServerMessage::Snapshot));
            // The tick it belongs to, so a client can lay the states it has in
            // order and draw between them rather than at whatever arrived last.
            out.u32(tick);
            out.u32(client.ack);
            out.f32(static_cast<float>(client.player.x));
            out.f32(static_cast<float>(client.player.y));
            out.u8(client.team);

            std::vector<const Client*> near;
            for (const auto& [otherPeer, other] : clients) {
                if (otherPeer == peer || other.room != client.room) continue;
                if (std::hypot(other.player.x - client.player.x,
                               other.player.y - client.player.y) > kInterestRadius) {
                    continue;
                }
                near.push_back(&other);
            }
            out.u16(static_cast<std::uint16_t>(near.size()));
            for (const Client* other : near) {
                out.u16(other->id);
                out.f32(static_cast<float>(other->player.x));
                out.f32(static_cast<float>(other->player.y));
                out.f32(static_cast<float>(other->player.aim));
                std::uint8_t flags = 0;
                if (other->player.alive) flags |= NetPlayerFlags::kAlive;
                if (other->player.sprinting) flags |= NetPlayerFlags::kSprinting;
                if (other->player.swimming) flags |= NetPlayerFlags::kSwimming;
                out.u8(flags);
                out.u8(other->team);
            }
            send(peer, kChannelState, out, false);
        }
        enet_host_flush(host);

        next += std::chrono::microseconds(static_cast<long long>(step * 1e6));
        std::this_thread::sleep_until(next);
    }

    enet_host_destroy(host);
    enet_deinitialize();
    return 0;
}
