#pragma once

#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/items/systems/inventory.hpp"
#include "sim/features/survival/types/player.struct.hpp"
#include "sim/features/world/systems/world.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/world/constants/daylight.constant.hpp"
#include "sim/features/world/types/biome.enum.hpp"

namespace sim {

/** What a body needs, and what it can take. */
struct PlayerVitals {
	static constexpr double kMaxHealth = 100;
	/**
	 * Two hundred, not a hundred.
	 *
	 * The top half is a reserve rather than a meter: nothing bad happens until
	 * a meter reaches nought, so a hundred was a number you topped up and
	 * forgot. Above a hundred of both you can spend the surplus on comfort at
	 * a fire, which is what makes eating past full worth doing.
	 */
	static constexpr double kMaxCalories = 200;
	static constexpr double kMaxHydration = 200;
	/** Where the reserve starts: comfort spends what is above this. */
	static constexpr double kWellFed = 100;
	/**
	 * What one person sitting at a fire is worth, as a fraction.
	 *
	 * A quarter each, up to four of you. A fire is better shared: the
	 * healing, and the food and water it costs, both scale with how many of
	 * you are round it, so a camp of four mends four times as fast and eats
	 * four times as much doing it.
	 */
	static constexpr double kComfortPerHead = 0.25;
	static constexpr int kComfortHeads = 4;
	/** Health a second at full comfort, and what it costs of each meter. */
	static constexpr double kComfortHeal = 1.0;
	static constexpr double kMaxRadiation = 100;
	/**
	 * Needs drain on Rust's timescale: roughly sixteen minutes of food and
	 * eleven of water from full, and running empty is a slow decline rather
	 * than a countdown.
	 */
	static constexpr double kCalorieDrain = 0.1;
	static constexpr double kHydrationDrain = 0.15;
	/**
	 * What an empty meter takes, a second.
	 *
	 * Nothing at all until it hits nought, and then a slow decline rather than
	 * a countdown: an empty stomach is a problem you have twenty minutes to
	 * solve, and an empty canteen is a problem you have eight. Thirst bites
	 * twice as hard as hunger, the way it does.
	 */
	static constexpr double kStarveDamage = 0.1;
	static constexpr double kThirstDamage = 0.2;
	static constexpr double kColdDamage = 0.45;
	/**
	 * What a full dose takes, a second, at the top of the scale.
	 *
	 * Twelve, not three. At three you could walk into the power plant with
	 * nothing on, take the worst the place has, and still have half a minute
	 * to loot and leave: the suit was a convenience. At twelve, an unprotected
	 * run into the hottest ground is over in about twenty seconds, start to
	 * finish, which is not enough to cross it. Suited, the intake is a tenth
	 * of that and the number never gets near the top of the scale.
	 */
	static constexpr double kRadDamage = 12.0;
	/**
	 * What a wound takes, a second.
	 *
	 * One health every two seconds: slow enough that a bleed is a thing you
	 * decide about rather than a thing you must answer at once, and a clean
	 * fraction, so what it is still going to cost comes out as a whole number
	 * of health on the screen rather than as a bar you have to guess at.
	 */
	static constexpr double kBleedDamage = 0.5;
	static constexpr double kComfortTemp = 10;
	/** How close you must be to pick up, harvest, open or use something. */
	static constexpr double kInteract = 39;
	/** A box is a big thing to stand at, so it is reached a little further. */
	static constexpr double kDeployableReachBonus = 20;
	/** How far from you a box, a fire or a bag can be put down. */
	static constexpr double kDeployReach = 200;
	/** How far out you can lay a foundation or a wall. */
	static constexpr double kBuildReach = 220;
	/** How far you may wander from an open box before it shuts itself. */
	static constexpr double kContainerSlack = 40;
};

/** Water is cold whatever the biome says. */
inline constexpr double kSwimTemperature = 8;
/** A fire warms this far, and by this much at the heart of it. */
inline constexpr double kFireWarmth = 26;
inline constexpr double kFireWarmthRadius = 150;
/** What a shore is worth to drink from. */
inline constexpr double kDrinkHydration = 34;

/** The ambient temperature of a biome, before night and fires. */
double biomeTemp(Biome biome);

/**
 * A second of being alive: needs drain, the cold bites, wounds bleed, and a
 * body that is fed and watered knits itself back together.
 *
 * `darkness` is how far into the night it is, nought to one.
 */
/**
 * `comfort` is how warm and how shared the fire you are at is, nought to one:
 * see PlayerVitals::kComfortPerHead. It only counts while both meters are
 * above kWellFed, because what it spends is the surplus.
 */
void updateSurvival(const World& world, Player& player, const Inventory& inventory, double dt,
					double darkness, double fireWarmth, double comfort = 0);

/** A blow landing on a player, after whatever they are wearing takes its share. */
void hurtPlayer(Player& player, Inventory& inventory, double amount);

/** Eating, drinking or applying something. Says whether it started. */
bool consume(Player& player, Inventory& inventory, ItemId id);

/** A use in progress. Running throws it away, as does taking a hit. */
void updateUse(Player& player, Inventory& inventory, double dt, bool sprinting);

/** A drink at the shore of a lake or a river. */
bool drink(const World& world, Player& player);

}  // namespace sim
