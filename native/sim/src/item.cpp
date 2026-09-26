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
    {ItemId::None, "", "", ItemCategory::Resource, 0, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Wood, "Wood", "Chopped from trees. The bottom of every recipe.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Stone, "Stones", "Broken from rock nodes.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::MetalOre, "Metal Ore", "Smelt it in a furnace for fragments.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Metal, "Metal Fragments", "Smelted ore. The backbone of good gear.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::SulfurOre, "Sulfur Ore", "Smelt it for sulfur.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Sulfur, "Sulfur", "Half of gunpowder, and therefore half of every raid.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Cloth, "Cloth", "Woven from nettle fibre. Bandages, clothing, bags.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Scrap, "Scrap", "Salvaged at monuments. Buys the good blueprints.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::LowGrade, "Low Grade Fuel", "Rendered from animal fat. Burns in lamps and satchels.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::PistolAmmo, "Pistol Ammo", "For the revolver.", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Raw meat is food and a gamble: it feeds you a little and costs you some
    // health for the privilege. Cooked is worth three times as much, for free.
    {ItemId::MeatRaw, "Raw Meat", "Eat it raw at your own risk. Cook it on a fire.",
     ItemCategory::Consumable, 20, kNone, kNoGun, {12, 0, -8, 0}, kNoWear, kNoBoom},
    {ItemId::Leather, "Leather", "Skinned from animals.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Bone, "Bone", "From carcasses. Crude tools and arrows.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::AnimalFat, "Animal Fat", "Cut off a carcass. Render it into fuel.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Charcoal, "Charcoal", "Furnace by-product. Half of gunpowder.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Gunpowder, "Gunpowder", "Sulfur and charcoal. Ammunition and explosives.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::MeatCooked, "Cooked Meat", "Proper food.", ItemCategory::Consumable, 20, kNone,
     kNoGun, {36, 0, 3, 0}, kNoWear, kNoBoom},
    // A drink you carry, worth a little more than a mouthful at the shore.
    {ItemId::Water, "Water", "Collected at rivers and lakes.", ItemCategory::Consumable, 250,
     kNone, kNoGun, {0, 40, 0, 0}, kNoWear, kNoBoom},
    // Stops bleeding, heals a little, slowly.
    {ItemId::Bandage, "Bandage", "Stops bleeding. Heals a little, slowly. Takes five seconds.", ItemCategory::Consumable, 12, kNone, kNoGun, {0, 0, 5, 5}, kNoWear, kNoBoom},
    {ItemId::Medkit, "Medical Syringe", "Heals a lot over a few seconds. Quick to apply.", ItemCategory::Consumable, 10, kNone, kNoGun,
     {0, 0, 60, 1.5}, kNoWear},
    // Keeps the cold off and turns a little damage.
    {ItemId::Clothing, "Hide Clothing", "Keeps the cold off and turns a little damage.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
     {14, 0.12, 0}},
    // The only way to loot a hot monument and walk out.
    {ItemId::Hazmat, "Hazmat Suit", "The only way to loot a hot monument and walk out.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
     {8, 0.2, 0.9}},
    {ItemId::Campfire, "Campfire", "Warmth, light, and it cooks meat.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Furnace, "Furnace", "Burns wood to smelt ore into fragments and sulfur.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::ToolCupboard, "Tool Cupboard", "Claims the ground around it. Nobody else builds inside your radius.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear},
    {ItemId::WoodenBox, "Wooden Box", "Storage. What raiders are actually coming for.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::SleepingBag, "Sleeping Bag", "Respawn point. Without one, death puts you back on the beach.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear},
    {ItemId::Arrow, "Arrow", "For the bow.", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::ShotgunShell, "Shotgun Shell", "For both shotguns. A handful of pellets each.", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::RifleAmmo, "Rifle Ammo", "For the rifles.", ItemCategory::Ammo, 128, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // What you start with. Slow at everything.
    {ItemId::Rock, "Rock", "What you start with. Slow at everything.", ItemCategory::Tool, 1, {12, 1, 0.62, 30}, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Hatchet, "Hatchet", "Fast on trees, poor on rock.", ItemCategory::Tool, 1, {22, 3, 0.5, 34}, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Pickaxe, "Pickaxe", "Fast on ore, poor on wood.", ItemCategory::Tool, 1, {20, 3, 0.52, 34}, kNoGun, kNoFood, kNoWear, kNoBoom},
    {ItemId::Hammer, "Hammer", "Upgrades and repairs building pieces you own.", ItemCategory::Tool, 1, {8, 1, 0.5, 36}, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Places twig foundations, walls and doorways.
    {ItemId::BuildingPlan, "Building Plan", "Places twig foundations, walls and doorways.", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Cheap reach, better than fists.
    {ItemId::Spear, "Wooden Spear", "Cheap reach. Better than fists.", ItemCategory::Weapon, 1, {38, 1, 0.85, 58}, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Quiet, cheap to feed, punishing to aim.
    {ItemId::Bow, "Hunting Bow", "Quiet, cheap to feed, punishing to aim.", ItemCategory::Weapon, 1, kNone,
     {48, 0.85, 760, 620, 0.03, ItemId::Arrow, 0, 0, 1}},
    {ItemId::Revolver, "Revolver", "First real gun most people build.", ItemCategory::Weapon, 1, kNone,
     {42, 0.34, 900, 640, 0.05, ItemId::PistolAmmo, 6, 3.0, 1}},
    // A cone of pellets: it kills up close and falls apart past short range.
    {ItemId::Waterpipe, "Waterpipe Shotgun", "One shell, a long reload, and a very bad day for whoever is in front of it.", ItemCategory::Weapon, 1, kNone,
     {14, 1.0, 820, 260, 0.26, ItemId::ShotgunShell, 1, 6.0, 9}},
    {ItemId::PumpShotgun, "Pump Shotgun", "Six shells, one a second. Nothing clears a room faster.", ItemCategory::Weapon, 1, kNone,
     {13, 1.05, 860, 300, 0.2, ItemId::ShotgunShell, 6, 5.5, 8}},
    {ItemId::Rifle, "Semi-Automatic Rifle", "The gun that decides most fights.", ItemCategory::Weapon, 1, kNone,
     {62, 0.19, 1250, 950, 0.035, ItemId::RifleAmmo, 16, 4.0, 1}},
    // Rust's AK kicks hard, which a top-down game has no camera to show, so
    // the recoil is spent as spread: it wins close and loses long.
    {ItemId::Ak47, "Assault Rifle", "The AK. Fast, loud and hard to hold on target past close range.", ItemCategory::Weapon, 1, kNone,
     {78, 0.145, 1250, 950, 0.06, ItemId::RifleAmmo, 30, 4.4, 1}},
    // Raiding.
    {ItemId::Satchel, "Satchel Charge", "Cheap raiding. Unreliable, and it takes several.", ItemCategory::Explosive, 10, kNone, kNoGun, kNoFood,
     kNoWear, {475, 90, 3.2}},
    {ItemId::C4, "Timed Explosive", "Opens anything. Expensive enough that you plan around it.", ItemCategory::Explosive, 5, kNone, kNoGun, kNoFood, kNoWear,
     {550, 120, 4.5}},
    // Raiding from range, Rust's way: one in the tube and a long reload.
    {ItemId::RocketLauncher, "Rocket Launcher", "For raiding. One rocket at a time, and a long reload.", ItemCategory::Weapon, 1, kNone,
     {0, 1.0, 560, 900, 0.02, ItemId::Rocket, 1, 6.0, 1}, kNoFood, kNoWear, {275, 100, 0}},
    {ItemId::Rocket, "Rocket", "For the rocket launcher. Half a timed explosive, from range.", ItemCategory::Ammo, 3, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // Goes on a door, and keeps everyone else the other side of it.
    {ItemId::Lock, "Code Lock", "Locks a door so raiders have to break it instead of walking in.", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
    // The benches. What you can make is what you are standing next to.
    {ItemId::Workbench1, "Workbench I", "Unlocks the first proper tier of recipes while you stand near it.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear, kNoBoom},
    {ItemId::Workbench2, "Workbench II", "Guns and better armour.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
     kNoWear, kNoBoom},
    {ItemId::Workbench3, "Workbench III", "Rifles and C4.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
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
            table[i] = ItemDef{static_cast<ItemId>(i), "", "", ItemCategory::Resource, 1,
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
