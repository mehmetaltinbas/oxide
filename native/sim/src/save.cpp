#include "sim/save.hpp"

#include <cstdio>
#include <vector>

#include "sim/net/protocol.hpp"

namespace sim {

namespace {

/** Bumped whenever the shape below changes, so an old save is refused. */
constexpr std::uint32_t kSaveVersion = 1;
constexpr std::uint32_t kMagic = 0x4f584944;  // "OXID"

void writeStack(net::Writer& out, const ItemStack& stack) {
    out.u8(static_cast<std::uint8_t>(stack.id));
    out.u16(static_cast<std::uint16_t>(stack.count));
}

ItemStack readStack(net::Reader& in) {
    ItemStack stack;
    stack.id = static_cast<ItemId>(in.u8());
    stack.count = in.u16();
    return stack;
}

}  // namespace

bool saveSession(const std::string& path, const Session& session, const World& world,
                 const BuildSystem& build) {
    net::Writer out;
    out.u32(kMagic);
    out.u32(kSaveVersion);
    out.u32(session.seed);
    out.f32(static_cast<float>(session.clock));

    const Player& player = session.player;
    out.f32(static_cast<float>(player.x));
    out.f32(static_cast<float>(player.y));
    out.f32(static_cast<float>(player.aim));
    out.f32(static_cast<float>(player.health));
    out.f32(static_cast<float>(player.calories));
    out.f32(static_cast<float>(player.hydration));
    out.f32(static_cast<float>(player.temperature));
    out.f32(static_cast<float>(player.radiation));
    out.u8(player.alive ? 1 : 0);

    for (const ItemStack& stack : session.inventory.hotbar()) writeStack(out, stack);
    for (const ItemStack& stack : session.inventory.pack()) writeStack(out, stack);
    writeStack(out, session.inventory.worn());
    out.u8(static_cast<std::uint8_t>(session.inventory.activeSlot()));

    // Only what differs from a fresh island: a felled tree, a broken node.
    std::vector<const ResourceNode*> changed;
    for (const ResourceNode& node : world.nodes()) {
        if (node.hp != node.maxHp || node.respawn > 0) changed.push_back(&node);
    }
    out.u32(static_cast<std::uint32_t>(changed.size()));
    for (const ResourceNode* node : changed) {
        out.u32(static_cast<std::uint32_t>(node->id));
        out.u16(static_cast<std::uint16_t>(node->hp));
        out.f32(static_cast<float>(node->respawn));
    }

    std::vector<const LootCrate*> looted;
    for (const LootCrate& crate : world.crates()) {
        if (crate.looted) looted.push_back(&crate);
    }
    out.u32(static_cast<std::uint32_t>(looted.size()));
    for (const LootCrate* crate : looted) {
        out.u32(static_cast<std::uint32_t>(crate->id));
        out.f32(static_cast<float>(crate->respawn));
    }

    out.u32(static_cast<std::uint32_t>(build.list().size()));
    for (const Structure& piece : build.list()) {
        out.u32(static_cast<std::uint32_t>(piece.id));
        out.u8(static_cast<std::uint8_t>(piece.kind));
        out.u8(static_cast<std::uint8_t>(piece.tier));
        out.i32(piece.gx);
        out.i32(piece.gy);
        out.u8(static_cast<std::uint8_t>(piece.side));
        out.u16(static_cast<std::uint16_t>(piece.hp));
        out.u16(static_cast<std::uint16_t>(piece.maxHp));
        out.u16(static_cast<std::uint16_t>(piece.owner));
        out.u8(piece.open ? 1 : 0);
        out.u8(piece.locked ? 1 : 0);
    }

    out.u32(static_cast<std::uint32_t>(build.deployables().size()));
    for (const Deployable& thing : build.deployables()) {
        out.u8(static_cast<std::uint8_t>(thing.kind));
        out.f32(static_cast<float>(thing.x));
        out.f32(static_cast<float>(thing.y));
        out.u16(static_cast<std::uint16_t>(thing.hp));
        out.u16(static_cast<std::uint16_t>(thing.owner));
        out.u8(thing.lit ? 1 : 0);
        out.f32(static_cast<float>(thing.fuel));
        out.f32(static_cast<float>(thing.progress));
        out.u8(static_cast<std::uint8_t>(thing.container.slots.size()));
        for (const ItemStack& stack : thing.container.slots) writeStack(out, stack);
    }

    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const bool wrote =
        std::fwrite(out.bytes().data(), 1, out.size(), file) == out.size();
    std::fclose(file);
    return wrote;
}

bool loadSession(const std::string& path, Session& session, World& world, BuildSystem& build) {
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    const bool read = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    std::fclose(file);
    if (!read || bytes.size() < 12) return false;

    net::Reader in(bytes.data(), bytes.size());
    if (in.u32() != kMagic) return false;
    if (in.u32() != kSaveVersion) return false;
    session.seed = in.u32();
    session.clock = in.f32();

    // The island is rebuilt from its seed rather than read back: it is the same
    // island, and only what happened to it is written down.
    world.generate(session.seed);

    Player& player = session.player;
    player = Player{};
    player.x = in.f32();
    player.y = in.f32();
    player.aim = in.f32();
    player.health = in.f32();
    player.calories = in.f32();
    player.hydration = in.f32();
    player.temperature = in.f32();
    player.radiation = in.f32();
    player.alive = in.u8() != 0;

    session.inventory = Inventory();
    for (ItemStack& stack : session.inventory.hotbar()) stack = readStack(in);
    for (ItemStack& stack : session.inventory.pack()) stack = readStack(in);
    session.inventory.worn() = readStack(in);
    session.inventory.selectSlot(in.u8());

    const std::uint32_t nodes = in.u32();
    for (std::uint32_t i = 0; i < nodes && in.ok(); ++i) {
        const int id = static_cast<int>(in.u32());
        const int hp = in.u16();
        const double respawn = in.f32();
        if (ResourceNode* node = world.nodeById(id)) {
            node->hp = hp;
            node->respawn = respawn;
        }
    }

    const std::uint32_t crates = in.u32();
    for (std::uint32_t i = 0; i < crates && in.ok(); ++i) {
        const int id = static_cast<int>(in.u32());
        const double respawn = in.f32();
        for (LootCrate& crate : world.crates()) {
            if (crate.id != id) continue;
            crate.looted = true;
            crate.respawn = respawn;
            for (ItemStack& slot : crate.container.slots) slot = ItemStack{};
            break;
        }
    }

    build = BuildSystem();
    const std::uint32_t pieces = in.u32();
    for (std::uint32_t i = 0; i < pieces && in.ok(); ++i) {
        const int id = static_cast<int>(in.u32());
        const auto kind = static_cast<BuildKind>(in.u8());
        const auto tier = static_cast<BuildTier>(in.u8());
        const int gx = in.i32();
        const int gy = in.i32();
        const auto side = static_cast<EdgeSide>(in.u8());
        const int hp = in.u16();
        const int maxHp = in.u16();
        const int owner = in.u16();
        const bool open = in.u8() != 0;
        const bool locked = in.u8() != 0;
        if (!in.ok()) break;
        Structure& piece = kind == BuildKind::Foundation
                               ? build.placeFoundation(gx, gy, owner, tier)
                               : build.placeEdge(gx, gy, side, kind, owner, tier);
        piece.id = id;
        piece.hp = hp;
        piece.maxHp = maxHp;
        piece.open = open;
        piece.locked = locked;
    }

    const std::uint32_t things = in.u32();
    for (std::uint32_t i = 0; i < things && in.ok(); ++i) {
        const auto kind = static_cast<DeployKind>(in.u8());
        const double x = in.f32();
        const double y = in.f32();
        const int hp = in.u16();
        const int owner = in.u16();
        const bool lit = in.u8() != 0;
        const double fuel = in.f32();
        const double progress = in.f32();
        const std::uint8_t slots = in.u8();
        if (!in.ok()) break;
        const int id = build.deploy(kind, static_cast<int>(x / kBuildCell),
                                    static_cast<int>(y / kBuildCell), owner);
        Deployable* thing = build.deployableById(id);
        if (!thing) break;
        thing->hp = hp;
        thing->lit = lit;
        thing->fuel = fuel;
        thing->progress = progress;
        thing->container.slots.assign(slots, ItemStack{});
        for (ItemStack& stack : thing->container.slots) stack = readStack(in);
    }
    return in.ok();
}

}  // namespace sim
