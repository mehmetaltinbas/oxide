#include "sim/item.hpp"

namespace sim {

namespace {

constexpr Melee kNone{0, 0, 0, 0};
constexpr Gun kNoGun{0, 0, 0, 0, 0, ItemId::None, 0, 0, 0};
constexpr Food kNoFood{0, 0, 0, 0};
constexpr Wear kNoWear{0, 0, 0};
constexpr Boom kNoBoom{0, 0, 0};

/** Every item there is. The order here does not matter; the ids do. */
constexpr ItemDef kItems[] = {
    {ItemId::None, "", ItemCategory::Resource, 0, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Wood, "Wood", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Stone, "Stones", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::MetalOre, "Metal Ore", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Metal, "Metal Fragments", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::SulfurOre, "Sulfur Ore", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Sulfur, "Sulfur", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Cloth, "Cloth", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Scrap, "Scrap", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::LowGrade, "Low Grade Fuel", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::PistolAmmo, "Pistol Ammo", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::MeatRaw, "Raw Meat", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Leather, "Leather", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Bone, "Bone", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::AnimalFat, "Animal Fat", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Charcoal, "Charcoal", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Gunpowder, "Gunpowder", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::MeatCooked, "Cooked Meat", ItemCategory::Resource, 20, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Water, "Water", ItemCategory::Resource, 250, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Stops bleeding, heals a little, slowly.
    {ItemId::Bandage, "Bandage", ItemCategory::Consumable, 12, kNone, kNoGun, {0, 0, 5, 5}, kNoWear, kNoBoom},
    {ItemId::Medkit, "Medical Syringe", ItemCategory::Consumable, 10, kNone, kNoGun,
     {0, 0, 60, 1.5}, kNoWear},
    // Keeps the cold off and turns a little damage.
    {ItemId::Clothing, "Hide Clothing", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
     {14, 0.12, 0}},
    // The only way to loot a hot monument and walk out.
    {ItemId::Hazmat, "Hazmat Suit", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
     {8, 0.2, 0.9}},
    {ItemId::Campfire, "Campfire", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Furnace, "Furnace", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::ToolCupboard, "Tool Cupboard", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear},
    {ItemId::WoodenBox, "Wooden Box", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::SleepingBag, "Sleeping Bag", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear},
    {ItemId::Arrow, "Arrow", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::ShotgunShell, "Shotgun Shell", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::RifleAmmo, "Rifle Ammo", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // What you start with. Slow at everything.
    {ItemId::Rock, "Rock", ItemCategory::Tool, 1, {12, 1, 0.62, 30}, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Hatchet, "Hatchet", ItemCategory::Tool, 1, {22, 3, 0.5, 34}, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Pickaxe, "Pickaxe", ItemCategory::Tool, 1, {20, 3, 0.52, 34}, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Hammer, "Hammer", ItemCategory::Tool, 1, {8, 1, 0.5, 36}, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Places twig foundations, walls and doorways.
    {ItemId::BuildingPlan, "Building Plan", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Cheap reach, better than fists.
    {ItemId::Spear, "Wooden Spear", ItemCategory::Weapon, 1, {38, 1, 0.85, 58}, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Quiet, cheap to feed, punishing to aim.
    {ItemId::Bow, "Hunting Bow", ItemCategory::Weapon, 1, kNone,
     {48, 0.85, 760, 620, 0.03, ItemId::Arrow, 0, 0, 1}},
    {ItemId::Revolver, "Revolver", ItemCategory::Weapon, 1, kNone,
     {42, 0.34, 900, 640, 0.05, ItemId::PistolAmmo, 6, 3.0, 1}},
    // A cone of pellets: it kills up close and falls apart past short range.
    {ItemId::Waterpipe, "Waterpipe Shotgun", ItemCategory::Weapon, 1, kNone,
     {14, 1.0, 820, 260, 0.26, ItemId::ShotgunShell, 1, 6.0, 9}},
    {ItemId::PumpShotgun, "Pump Shotgun", ItemCategory::Weapon, 1, kNone,
     {13, 1.05, 860, 300, 0.2, ItemId::ShotgunShell, 6, 5.5, 8}},
    {ItemId::Rifle, "Semi-Automatic Rifle", ItemCategory::Weapon, 1, kNone,
     {62, 0.19, 1250, 950, 0.035, ItemId::RifleAmmo, 16, 4.0, 1}},
    // Rust's AK kicks hard, which a top-down game has no camera to show, so
    // the recoil is spent as spread: it wins close and loses long.
    {ItemId::Ak47, "Assault Rifle", ItemCategory::Weapon, 1, kNone,
     {78, 0.145, 1250, 950, 0.06, ItemId::RifleAmmo, 30, 4.4, 1}},
    // Raiding.
    {ItemId::Satchel, "Satchel Charge", ItemCategory::Explosive, 10, kNone, kNoGun, kNoFood,
     kNoWear, {475, 90, 3.2}},
    {ItemId::C4, "Timed Explosive", ItemCategory::Explosive, 5, kNone, kNoGun, kNoFood, kNoWear,
     {550, 120, 4.5}},
    // Raiding from range, Rust's way: one in the tube and a long reload.
    {ItemId::RocketLauncher, "Rocket Launcher", ItemCategory::Weapon, 1, kNone,
     {0, 1.0, 560, 900, 0.02, ItemId::Rocket, 1, 6.0, 1}, kNoFood, kNoWear, {275, 100, 0}},
    {ItemId::Rocket, "Rocket", ItemCategory::Ammo, 3, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Goes on a door, and keeps everyone else the other side of it.
    {ItemId::Lock, "Code Lock", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // The benches. What you can make is what you are standing next to.
    {ItemId::Workbench1, "Workbench I", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear, kNoBoom},
    {ItemId::Workbench2, "Workbench II", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear, kNoBoom},
    {ItemId::Workbench3, "Workbench III", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear, kNoBoom},
};

/**
 * Looked up by the id on each row rather than by where the row happens to sit.
 *
 * Written out in order once, they drifted: eight items were added to the enum
 * and to the table in different places, and every id past that point read as a
 * different item, which is how a rock stopped being able to fell a tree.
 */
const ItemDef* byId() {
    static ItemDef table[kItemCount];
    static bool filled = false;
    if (!filled) {
        for (int i = 0; i < kItemCount; ++i) {
            table[i] = ItemDef{static_cast<ItemId>(i), "", ItemCategory::Resource, 1,
                               kNone, kNoGun, kNoFood, kNoWear, kNoBoom};
        }
        for (const ItemDef& def : kItems) {
            table[static_cast<int>(def.id)] = def;
        }
        filled = true;
    }
    return table;
}

}  // namespace

const ItemDef& itemDef(ItemId id) { return byId()[static_cast<int>(id)]; }

}  // namespace sim
