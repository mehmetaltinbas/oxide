#include "sim/item.hpp"

namespace sim {

namespace {

constexpr Melee kNone{0, 0, 0, 0};
constexpr Gun kNoGun{0, 0, 0, 0, 0, ItemId::None, 0, 0, 0};
constexpr Food kNoFood{0, 0, 0, 0};
constexpr Wear kNoWear{0, 0, 0};

/** The table, in the order of ItemId. */
constexpr ItemDef kItems[kItemCount] = {
    {ItemId::None, "", ItemCategory::Resource, 0, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Wood, "Wood", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Stone, "Stones", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::MetalOre, "Metal Ore", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Metal, "Metal Fragments", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::SulfurOre, "Sulfur Ore", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Sulfur, "Sulfur", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Cloth, "Cloth", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Scrap, "Scrap", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::LowGrade, "Low Grade Fuel", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::PistolAmmo, "Pistol Ammo", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::MeatRaw, "Raw Meat", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Leather, "Leather", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Bone, "Bone", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::AnimalFat, "Animal Fat", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Charcoal, "Charcoal", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Gunpowder, "Gunpowder", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::MeatCooked, "Cooked Meat", ItemCategory::Resource, 20, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::Water, "Water", ItemCategory::Resource, 250, kNone, kNoGun, kNoFood, kNoWear},
    // Stops bleeding, heals a little, slowly.
    {ItemId::Bandage, "Bandage", ItemCategory::Consumable, 12, kNone, kNoGun, {0, 0, 5, 5}, kNoWear},
    {ItemId::Medkit, "Medical Syringe", ItemCategory::Consumable, 10, kNone, kNoGun,
     {0, 0, 60, 1.5}, kNoWear},
    // Keeps the cold off and turns a little damage.
    {ItemId::Clothing, "Hide Clothing", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
     {14, 0.12, 0}},
    // The only way to loot a hot monument and walk out.
    {ItemId::Hazmat, "Hazmat Suit", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
     {8, 0.2, 0.9}},
    {ItemId::Arrow, "Arrow", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::ShotgunShell, "Shotgun Shell", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear},
    {ItemId::RifleAmmo, "Rifle Ammo", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear},
    // What you start with. Slow at everything.
    {ItemId::Rock, "Rock", ItemCategory::Tool, 1, {12, 1, 0.62, 30}, kNoGun, kNoFood, kNoWear},
    {ItemId::Hatchet, "Hatchet", ItemCategory::Tool, 1, {22, 3, 0.5, 34}, kNoGun, kNoFood, kNoWear},
    {ItemId::Pickaxe, "Pickaxe", ItemCategory::Tool, 1, {20, 3, 0.52, 34}, kNoGun, kNoFood, kNoWear},
    {ItemId::Hammer, "Hammer", ItemCategory::Tool, 1, {8, 1, 0.5, 36}, kNoGun, kNoFood, kNoWear},
    // Places twig foundations, walls and doorways.
    {ItemId::BuildingPlan, "Building Plan", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear},
    // Cheap reach, better than fists.
    {ItemId::Spear, "Wooden Spear", ItemCategory::Weapon, 1, {38, 1, 0.85, 58}, kNoGun, kNoFood, kNoWear},
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
};

}  // namespace

const ItemDef& itemDef(ItemId id) { return kItems[static_cast<int>(id)]; }

}  // namespace sim
