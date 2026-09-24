#pragma once

#include <enet/enet.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "sim/build.hpp"
#include "sim/net/protocol.hpp"
#include "sim/player.hpp"

namespace client {

/** Somebody else on the island, as the server last described them. */
struct Other {
    std::uint16_t id = 0;
    std::string name = "survivor";
    double x = 0;
    double y = 0;
    /** Where they were a moment ago, so they glide rather than step. */
    double drawX = 0;
    double drawY = 0;
    double aim = 0;
    bool alive = true;
    bool sprinting = false;
    bool swimming = false;
    std::uint8_t team = 0;
    /** How far through a stride they look to be, worked out from how they move. */
    double walkPhase = 0;
};

/**
 * The other end of the wire.
 *
 * Owns nothing about the game but who else is here and what has been built:
 * the island itself is generated from the seed the server sends, so nobody is
 * ever sent a map. Movement is predicted locally and corrected towards what
 * the server says, which is what keeps walking about feeling immediate on a
 * connection that is not.
 */
class NetClient {
public:
    ~NetClient();

    /** Opens the connection and waits for the welcome. Says whether it worked. */
    bool connect(const std::string& host, std::uint16_t port, const std::string& name,
                 std::string& problem);

    bool connected() const { return host_ != nullptr && joined_; }
    std::uint32_t seed() const { return seed_; }
    std::uint16_t id() const { return id_; }
    std::uint8_t team() const { return team_; }
    double startX() const { return startX_; }
    double startY() const { return startY_; }

    const std::unordered_map<std::uint16_t, Other>& others() const { return others_; }
    const std::vector<std::string>& chat() const { return chat_; }
    /** Whatever the server last refused, for the interface to say out loud. */
    std::string takeRefusal();

    /** Where the server says you are, for pulling your own guess back to it. */
    double serverX() const { return serverX_; }
    double serverY() const { return serverY_; }

    void sendInput(const sim::PlayerInput& input, std::uint32_t seq);
    void sendBuild(sim::BuildKind kind, int gx, int gy, sim::EdgeSide side);
    void sendDoor(int id, bool open);
    void sendDamage(int id, double amount);
    void sendChat(const std::string& text);
    void sendInvite(std::uint16_t to);
    void sendInviteReply(std::uint16_t from, bool yes);

    /** Who last asked you to team up, and nought if nobody has. */
    std::uint16_t inviteFrom() const { return inviteFrom_; }
    void clearInvite() { inviteFrom_ = 0; }

    /** Reads what has arrived and applies it. Pieces land in `build`. */
    void poll(sim::BuildSystem& build, double dt);

private:
    ENetHost* host_ = nullptr;
    ENetPeer* peer_ = nullptr;
    bool joined_ = false;
    std::uint16_t id_ = 0;
    std::uint32_t seed_ = 0;
    std::uint8_t team_ = 0;
    double startX_ = 0;
    double startY_ = 0;
    double serverX_ = 0;
    double serverY_ = 0;
    std::unordered_map<std::uint16_t, Other> others_;
    std::vector<std::string> chat_;
    std::string refusal_;
    std::uint16_t inviteFrom_ = 0;

    void handle(const std::uint8_t* bytes, std::size_t size, sim::BuildSystem& build);
};

}  // namespace client
