#include "net_client.hpp"

#include <algorithm>
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
            Snapshot shot;
            shot.tick = in.u32();
            in.u32();  // the input the server has seen, for dropping what it has
            serverX_ = in.f32();
            serverY_ = in.f32();
            team_ = in.u8();
            const std::uint16_t count = in.u16();
            for (std::uint16_t i = 0; i < count && in.ok(); ++i) {
                Pose pose;
                pose.id = in.u16();
                pose.x = in.f32();
                pose.y = in.f32();
                pose.aim = in.f32();
                const std::uint8_t flags = in.u8();
                pose.alive = (flags & NetPlayerFlags::kAlive) != 0;
                pose.sprinting = (flags & NetPlayerFlags::kSprinting) != 0;
                pose.swimming = (flags & NetPlayerFlags::kSwimming) != 0;
                pose.team = in.u8();
                shot.poses.push_back(pose);
                // Anyone in a snapshot is somebody we know about, even before
                // there are two states to draw them between.
                Other& other = others_[pose.id];
                other.id = pose.id;
            }
            if (!in.ok()) break;
            // Kept in order, and only a dozen: a burst of jitter must not be
            // able to empty the buffer, and nothing older is any use.
            snapshots_.push_back(std::move(shot));
            std::sort(snapshots_.begin(), snapshots_.end(),
                      [](const Snapshot& a, const Snapshot& b) { return a.tick < b.tick; });
            if (snapshots_.size() > static_cast<std::size_t>(kSnapshotBuffer)) {
                snapshots_.erase(snapshots_.begin());
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
            inviteLeft_ = 20;
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
    if (inviteLeft_ > 0) {
        inviteLeft_ -= dt;
        if (inviteLeft_ <= 0) inviteFrom_ = 0;
    }
    // Everyone else is drawn a hundred milliseconds in the past, between the
    // two states either side of that moment. Drawing whatever arrived last has
    // them stepping thirty times a second while your own survivor glides.
    if (snapshots_.size() < 2) return;
    const double newest = static_cast<double>(snapshots_.back().tick);
    const double want = newest - kInterpolationDelay * kTickHz;
    if (!playoutStarted_) {
        playout_ = want;
        playoutStarted_ = true;
    } else {
        playout_ += dt * kTickHz;
        // Steered gently towards where it should be rather than snapped: the
        // arrival times carry the network's jitter, and following them makes
        // everybody stretch and squeeze as they walk.
        const double drift = want - playout_;
        if (std::abs(drift) > 12) {
            playout_ = want;
        } else {
            playout_ += drift * 0.08;
        }
    }

    const Snapshot* before = nullptr;
    const Snapshot* after = nullptr;
    for (const Snapshot& shot : snapshots_) {
        if (static_cast<double>(shot.tick) <= playout_) before = &shot;
        if (!after && static_cast<double>(shot.tick) >= playout_) after = &shot;
    }
    if (!before) before = &snapshots_.front();
    if (!after) after = &snapshots_.back();
    const double span = static_cast<double>(after->tick) - before->tick;
    const double t = span > 0 ? std::clamp((playout_ - before->tick) / span, 0.0, 1.0) : 0.0;

    for (auto& [id, other] : others_) {
        const Pose* from = nullptr;
        const Pose* to = nullptr;
        for (const Pose& pose : before->poses) {
            if (pose.id == id) from = &pose;
        }
        for (const Pose& pose : after->poses) {
            if (pose.id == id) to = &pose;
        }
        if (!from && !to) continue;
        if (!from) from = to;
        if (!to) to = from;
        const double wasX = other.drawX;
        const double wasY = other.drawY;
        other.drawX = from->x + (to->x - from->x) * t;
        other.drawY = from->y + (to->y - from->y) * t;
        other.x = to->x;
        other.y = to->y;
        other.aim = to->aim;
        other.alive = to->alive;
        other.sprinting = to->sprinting;
        other.swimming = to->swimming;
        other.team = to->team;
        // No walk cycle comes over the wire, so their feet move as they do.
        other.walkPhase += std::hypot(other.drawX - wasX, other.drawY - wasY) * 0.08;
    }
}

}  // namespace client
