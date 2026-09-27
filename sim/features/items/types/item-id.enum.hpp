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
	Salvage,
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
	RadSuit,
	Backpack,
	// What you put down rather than carry.
	Campfire,
	Furnace,
	ToolCupboard,
	WoodenBox,
	LargeBox,
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
	// Added after the first pass, so they sit at the end: the table is looked
	// up by id rather than by position, and appending keeps every save valid.
	HqMetalOre,
	HqMetal,
	MetalSuit,
	HeavyMetalSuit,
	Torch,
};

inline constexpr int kItemCount = 58;

}  // namespace sim
