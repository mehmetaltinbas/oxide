#pragma once

#include <cstdint>

namespace sim {

/** Everything that can sit in a slot. */
enum class ItemId : std::uint8_t {
    None,
    // Resources.
    Wood,
    Stone,
    MetalOre,
    Metal,
    SulfurOre,
    Sulfur,
    Cloth,
    Scrap,
    LowGrade,
    PistolAmmo,
    MeatRaw,
    Leather,
    Bone,
    AnimalFat,
    // Tools.
    Rock,
    Hatchet,
    Pickaxe,
    Hammer,
};

inline constexpr int kItemCount = 19;

enum class ItemCategory : std::uint8_t { Resource, Tool, Ammo };

/** What a tool does when it lands. */
struct Melee {
    double damage;
    /** How much comes off a node per blow, before the node's own preference. */
    double gather;
    double cooldown;
    /** How far the head of it reaches, measured off the swing as it is drawn. */
    double reach;
};

struct ItemDef {
    ItemId id;
    const char* name;
    ItemCategory category;
    int stack;
    /** Zero damage means it is not a thing you swing. */
    Melee melee;
};

const ItemDef& itemDef(ItemId id);

/** A count of one thing, as it sits in a slot or on the ground. */
struct ItemStack {
    ItemId id = ItemId::None;
    int count = 0;
};

/**
 * Bare hands, as a fraction of the rock you start with.
 *
 * Half the damage and half the yield: enough to get a log and a stone together
 * and make a rock again, and slow enough that you want to.
 */
inline constexpr double kFistFraction = 0.5;

/** What bare hands can work: a tree, and a plain stone. */
bool fistsCanWork(ItemId nodeKindAsItem);

}  // namespace sim
