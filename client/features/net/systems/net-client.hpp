#pragma once

#include <enet/enet.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/net/types/protocol.struct.hpp"
#include "sim/features/survival/types/player-input.struct.hpp"
#include "client/features/net/types/other.struct.hpp"
#include "client/features/net/types/room-entry.struct.hpp"
#include "client/features/net/types/snapshot.struct.hpp"

namespace client {

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

	/**
	 * Opens the connection and waits for the lobby. Says whether it worked:
	 * from here the island is chosen rather than given.
	 */
	bool connect(const std::string& host, std::uint16_t port, const std::string& name,
				 std::string& problem);

	/** Reads until the lobby has answered or the wait runs out. */
	bool waitForRooms(double seconds);
	/** Reads until an island has been joined, which brings the seed with it. */
	bool waitForWelcome(double seconds);

	const std::vector<RoomEntry>& rooms() const { return rooms_; }
	void askForRooms();
	void joinRoom(std::uint16_t id);
	void createRoom(const std::string& name, std::uint32_t seed);

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
	std::vector<RoomEntry> rooms_;
	/** The last dozen ticks, and the moment being drawn on our own clock. */
	std::vector<Snapshot> snapshots_;
	double playout_ = 0;
	bool playoutStarted_ = false;
	bool haveRooms_ = false;
	std::string refusal_;
	std::uint16_t inviteFrom_ = 0;
	/** Seconds left to answer it, because an offer does not stand all day. */
	double inviteLeft_ = 0;

	void handle(const std::uint8_t* bytes, std::size_t size, sim::BuildSystem& build);
};

}  // namespace client
