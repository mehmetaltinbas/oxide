#include "net_client.hpp"

#include <cmath>

namespace client {

using namespace sim::net;

NetClient::~NetClient() {
    if (peer_) enet_peer_disconnect_now(peer_, 0);
    if (host_) enet_host_destroy(host_);
}

bool NetClient::connect(const std::string& host, std::uint16_t port, const std::string& name,
                        std::string& problem) {
    if (enet_initialize() != 0) {
        problem = "ENet would not start";
        return false;
    }
    host_ = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host_) {
        problem = "no socket";
        return false;
    }
    ENetAddress address{};
    if (enet_address_set_host(&address, host.c_str()) != 0) {
        problem = "cannot find " + host;
        return false;
    }
    address.port = port;
    peer_ = enet_host_connect(host_, &address, 2, 0);
    if (!peer_) {
        problem = "nothing to connect to";
        return false;
    }

    // The handshake is worth waiting for: there is no island to draw until the
    // server has said which one it is.
    ENetEvent event;
    if (enet_host_service(host_, &event, 4000) <= 0 || event.type != ENET_EVENT_TYPE_CONNECT) {
        problem = "no answer from " + host;
        return false;
    }

    Writer hello;
    hello.u8(static_cast<std::uint8_t>(ClientMessage::Hello));
    hello.u16(kProtocolVersion);
    hello.text(name);
    ENetPacket* packet =
        enet_packet_create(hello.bytes().data(), hello.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
    enet_host_flush(host_);

    if (!waitForRooms(5.0)) {
        problem = refusal_.empty() ? "the server never listed its islands" : refusal_;
        return false;
    }
    return true;
}

bool NetClient::waitForRooms(double seconds) {
    sim::BuildSystem ignored;
    ENetEvent event;
    const std::uint64_t until = enet_time_get() + static_cast<std::uint64_t>(seconds * 1000);
    while (enet_time_get() < until) {
        while (enet_host_service(host_, &event, 50) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                handle(event.packet->data, event.packet->dataLength, ignored);
                enet_packet_destroy(event.packet);
            }
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) return false;
        }
        if (haveRooms_) return true;
    }
    return false;
}

bool NetClient::waitForWelcome(double seconds) {
    sim::BuildSystem ignored;
    ENetEvent event;
    const std::uint64_t until = enet_time_get() + static_cast<std::uint64_t>(seconds * 1000);
    while (enet_time_get() < until) {
        while (enet_host_service(host_, &event, 50) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                handle(event.packet->data, event.packet->dataLength, ignored);
                enet_packet_destroy(event.packet);
            }
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) return false;
        }
        if (joined_) return true;
        if (!refusal_.empty()) return false;
    }
    return false;
}

void NetClient::askForRooms() {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Rooms));
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
    enet_host_flush(host_);
}

void NetClient::joinRoom(std::uint16_t id) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Join));
    out.u16(id);
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
    enet_host_flush(host_);
}

void NetClient::createRoom(const std::string& name, std::uint32_t seed) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Create));
    out.text(name);
    out.u32(seed);
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
    enet_host_flush(host_);
}

std::string NetClient::takeRefusal() {
    std::string out;
    out.swap(refusal_);
    return out;
}

void NetClient::sendInput(const sim::PlayerInput& input, std::uint32_t seq) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Input));
    std::uint8_t bits = 0;
    if (input.moveY < 0) bits |= InputBits::kUp;
    if (input.moveY > 0) bits |= InputBits::kDown;
    if (input.moveX < 0) bits |= InputBits::kLeft;
    if (input.moveX > 0) bits |= InputBits::kRight;
    if (input.sprint) bits |= InputBits::kRun;
    out.u8(bits);
    out.f32(static_cast<float>(input.aim));
    out.u32(seq);
    // Sent unreliably: another one is along in a thirtieth of a second.
    ENetPacket* packet = enet_packet_create(out.bytes().data(), out.size(), 0);
    enet_peer_send(peer_, 0, packet);
}

void NetClient::sendBuild(sim::BuildKind kind, int gx, int gy, sim::EdgeSide side) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Build));
    out.u8(static_cast<std::uint8_t>(kind));
    out.i32(gx);
    out.i32(gy);
    out.u8(static_cast<std::uint8_t>(side));
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
}

void NetClient::sendDoor(int id, bool open) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Door));
    out.i32(id);
    out.u8(open ? 1 : 0);
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
}

void NetClient::sendDamage(int id, double amount) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Damage));
    out.i32(id);
    out.f32(static_cast<float>(amount));
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
}

void NetClient::sendChat(const std::string& text) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Chat));
    out.text(text);
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
}

void NetClient::sendInvite(std::uint16_t to) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::Invite));
    out.u16(to);
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
}

void NetClient::sendInviteReply(std::uint16_t from, bool yes) {
    if (!peer_) return;
    Writer out;
    out.u8(static_cast<std::uint8_t>(ClientMessage::InviteReply));
    out.u16(from);
    out.u8(yes ? 1 : 0);
    ENetPacket* packet =
        enet_packet_create(out.bytes().data(), out.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, 1, packet);
}

