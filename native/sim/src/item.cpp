#include "sim/item.hpp"

namespace sim {

namespace {

constexpr Melee kNone{0, 0, 0, 0};

/** The table, in the order of ItemId. */
constexpr ItemDef kItems[kItemCount] = {
    {ItemId::None, "", ItemCategory::Resource, 0, kNone},
    {ItemId::Wood, "Wood", ItemCategory::Resource, 1000, kNone},
    {ItemId::Stone, "Stones", ItemCategory::Resource, 1000, kNone},
    {ItemId::MetalOre, "Metal Ore", ItemCategory::Resource, 1000, kNone},
    {ItemId::Metal, "Metal Fragments", ItemCategory::Resource, 1000, kNone},
    {ItemId::SulfurOre, "Sulfur Ore", ItemCategory::Resource, 1000, kNone},
    {ItemId::Sulfur, "Sulfur", ItemCategory::Resource, 1000, kNone},
    {ItemId::Cloth, "Cloth", ItemCategory::Resource, 1000, kNone},
    {ItemId::Scrap, "Scrap", ItemCategory::Resource, 1000, kNone},
    {ItemId::LowGrade, "Low Grade Fuel", ItemCategory::Resource, 1000, kNone},
    {ItemId::PistolAmmo, "Pistol Ammo", ItemCategory::Ammo, 128, kNone},
    {ItemId::MeatRaw, "Raw Meat", ItemCategory::Resource, 1000, kNone},
    {ItemId::Leather, "Leather", ItemCategory::Resource, 1000, kNone},
    {ItemId::Bone, "Bone", ItemCategory::Resource, 1000, kNone},
    {ItemId::AnimalFat, "Animal Fat", ItemCategory::Resource, 1000, kNone},
    // What you start with. Slow at everything.
    {ItemId::Rock, "Rock", ItemCategory::Tool, 1, {12, 1, 0.62, 30}},
    {ItemId::Hatchet, "Hatchet", ItemCategory::Tool, 1, {22, 3, 0.5, 34}},
    {ItemId::Pickaxe, "Pickaxe", ItemCategory::Tool, 1, {20, 3, 0.52, 34}},
    {ItemId::Hammer, "Hammer", ItemCategory::Tool, 1, {8, 1, 0.5, 36}},
};

}  // namespace

const ItemDef& itemDef(ItemId id) { return kItems[static_cast<int>(id)]; }

}  // namespace sim
