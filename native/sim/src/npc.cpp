#include "sim/npc.hpp"

namespace sim {

namespace {

constexpr NpcGun kNoGun{0, 0, 0, 0, 0};

/**
 * The island's wildlife.
 *
 * Five animals, each living in its own country, arranged as a food chain
 * rather than as a difficulty ladder: a chicken runs from everything, a deer
 * runs from the three things that eat it, hyenas and wolves hunt both, and a
 * bear runs from nothing and eats whatever it catches. What each is worth
 * dead is roughly what it was worth alive, so a deer pays for the walk into
 * the forest and a chicken does not pay for the arrow.
 */
constexpr NpcDef kDefs[kNpcKindCount] = {
	// Bottom of the chain: quick, useless, and something to practise a bow on.
	{NpcKind::Chicken, "Chicken", 15, 108, 7, 2, 18, 1.4, false, false, kNoGun,
	 {{ItemId::MeatRaw, 1, 2}, {ItemId::Bone, 1, 2}, {ItemId::AnimalFat, 0, 1}},
	 3,
	 {Biome::Grass, Biome::Forest},
	 2,
	 0,
	 {},
	 0,
	 true},
	// The meat animal. Fast enough that you need a bow or a plan, and worth
	// the trouble: one deer is a full day of food and most of a set of hide.
	{NpcKind::Deer, "Deer", 120, 178, 15, 8, 26, 1.4, false, false, kNoGun,
	 {{ItemId::MeatRaw, 5, 9},
	  {ItemId::Leather, 14, 24},
	  {ItemId::Bone, 6, 11},
	  {ItemId::AnimalFat, 5, 10}},
	 4,
	 {Biome::Forest, Biome::Snow},
	 2,
	 1,
	 {},
	 0,
	 true},
	// The desert's own, and the reason the sand is not a free walk. Hunts in
	// the open and backs off a bear.
	{NpcKind::Hyena, "Hyena", 95, 150, 12, 18, 26, 0.85, true, false, kNoGun,
	 {{ItemId::MeatRaw, 2, 4},
	  {ItemId::Leather, 5, 10},
	  {ItemId::Bone, 4, 8},
	  {ItemId::AnimalFat, 2, 4}},
	 4,
	 {Biome::Desert, Biome::Grass},
	 2,
	 3,
	 {NpcKind::Chicken, NpcKind::Deer},
	 2,
	 true},
	// Open country and tundra. Fast, and it does not wait to be provoked.
	{NpcKind::Wolf, "Wolf", 80, 155, 13, 22, 28, 0.9, true, false, kNoGun,
	 {{ItemId::MeatRaw, 2, 4},
	  {ItemId::Leather, 6, 12},
	  {ItemId::Bone, 3, 6},
	  {ItemId::AnimalFat, 2, 5}},
	 4,
	 {Biome::Grass, Biome::Snow},
	 2,
	 4,
	 {NpcKind::Chicken, NpcKind::Deer},
	 2,
	 true},
	// Top of the chain. Slow, enormous, and the one thing on the island that
	// runs from nothing at all.
	{NpcKind::Bear, "Bear", 340, 92, 22, 42, 40, 1.3, true, false, kNoGun,
	 {{ItemId::MeatRaw, 8, 16},
	  {ItemId::Leather, 20, 36},
	  {ItemId::Bone, 10, 20},
	  {ItemId::AnimalFat, 14, 30}},
	 4,
	 {Biome::Forest, Biome::Snow},
	 2,
	 9,
	 {NpcKind::Deer, NpcKind::Wolf, NpcKind::Hyena},
	 3,
	 false},
	// The monuments' own: sealed in a suit, and armed. Placed by hand rather
	// than scattered, so they have no country of their own.
	{NpcKind::Scientist, "Scientist", 130, 118, 12, 14, 30, 1.0, true, true,
	 {19, 470, 1.15, 900, 0.07},
	 {{ItemId::Scrap, 8, 20},
	  {ItemId::PistolAmmo, 4, 12},
	  {ItemId::Metal, 10, 30},
	  {ItemId::Medkit, 0, 1}},
	 4,
	 {},
	 0,
	 7,
	 {},
	 0,
	 false},
	// Better trained than the scientists, in the field green of an army.
	{NpcKind::Soldier, "Soldier", 170, 126, 12, 18, 30, 0.9, true, true,
	 {24, 520, 1.0, 950, 0.06},
	 {{ItemId::Scrap, 12, 28},
	  {ItemId::RifleAmmo, 6, 16},
	  {ItemId::Metal, 20, 50},
	  {ItemId::Medkit, 0, 2},
	  {ItemId::Ak47, 0, 1}},
	 5,
	 {},
	 0,
	 8,
	 {},
	 0,
	 false},
};

}  // namespace

const NpcDef& npcDef(NpcKind kind) { return kDefs[static_cast<int>(kind)]; }

bool livesIn(const NpcDef& def, Biome biome) {
	for (int i = 0; i < def.homeCount; ++i) {
		if (def.homes[i] == biome) return true;
	}
	return false;
}

bool hunts(const NpcDef& def, NpcKind prey) {
	for (int i = 0; i < def.eatsCount; ++i) {
		if (def.eats[i] == prey) return true;
	}
	return false;
}

}  // namespace sim
