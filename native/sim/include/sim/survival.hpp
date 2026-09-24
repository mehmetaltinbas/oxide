#pragma once

#include "sim/build.hpp"
#include "sim/inventory.hpp"
#include "sim/player.hpp"
#include "sim/world.hpp"

namespace sim {

/** What a body needs, and what it can take. */
struct PlayerVitals {
    static constexpr double kMaxHealth = 100;
    static constexpr double kMaxCalories = 100;
    static constexpr double kMaxHydration = 100;
    static constexpr double kMaxRadiation = 100;
    /**
     * Needs drain on Rust's timescale: roughly sixteen minutes of food and
     * eleven of water from full, and running empty is a slow decline rather
     * than a countdown.
     */
    static constexpr double kCalorieDrain = 0.1;
    static constexpr double kHydrationDrain = 0.15;
    static constexpr double kStarveDamage = 0.3;
    static constexpr double kColdDamage = 0.45;
    static constexpr double kRadDamage = 3.0;
    static constexpr double kComfortTemp = 10;
    /** How close you must be to pick up, harvest, open or use something. */
    static constexpr double kInteract = 39;
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
void updateSurvival(const World& world, Player& player, const Inventory& inventory, double dt,
                    double darkness, double fireWarmth);

/** A blow landing on a player, after whatever they are wearing takes its share. */
void hurtPlayer(Player& player, Inventory& inventory, double amount);

/** Eating, drinking or applying something. Says whether it started. */
bool consume(Player& player, Inventory& inventory, ItemId id);

/** A use in progress. Running throws it away, as does taking a hit. */
void updateUse(Player& player, Inventory& inventory, double dt, bool sprinting);

/** A drink at the shore of a lake or a river. */
bool drink(const World& world, Player& player);

}  // namespace sim