void NetClient::handle(const std::uint8_t* bytes, std::size_t size, sim::BuildSystem& build) {
    Reader in(bytes, size);
    switch (static_cast<ServerMessage>(in.u8())) {
        case ServerMessage::Welcome: {
            id_ = in.u16();
            seed_ = in.u32();
            startX_ = in.f32();
            startY_ = in.f32();
            serverX_ = startX_;
            serverY_ = startY_;
            joined_ = true;
            break;
        }
        case ServerMessage::Snapshot: {
            in.u32();  // the input the server has seen, for dropping what it has
            serverX_ = in.f32();
            serverY_ = in.f32();
            team_ = in.u8();
            const std::uint16_t count = in.u16();
            // Anyone not in this snapshot is out of sight rather than gone, so
            // they are kept until the server says they left.
            for (std::uint16_t i = 0; i < count && in.ok(); ++i) {
                const std::uint16_t id = in.u16();
                Other& other = others_[id];
                other.id = id;
                const double wasX = other.x;
                const double wasY = other.y;
                other.x = in.f32();
                other.y = in.f32();
                other.aim = in.f32();
                const std::uint8_t flags = in.u8();
                other.alive = (flags & NetPlayerFlags::kAlive) != 0;
                other.sprinting = (flags & NetPlayerFlags::kSprinting) != 0;
                other.swimming = (flags & NetPlayerFlags::kSwimming) != 0;
                other.team = in.u8();
                // No walk cycle comes over the wire, so their feet move as they
                // do: the stride is read off how far they travelled.
                other.walkPhase += std::hypot(other.x - wasX, other.y - wasY) * 0.08;
            }
            break;
        }
        case ServerMessage::Piece: {
            const int id = static_cast<int>(in.u32());
            const auto kind = static_cast<sim::BuildKind>(in.u8());
            const auto tier = static_cast<sim::BuildTier>(in.u8());
            const int gx = in.i32();
            const int gy = in.i32();
            const auto side = static_cast<sim::EdgeSide>(in.u8());
            const int owner = in.u16();
            if (!in.ok()) break;
            sim::Structure& piece = kind == sim::BuildKind::Foundation
                                        ? build.placeFoundation(gx, gy, owner, tier)
                                        : build.placeEdge(gx, gy, side, kind, owner, tier);
            // The server's id, so a later message about this piece finds it.
            piece.id = id;
            break;
        }
        case ServerMessage::Destroyed: {
            const int id = in.i32();
            for (sim::Structure& piece : build.list2()) {
                if (piece.id == id) build.damage(piece, piece.hp);
            }
            break;
        }
        case ServerMessage::Door: {
            const int id = in.i32();
            const bool open = in.u8() != 0;
            for (sim::Structure& piece : build.list2()) {
                if (piece.id == id) piece.open = open;
            }
            break;
        }
        case ServerMessage::Chat: {
            const std::string who = in.text();
            const std::string what = in.text();
            chat_.push_back(who + ": " + what);
            if (chat_.size() > 40) chat_.erase(chat_.begin());
            break;
        }
        case ServerMessage::Joined: {
            const std::uint16_t id = in.u16();
            Other& other = others_[id];
            other.id = id;
            other.name = in.text();
            chat_.push_back(other.name + " joined");
            break;
        }
        case ServerMessage::Left: {
            const std::uint16_t id = in.u16();
            const auto it = others_.find(id);
            if (it != others_.end()) {
                chat_.push_back(it->second.name + " left");
                others_.erase(it);
            }
            break;
        }
        case ServerMessage::Refused: refusal_ = in.text(); break;
        case ServerMessage::RoomList: {
            rooms_.clear();
            const std::uint8_t count = in.u8();
            for (std::uint8_t i = 0; i < count && in.ok(); ++i) {
                RoomEntry entry;
                entry.id = in.u16();
                entry.name = in.text();
                entry.players = in.u8();
                entry.max = in.u8();
                rooms_.push_back(entry);
            }
            haveRooms_ = true;
            break;
        }
        case ServerMessage::Left_Room: {
            joined_ = false;
            others_.clear();
            break;
        }
        case ServerMessage::Team: team_ = in.u8(); break;
        case ServerMessage::Invited: {
            inviteFrom_ = in.u16();
            chat_.push_back(in.text() + " asked you to team up   (Y to accept)");
            break;
        }
    }
}

void NetClient::poll(sim::BuildSystem& build, double dt) {
    if (!host_) return;
    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            handle(event.packet->data, event.packet->dataLength, build);
            enet_packet_destroy(event.packet);
        }
        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            joined_ = false;
            chat_.push_back("disconnected");
        }
    }
    // Everyone else glides towards where they were last said to be, rather
    // than stepping thirty times a second.
    for (auto& [id, other] : others_) {
        const double blend = std::min(1.0, dt * 12);
        other.drawX += (other.x - other.drawX) * blend;
        other.drawY += (other.y - other.drawY) * blend;
    }
}

}  // namespace client
