#include "sim/survival.hpp"

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
                    double darkness, double fireWarmth) {
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
    if (player.hydration <= 0) damage += PlayerVitals::kStarveDamage * 1.4;
    if (player.temperature < PlayerVitals::kComfortTemp - 12) {
        // Scales with how cold it actually is, so a chilly night is survivable
        // and a snowstorm without clothing is not.
        const double below = PlayerVitals::kComfortTemp - 12 - player.temperature;
        damage += PlayerVitals::kColdDamage * (1 + std::min(2.5, below / 14));
    }
    if (player.radiation > 45) damage += PlayerVitals::kRadDamage * ((player.radiation - 45) / 55);
    if (player.bleeding > 0) {
        damage += 1.6;
        player.bleeding -= dt;
    }
    if (damage > 0) {
        player.health -= damage * dt;
        if (player.health <= 0) {
            player.health = 0;
            player.alive = false;
        }
    }

    if (player.healOverTime > 0) {
        const double tick = std::min(player.healOverTime, 12 * dt);
        player.health = std::min(PlayerVitals::kMaxHealth, player.health + tick);
        player.healOverTime -= tick;
    } else if (player.calories > 40 && player.hydration > 40 && damage == 0) {
        // Well fed and watered, you slowly knit back together.
        player.health = std::min(PlayerVitals::kMaxHealth, player.health + 0.6 * dt);
    }
}

void hurtPlayer(Player& player, Inventory& inventory, double amount) {
    if (!player.alive || player.invuln > 0) return;
    const double armor = itemDef(inventory.worn().id).wear.armor;
    player.health -= amount * (1 - armor);
    // A hit costs you whatever you were in the middle of.
    player.applying = ItemId::None;
    player.useLeft = 0;
    player.reloadLeft = 0;
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
        // A syringe works over a few seconds; everything else is at once.
        if (id == ItemId::Medkit) {
            player.healOverTime += food.health;
        } else {
            player.health = std::min(PlayerVitals::kMaxHealth, player.health + food.health);
        }
        player.bleeding = 0;
    }
}

bool isFood(const Food& food) {
    return food.calories > 0 || food.hydration > 0 || food.health > 0;
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
    if (sprinting) {
        // You can walk through a bandage. You cannot run through one.
        player.applying = ItemId::None;
        player.useLeft = 0;
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
