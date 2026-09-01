export const PLAYER = {
    radius: 13,
    maxHealth: 100,
    speed: 132,
    sprint: 1.4,
    maxCalories: 100,
    maxHydration: 100,
    /**
     * Needs drain on Rust's timescale, not a survival-horror one: roughly 16
     * minutes of food and 11 of water from full, and running empty is a slow
     * decline rather than a countdown.
     */
    calorieDrain: 0.1,
    hydrationDrain: 0.15,
    /** Damage per second while starving or parched. */
    starveDamage: 0.3,
    /** Freezing hurts, but on Rust's timescale: a slow decline you can outrun. */
    coldDamage: 0.45,
    radDamage: 3.0,
    maxRadiation: 100,
    comfortTemp: 10,
    interact: 78,
} as const;
