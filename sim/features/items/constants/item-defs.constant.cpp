#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/boom.struct.hpp"
#include "sim/features/items/types/food.struct.hpp"
#include "sim/features/items/types/gun.struct.hpp"
#include "sim/features/items/types/item-category.enum.hpp"
#include "sim/features/items/types/item-def.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/melee.struct.hpp"
#include "sim/features/items/types/wear.struct.hpp"
#include "sim/features/building/types/deployable.struct.hpp"

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
	{ItemId::MetalOre, "Metal Ore", "Smelt it in a furnace for metal.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	{ItemId::Metal, "Metal", "Smelted ore. The backbone of good gear.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
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
	// Five seconds, five health, and fifty seconds of bleeding stopped: a
	// bandage is for the wound rather than for the health bar.
	{ItemId::Bandage, "Bandage", "Stops bleeding. Heals a little. Takes five seconds.", ItemCategory::Consumable, 12, kNone, kNoGun, {0, 0, 5, 5, 0, 50, 0}, kNoWear, kNoBoom},
	// Two and a half seconds, fifteen health at once and twenty more over the
	// next twenty, and it clears ten radiation on the way. The instant part is
	// what makes it worth carrying into a fight; the rest is what makes it
	// worth using before one.
	{ItemId::Medkit, "Medical Syringe", "Fifteen health at once and twenty more over the next twenty seconds. Clears a little radiation.", ItemCategory::Consumable, 10, kNone, kNoGun,
	 {0, 0, 15, 2.5, 20, 0, 10}, kNoWear},
	// Twelve more slots, and nothing else: no warmth, no armour, no
	// protection. It goes on your back rather than over your body, so it is
	// worn alongside a suit rather than instead of one.
	{ItemId::Backpack, "Backpack", "Twelve more slots. Worn on your back, so it costs you no armour.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	// Keeps the cold off and turns a little damage.
	{ItemId::Clothing, "Hide Clothing", "Keeps the cold off and turns a little damage.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
	// The warmest thing on the island: hide is what you wear because it is
	// cold, and every suit after it trades that away for something else.
	 {14, 0.15, 0}},
	// The only way to loot a hot monument and walk out.
	{ItemId::RadSuit, "Radiation Suit", "The only way to loot a hot monument and walk out.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
	// Ninety-five, not ninety. With the plant's dose now lethal in sixteen
	// seconds bare, ninety left barely a minute of safe time inside it, which
	// is not a looting run: the suit has to be the answer, not a stay of
	// execution.
	 {8, 0.2, 0.95}},
	{ItemId::Campfire, "Campfire", "Warmth, light, and it cooks meat.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	{ItemId::Furnace, "Furnace", "Burns wood to smelt ore into fragments and sulfur.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	{ItemId::ToolCupboard, "Tool Cupboard", "Claims the ground around it. Nobody else builds inside your radius.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
	 kNoWear},
	{ItemId::WoodenBox, "Small Storage Box", "Cheap storage. What raiders are actually coming for.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	{ItemId::LargeBox, "Large Storage Box", "Three times the room and three times the loss.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
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
	// Reach is the whole of what a spear is for: it is slower and weaker than
	// a hatchet, and worth carrying only because it lands before the thing in
	// front of you does. At 78 a bear's bite came in at 53 and the margin was
	// thin enough that the spear felt like a short stick; 104 is half again as
	// far as anything on the island can reach.
	{ItemId::Spear, "Wooden Spear", "Cheap reach. Better than fists.", ItemCategory::Weapon, 1, {38, 1, 0.85, 104}, kNoGun, kNoFood, kNoWear, kNoBoom},
	// Quiet, cheap to feed, punishing to aim.
	{ItemId::Bow, "Hunting Bow", "Quiet, cheap to feed, punishing to aim.", ItemCategory::Weapon, 1, kNone,
	 {48, 0.85, 760, 620, 0.03, ItemId::Arrow, 0, 0, 1, 26}},
	{ItemId::Revolver, "Revolver", "First real gun most people build.", ItemCategory::Weapon, 1, kNone,
	 {42, 0.34, 900, 640, 0.05, ItemId::PistolAmmo, 6, 3.0, 1, 22}},
	// A cone of pellets: it kills up close and falls apart past short range.
	{ItemId::Waterpipe, "Waterpipe Shotgun", "One shell, a long reload, and a very bad day for whoever is in front of it.", ItemCategory::Weapon, 1, kNone,
	 {14, 1.0, 820, 260, 0.26, ItemId::ShotgunShell, 1, 2.4, 9, 32, true}},
	{ItemId::PumpShotgun, "Pump Shotgun", "Six shells, one a second. Nothing clears a room faster.", ItemCategory::Weapon, 1, kNone,
	 {13, 1.05, 860, 300, 0.2, ItemId::ShotgunShell, 6, 0.62, 8, 34, true}},
	{ItemId::Rifle, "Semi-Automatic Rifle", "The gun that decides most fights.", ItemCategory::Weapon, 1, kNone,
	 {62, 0.19, 1250, 950, 0.035, ItemId::RifleAmmo, 16, 4.0, 1, 35}},
	// Rust's AK kicks hard, which a top-down game has no camera to show, so
	// the recoil is spent as spread: it wins close and loses long.
	{ItemId::Ak47, "Assault Rifle", "The AK. Fast, loud and hard to hold on target past close range.", ItemCategory::Weapon, 1, kNone,
	 {78, 0.145, 1250, 950, 0.06, ItemId::RifleAmmo, 30, 4.4, 1, 37}},
	// Raiding.
	{ItemId::Satchel, "Satchel Charge", "Cheap raiding. Unreliable, and it takes several.", ItemCategory::Explosive, 10, kNone, kNoGun, kNoFood,
	 kNoWear, {475, 90, 3.2}},
	{ItemId::C4, "Timed Explosive", "Opens anything. Expensive enough that you plan around it.", ItemCategory::Explosive, 5, kNone, kNoGun, kNoFood, kNoWear,
	 {550, 120, 4.5}},
	// Raiding from range, Rust's way: one in the tube and a long reload.
	{ItemId::RocketLauncher, "Rocket Launcher", "For raiding. One rocket at a time, and a long reload.", ItemCategory::Weapon, 1, kNone,
	 {0, 1.0, 560, 900, 0.02, ItemId::Rocket, 1, 6.0, 1, 40}, kNoFood, kNoWear, {275, 100, 0}},
	{ItemId::Rocket, "Rocket", "For the rocket launcher. Half a timed explosive, from range.", ItemCategory::Ammo, 3, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	// Goes on a door, and keeps everyone else the other side of it.
	{ItemId::Lock, "Code Lock", "Locks a door so raiders have to break it instead of walking in.", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	// The benches. What you can make is what you are standing next to.
	{ItemId::Workbench1, "Workbench I", "Unlocks the first proper tier of recipes while you stand near it.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
	 kNoWear, kNoBoom},
	{ItemId::Workbench2, "Workbench II", "Guns and better armour.", ItemCategory::Deployable, 1, kNone, kNoGun, kNoFood,
	 kNoWear, kNoBoom},
	// High quality metal: the top of the ladder. Chipped out of a metal node a
	// few pieces at a time, smelted like any other ore, and found ready-made in
	// the crates at a monument, which is what makes a monument run worth it.
	{ItemId::HqMetalOre, "High Quality Metal Ore", "Rare, out of metal nodes. Smelt it for high quality metal.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	{ItemId::HqMetal, "High Quality Metal", "The best armour and the best guns are made of it.", ItemCategory::Resource, 1000, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
	// What a road sign nailed to a jacket buys you: real protection, no warmth.
	{ItemId::MetalSuit, "Metal Suit", "Road signs and scrap, strapped on. Turns a third of a blow and keeps none of the cold off.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
	 {2, 0.35, 0.1}},
	// The full plate. Nothing short of a rifle gets through it.
	{ItemId::HeavyMetalSuit, "Heavy Metal Suit", "Plate over everything. Turns half a blow, and you feel every step of it.", ItemCategory::Clothing, 1, kNone, kNoGun, kNoFood,
	// Coldest of the lot: plate over everything holds no heat at all, and you
	// feel every step of it. Hide, then the suit, then scrap, then plate.
	 {1, 0.5, 0.25}},
	// Light you can carry, which is the whole first night sorted.
	// A light, and only a light: swinging a burning rag at a bear is not a
	// plan, and letting it work made the torch a free first weapon.
	{ItemId::Torch, "Torch", "Cloth on a stick. Light to carry, and no use for anything else.", ItemCategory::Tool, 1, kNone, kNoGun, kNoFood, kNoWear, kNoBoom},
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
