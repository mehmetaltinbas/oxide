#include "sim/features/survival/systems/survival.hpp"
#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/food.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/survival/types/player.struct.hpp"
#include "sim/features/world/constants/daylight.constant.hpp"
#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/survival/utils/channelled.util.hpp"

#include <algorithm>
#include <cmath>

namespace sim {

double biomeTemp(Biome biome) {
	switch (biome) {
		case Biome::Grass: return 20;
		case Biome::Forest: return 16;
		case Biome::Beach: return 22;
		// Sand under snow: as cold as the snowfield it edges.
		case Biome::SnowBeach: return -6;
		case Biome::Desert: return 34;
		case Biome::Snow: return -8;
		case Biome::Road: return 20;
		case Biome::Water: return 12;
	}
	return 20;
}

void updateSurvival(const World& world, Player& player, const Inventory& inventory, double dt,
					double darkness, double fireWarmth, double comfort) {
	if (!player.alive) {
		player.respawnTimer -= dt;
		return;
	}
	if (player.invuln > 0) player.invuln -= dt;
	if (player.hurtFlash > 0) player.hurtFlash -= dt;

	player.calories = std::max(0.0, player.calories - PlayerVitals::kCalorieDrain * dt);
	player.hydration = std::max(0.0, player.hydration - PlayerVitals::kHydrationDrain * dt);

	// Temperature: the biome, the hour, what you have on, and any fire nearby.
	const Biome biome = world.biomeAt(player.x, player.y);
	double ambient = (biome == Biome::Water ? kSwimTemperature : biomeTemp(biome)) - darkness * 18;
	ambient += fireWarmth;
	ambient += itemDef(inventory.worn().id).wear.warmth;
	player.temperature += (ambient - player.temperature) * std::min(1.0, dt * 0.5);

	// Radiation, kept out by a suit, and shed slowly once you are out of it.
	const double rads = world.radiationAt(player.x, player.y);
	const double protection = itemDef(inventory.worn().id).wear.radiation;
	if (rads > 0) {
		player.radiation =
			std::min(PlayerVitals::kMaxRadiation, player.radiation + rads * (1 - protection) * dt);
	} else {
		player.radiation = std::max(0.0, player.radiation - 2.2 * dt);
	}

	double damage = 0;
	if (player.calories <= 0) damage += PlayerVitals::kStarveDamage;
	if (player.hydration <= 0) damage += PlayerVitals::kThirstDamage;
	if (player.temperature < PlayerVitals::kComfortTemp - 12) {
		// Scales with how cold it actually is, so a chilly night is survivable
		// and a snowstorm without clothing is not.
		const double below = PlayerVitals::kComfortTemp - 12 - player.temperature;
		damage += PlayerVitals::kColdDamage * (1 + std::min(2.5, below / 14));
	}
	if (player.radiation > 45) damage += PlayerVitals::kRadDamage * ((player.radiation - 45) / 55);
	if (player.bleeding > 0) {
		damage += PlayerVitals::kBleedDamage;
		player.bleeding -= dt;
	}
	if (damage > 0) {
		player.health -= damage * dt;
		if (player.health <= 0) {
			player.health = 0;
			player.alive = false;
		}
	}

	// Comfort: what a fire and the company round it are worth.
	//
	// Only on the surplus. Both meters have to be above kWellFed, and what it
	// heals it takes straight back out of them on top of the ordinary drain,
	// so sitting at a fire is spending food to buy health rather than getting
	// health for nothing.
	player.comfort = 0;
	if (comfort > 0 && player.calories > PlayerVitals::kWellFed &&
		player.hydration > PlayerVitals::kWellFed && damage == 0) {
		player.comfort = std::min(1.0, comfort);
		const double rate = player.comfort * PlayerVitals::kComfortHeal;
		player.health = std::min(PlayerVitals::kMaxHealth, player.health + rate * dt);
		player.calories = std::max(0.0, player.calories - rate * dt);
		player.hydration = std::max(0.0, player.hydration - rate * dt);
	}

	if (player.healOverTime > 0) {
		// One a second, so twenty health is twenty seconds and the number
		// beside the bar counts down at the rate it reads.
		const double tick = std::min(player.healOverTime, 1.0 * dt);
		player.health = std::min(PlayerVitals::kMaxHealth, player.health + tick);
		player.healOverTime -= tick;
	} else if (player.comfort <= 0 && player.calories > 40 && player.hydration > 40 &&
			   damage == 0) {
		// Well fed and watered, you slowly knit back together. Not while a
		// fire is doing it for you: the two would stack and a camp would mend
		// faster than a syringe.
		player.health = std::min(PlayerVitals::kMaxHealth, player.health + 0.6 * dt);
	}
}

void hurtPlayer(Player& player, Inventory& inventory, double amount) {
	if (!player.alive || player.invuln > 0) return;
	const double armor = itemDef(inventory.worn().id).wear.armor;
	player.health -= amount * (1 - armor);
	// A hit does not cost you what you were in the middle of. It used to wipe
	// the bandage, the syringe and the reload, which meant the moment you most
	// needed any of them was the moment you could not finish one: a wolf on
	// you cancelled the dressing every second it landed a bite.
	player.invuln = 0.35;
	player.hurtFlash = 0.3;
	// Some blows open you up, and a wound that is not dressed keeps taking.
	Rng rng(static_cast<std::uint32_t>((player.x + player.y + amount) * 37.0));
	if (rng.unit() < 0.3) player.bleeding = std::max(player.bleeding, 6.0);
	if (player.health <= 0) {
		player.health = 0;
		player.alive = false;
	}
}

namespace {

/** What a consumable does, once it is actually swallowed or applied. */
void apply(Player& player, Inventory& inventory, ItemId id) {
	const Food& food = itemDef(id).food;
	if (inventory.take(id, 1) < 1) return;
	if (food.calories > 0) {
		player.calories = std::min(PlayerVitals::kMaxCalories, player.calories + food.calories);
	}
	if (food.hydration > 0) {
		player.hydration = std::min(PlayerVitals::kMaxHydration, player.hydration + food.hydration);
	}
	if (food.health > 0) {
		player.health = std::min(PlayerVitals::kMaxHealth, player.health + food.health);
	} else if (food.health < 0) {
		// Raw meat: it feeds you and it costs you.
		player.health = std::max(1.0, player.health + food.health);
	}
	// What comes back slowly, what stops the bleeding, and what clears the
	// dose: three separate things a thing can do, on the definition rather
	// than on the item's name. A bandage stops fifty seconds of bleeding and
	// a syringe stops none, which is why the two are worth carrying together.
	if (food.overTime > 0) player.healOverTime += food.overTime;
	if (food.bleedCure > 0) player.bleeding = std::max(0.0, player.bleeding - food.bleedCure);
	if (food.radCure > 0) player.radiation = std::max(0.0, player.radiation - food.radCure);
}

bool isFood(const Food& food) {
	// Health can be negative and the thing still be food: raw meat feeds you
	// and makes you ill at the same time.
	return food.calories > 0 || food.hydration > 0 || food.health != 0;
}

}  // namespace

bool consume(Player& player, Inventory& inventory, ItemId id) {
	const Food& food = itemDef(id).food;
	if (!isFood(food)) return false;
	if (inventory.count(id) <= 0) return false;
	if (food.useSeconds > 0) {
		player.applying = id;
		player.useLeft = food.useSeconds;
		player.useTotal = food.useSeconds;
		player.attackTimer = food.useSeconds;
		return true;
	}
	apply(player, inventory, id);
	return true;
}

void updateUse(Player& player, Inventory& inventory, double dt, bool sprinting) {
	if (player.applying == ItemId::None) return;
	// Put it away and you stop applying it: the same question a reload asks,
	// for the same reason. See docs/systems/channelled-actions.md.
	if (!stillInHand(inventory, player.applying)) {
		stopChannelling(player);
		return;
	}
	if (sprinting || player.swimming) {
		// You can walk through a bandage. You cannot run through one, and you
		// certainly cannot wind one while treading water: both hands are busy
		// keeping you up.
		stopChannelling(player);
		return;
	}
	player.useLeft -= dt;
	if (player.useLeft > 0) return;
	const ItemId id = player.applying;
	player.applying = ItemId::None;
	player.useLeft = 0;
	apply(player, inventory, id);
}

bool drink(const World& world, Player& player) {
	// Fresh water only: the sea is no use to anybody.
	if (!world.freshAt(player.x, player.y)) return false;
	player.hydration = std::min(PlayerVitals::kMaxHydration, player.hydration + kDrinkHydration);
	return true;
}

}  // namespace sim
