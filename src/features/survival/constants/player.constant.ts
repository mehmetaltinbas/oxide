import PLAYER_NUMBERS from 'shared/player.json';

export const PLAYER = {
    radius: PLAYER_NUMBERS.radius,
    maxHealth: 100,
    speed: PLAYER_NUMBERS.speed,
    sprint: PLAYER_NUMBERS.sprint,
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
    /**
     * How close you must be to pick up, harvest, open or use something.
     *
     * Halved from 78: at that range the prompt came up from well across a
     * clearing, and with two things nearby it was a guess which one E would
     * take. At 39 you have to be standing at it.
     */
    interact: 39,
} as const;
