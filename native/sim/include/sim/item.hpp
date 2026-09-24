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
    // Ammunition.
    Arrow,
    ShotgunShell,
    RifleAmmo,
    // Tools.
    Rock,
    Hatchet,
    Pickaxe,
    Hammer,
    // Weapons.
    Spear,
    Bow,
    Revolver,
    Waterpipe,
    PumpShotgun,
    Rifle,
    Ak47,
};

inline constexpr int kItemCount = 29;

enum class ItemCategory : std::uint8_t { Resource, Tool, Ammo, Weapon };

/** What a tool does when it lands. */
struct Melee {
    double damage;
    /** How much comes off a node per blow, before the node's own preference. */
    double gather;
    double cooldown;
    /** How far the head of it reaches, measured off the swing as it is drawn. */
    double reach;
};

/** What a gun does when it goes off. */
struct Gun {
    double damage;
    double cooldown;
    /** How fast the round travels, and how far before it is spent. */
    double speed;
    double range;
    /** How far off true a shot can come out, in radians. */
    double spread;
    ItemId ammo;
    /** How many rounds it holds, and how long it takes to fill again. */
    int magazine;
    double reloadSeconds;
    /** More than one means a cone of them: a shotgun. */
    int pellets;
};

struct ItemDef {
    ItemId id;
    const char* name;
    ItemCategory category;
    int stack;
    /** Zero damage means it is not a thing you swing. */
    Melee melee;
    /** Zero damage means it is not a thing you fire. */
    Gun gun;
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

/** How long a bow takes to draw: an arrow cannot be loosed before it is full. */
inline constexpr double kBowDrawSeconds = 1.0;

}  // namespace sim
