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
    Charcoal,
    Gunpowder,
    MeatCooked,
    Water,
    // Consumables and what you wear.
    Bandage,
    Medkit,
    Clothing,
    Hazmat,
    // What you put down rather than carry.
    Campfire,
    Furnace,
    ToolCupboard,
    WoodenBox,
    SleepingBag,
    // Raiding.
    Satchel,
    C4,
    RocketLauncher,
    Rocket,
    Lock,
    Workbench1,
    Workbench2,
    Workbench3,
    // Ammunition.
    Arrow,
    ShotgunShell,
    RifleAmmo,
    // Tools.
    Rock,
    Hatchet,
    Pickaxe,
    Hammer,
    BuildingPlan,
    // Weapons.
    Spear,
    Bow,
    Revolver,
    Waterpipe,
    PumpShotgun,
    Rifle,
    Ak47,
};

inline constexpr int kItemCount = 51;

enum class ItemCategory : std::uint8_t {
    Resource,
    Tool,
    Ammo,
    Weapon,
    Consumable,
    Clothing,
    Deployable,
    Explosive,
};

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

/** What eating, drinking or applying something does. */
struct Food {
    double calories;
    double hydration;
    double health;
    /** Zero means it happens at once; otherwise it is a channel of seconds. */
    double useSeconds;
};

/** What a charge does when it goes off. */
struct Boom {
    double damage;
    double radius;
    /** How long between throwing it and it going off. */
    double fuse;
};

/** What wearing something does. */
struct Wear {
    double warmth;
    /** The fraction of a blow it turns, and of the radiation it keeps out. */
    double armor;
    double radiation;
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
    /** All zero means it is not a thing you consume. */
    Food food;
    /** All zero means it is not a thing you wear. */
    Wear wear;
    /** All zero means it is not a thing that goes off. */
    Boom boom;
};

const ItemDef& itemDef(ItemId id);

/**
 * So much of one thing: what a recipe asks for, what a wall costs.
 *
 * The same shape as a stack, and deliberately a different name: a stack is
 * something that exists somewhere, a cost is something being asked for.
 */
struct Cost {
    ItemId id;
    int count;
};

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
