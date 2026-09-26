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
	// Bottom of the chain: quick, twitchy, barely worth an arrow, and the one
	// thing you can kill on your first morning with a rock.
	{NpcKind::Rabbit, "Rabbit", 12, 150, 6, 0, 14, 1.4, false, false, kNoGun,
	 {{ItemId::MeatRaw, 1, 2}, {ItemId::Leather, 1, 3}, {ItemId::Bone, 1, 2}},
	 3,
	 {Biome::Grass, Biome::Forest},
	 2,
	 0,
	 {},
	 0,
	 true,
	 0},

	// The meat animal, and a big one: an elk is most of a week of food and a
	// full set of hide. A sprint is 185 and an elk is 165, so you can run one
	// down on foot and arrive with nothing left; a bow is the sane way.
	{NpcKind::Elk, "Elk", 160, 165, 18, 14, 28, 1.4, false, false, kNoGun,
	 {{ItemId::MeatRaw, 7, 12},
	  {ItemId::Leather, 18, 30},
	  {ItemId::Bone, 8, 14},
	  {ItemId::AnimalFat, 7, 13}},
	 4,
	 {Biome::Forest, Biome::Snow},
	 2,
	 2,
	 {},
	 0,
	 true,
	 0},

	// Neutral, and the one thing on the island that teaches it. It will graze
	// past you all day. Hit it once and it turns round and kicks hard enough
	// that you will not do it twice without meaning to.
	{NpcKind::Kangaroo, "Kangaroo", 130, 168, 13, 24, 26, 1.2, false, false, kNoGun,
	 {{ItemId::MeatRaw, 4, 8},
	  {ItemId::Leather, 11, 19},
	  {ItemId::Bone, 5, 10},
	  {ItemId::AnimalFat, 5, 11}},
	 4,
	 {Biome::Grass, Biome::Desert},
	 2,
	 3,
	 {},
	 0,
	 false,
	 0},

	// Everywhere that is not trees: grass, tundra and sand alike, which is what
	// keeps the desert from being a free walk.
	//
	// 176 against a sprint of 185. It has to be under a sprint, or a wolf is a
	// death sentence rather than a decision; it has to be over an elk's 165, or
	// it never eats and the food chain does nothing. Nine units of margin is
	// what is left, which is a break you have to commit to rather than one you
	// stroll away with. It was 184, which is a sprint to within a rounding
	// error: you could not get away at all.
	{NpcKind::Wolf, "Wolf", 80, 176, 13, 22, 28, 0.9, true, false, kNoGun,
	 {{ItemId::MeatRaw, 2, 4},
	  {ItemId::Leather, 6, 12},
	  {ItemId::Bone, 3, 6},
	  {ItemId::AnimalFat, 2, 5}},
	 4,
	 {Biome::Grass, Biome::Snow, Biome::Desert},
	 3,
	 4,
	 {NpcKind::Rabbit, NpcKind::Elk},
	 2,
	 true,
	 0},
	// Top of the chain, and the one thing on the island that runs from nothing
	// at all. 150 against a sprint of 185: it runs down anybody who walks away
	// from it and never catches anybody who sprints, which is the shape of
	// every threat here. It was 92, at which it could not catch a walking
	// player either, so the thing that is supposed to make the forest somewhere
	// you do not go yet was something you strolled past.
	{NpcKind::Bear, "Bear", 340, 150, 22, 42, 40, 1.3, true, false, kNoGun,
	 {{ItemId::MeatRaw, 8, 16},
	  {ItemId::Leather, 20, 36},
	  {ItemId::Bone, 10, 20},
	  {ItemId::AnimalFat, 14, 30}},
	 4,
	 {Biome::Forest, Biome::Snow},
	 2,
	 9,
	 {NpcKind::Elk, NpcKind::Wolf},
	 2,
	 false,
	 0},
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
	 false,
	 0},
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
	 false,
	 0},
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
