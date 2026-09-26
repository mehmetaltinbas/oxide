#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace sim::net {

/**
 * The wire.
 *
 * Binary rather than JSON, and fixed-shape rather than self-describing: a
 * snapshot of two dozen players is a few hundred bytes here where the old game
 * sent several kilobytes of text thirty times a second to everybody.
 *
 * Bumped whenever the shape of anything below changes, because a client and a
 * server that disagree about the shape of a message are worse than one that
 * refuses to connect.
 */
inline constexpr std::uint16_t kProtocolVersion = 3;

/** The port the server listens on unless told otherwise. */
inline constexpr std::uint16_t kDefaultPort = 8787;

/** How often the server steps and speaks. */
inline constexpr int kTickHz = 30;

/**
 * How far in the past everyone else is drawn, in seconds.
 *
 * The server sends thirty states a second, so an update is up to 33ms stale
 * before the next arrives, and drawing whatever came last has everybody else
 * stepping while your own survivor, predicted locally, glides. Holding the
 * picture back three ticks means there is nearly always a state either side of
 * the moment being drawn, so it can be smoothed between them.
 */
inline constexpr double kInterpolationDelay = 0.1;
/** How many states to keep, enough that a burst of jitter cannot empty it. */
inline constexpr int kSnapshotBuffer = 12;

/** How far a player is told about: what is beyond this is not their business. */
inline constexpr double kInterestRadius = 2400;

/** How many may share one island. */
inline constexpr int kMaxPlayers = 100;

/** From the client. */
enum class ClientMessage : std::uint8_t {
    Hello = 1,
    Input = 2,
    Build = 3,
    Door = 4,
    Damage = 5,
    Chat = 6,
    Invite = 7,
    InviteReply = 8,
    /** The lobby: what islands are running, start one, join one, leave one. */
    Rooms = 9,
    Create = 10,
    Join = 11,
    Leave = 12,
};

/** From the server. */
enum class ServerMessage : std::uint8_t {
    Welcome = 1,
    Snapshot = 2,
    Piece = 3,
    Destroyed = 4,
    Door = 5,
    Chat = 6,
    Joined = 7,
    Left = 8,
    Refused = 9,
    Team = 10,
    Invited = 11,
    /** The lobby's answer: every island running, and who is on it. */
    RoomList = 12,
    Left_Room = 13,
};

/** How many islands one server will run at once. */
inline constexpr int kMaxRooms = 8;
/** How many may share one island. */
inline constexpr int kMaxPlayersPerRoom = 24;
/** An island with nobody on it is given up after this long. */
inline constexpr double kIdleRoomSeconds = 300;

/** What a player is asking to do, as it goes over the wire. */
struct InputBits {
    static constexpr std::uint8_t kUp = 1 << 0;
    static constexpr std::uint8_t kDown = 1 << 1;
    static constexpr std::uint8_t kLeft = 1 << 2;
    static constexpr std::uint8_t kRight = 1 << 3;
    static constexpr std::uint8_t kRun = 1 << 4;
};

/** What a player looks like to everyone else. */
struct NetPlayerFlags {
    static constexpr std::uint8_t kAlive = 1 << 0;
    static constexpr std::uint8_t kSprinting = 1 << 1;
    static constexpr std::uint8_t kSwimming = 1 << 2;
};

/** Bytes out. Little endian, because every machine this runs on is. */
class Writer {
public:
    void u8(std::uint8_t v) { bytes_.push_back(v); }
    void u16(std::uint16_t v) { raw(&v, sizeof(v)); }
    void u32(std::uint32_t v) { raw(&v, sizeof(v)); }
    void i32(std::int32_t v) { raw(&v, sizeof(v)); }
    void f32(float v) { raw(&v, sizeof(v)); }
    void text(const std::string& v) {
        const std::uint8_t length = static_cast<std::uint8_t>(v.size() > 255 ? 255 : v.size());
        u8(length);
        bytes_.insert(bytes_.end(), v.begin(), v.begin() + length);
    }

    const std::vector<std::uint8_t>& bytes() const { return bytes_; }
    std::size_t size() const { return bytes_.size(); }

private:
    std::vector<std::uint8_t> bytes_;

    void raw(const void* from, std::size_t size) {
        const std::uint8_t* at = static_cast<const std::uint8_t*>(from);
        bytes_.insert(bytes_.end(), at, at + size);
    }
};

/**
 * Bytes in.
 *
 * Every read is bounds-checked and a short message simply reads as zeros with
 * `ok()` false afterwards: the other end of a socket is not to be trusted, and
 * a malformed packet must not be able to walk off the end of a buffer.
 */
class Reader {
public:
    Reader(const std::uint8_t* bytes, std::size_t size) : bytes_(bytes), size_(size) {}

    std::uint8_t u8() {
        std::uint8_t v = 0;
        raw(&v, sizeof(v));
        return v;
    }
    std::uint16_t u16() {
        std::uint16_t v = 0;
        raw(&v, sizeof(v));
        return v;
    }
    std::uint32_t u32() {
        std::uint32_t v = 0;
        raw(&v, sizeof(v));
        return v;
    }
    std::int32_t i32() {
        std::int32_t v = 0;
        raw(&v, sizeof(v));
        return v;
    }
    float f32() {
        float v = 0;
        raw(&v, sizeof(v));
        return v;
    }
    std::string text() {
        const std::uint8_t length = u8();
        std::string out;
        for (std::uint8_t i = 0; i < length; ++i) out.push_back(static_cast<char>(u8()));
        return out;
    }

    bool ok() const { return ok_; }
    std::size_t left() const { return size_ - at_; }

private:
    const std::uint8_t* bytes_;
    std::size_t size_;
    std::size_t at_ = 0;
    bool ok_ = true;

    void raw(void* into, std::size_t size) {
        if (at_ + size > size_) {
            ok_ = false;
            std::memset(into, 0, size);
            return;
        }
        std::memcpy(into, bytes_ + at_, size);
        at_ += size;
    }
};

}  // namespace sim::net
