#include "sim/item.hpp"

namespace sim {

namespace {

constexpr Melee kNone{0, 0, 0, 0};
constexpr Gun kNoGun{0, 0, 0, 0, 0, ItemId::None, 0, 0, 0};

/** The table, in the order of ItemId. */
constexpr ItemDef kItems[kItemCount] = {
    {ItemId::None, "", ItemCategory::Resource, 0, kNone, kNoGun},
    {ItemId::Wood, "Wood", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Stone, "Stones", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::MetalOre, "Metal Ore", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Metal, "Metal Fragments", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::SulfurOre, "Sulfur Ore", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Sulfur, "Sulfur", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Cloth, "Cloth", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Scrap, "Scrap", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::LowGrade, "Low Grade Fuel", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::PistolAmmo, "Pistol Ammo", ItemCategory::Ammo, 128, kNone, kNoGun},
    {ItemId::MeatRaw, "Raw Meat", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Leather, "Leather", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Bone, "Bone", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::AnimalFat, "Animal Fat", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Charcoal, "Charcoal", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Gunpowder, "Gunpowder", ItemCategory::Resource, 1000, kNone, kNoGun},
    {ItemId::Arrow, "Arrow", ItemCategory::Ammo, 128, kNone, kNoGun},
    {ItemId::ShotgunShell, "Shotgun Shell", ItemCategory::Ammo, 128, kNone, kNoGun},
    {ItemId::RifleAmmo, "Rifle Ammo", ItemCategory::Ammo, 128, kNone, kNoGun},
    // What you start with. Slow at everything.
    {ItemId::Rock, "Rock", ItemCategory::Tool, 1, {12, 1, 0.62, 30}, kNoGun},
    {ItemId::Hatchet, "Hatchet", ItemCategory::Tool, 1, {22, 3, 0.5, 34}, kNoGun},
    {ItemId::Pickaxe, "Pickaxe", ItemCategory::Tool, 1, {20, 3, 0.52, 34}, kNoGun},
    {ItemId::Hammer, "Hammer", ItemCategory::Tool, 1, {8, 1, 0.5, 36}, kNoGun},
    // Places twig foundations, walls and doorways.
    {ItemId::BuildingPlan, "Building Plan", ItemCategory::Tool, 1, kNone, kNoGun},
    // Cheap reach, better than fists.
    {ItemId::Spear, "Wooden Spear", ItemCategory::Weapon, 1, {38, 1, 0.85, 58}, kNoGun},
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
