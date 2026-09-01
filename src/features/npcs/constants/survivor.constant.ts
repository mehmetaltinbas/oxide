/**
 * Survivor AI tuning. See docs/systems/ai-survivor.md: that document is the contract
 * for this behaviour, and should be updated alongside any change here.
 */
export const SURVIVOR = {
    /**
     * Needs, per second. Deliberately slow: at the old rate a survivor starved in
     * under four minutes, so a single failed hunt wiped a clan. Eating should be
     * a chore they attend to, not a countdown to death.
     */
    hungerRate: 0.13,
    fatigueRate: 0.09,
    fatigueNightMultiplier: 2.1,
    restRecovery: 7,
    eatValue: 55,
    starvingAt: 85,
    tiredAt: 70,
    /** Seconds between re-scoring goals; better clans think more often. */
    replanInterval: 2.4,
    /** Minimum seconds to stay committed to a goal, so they do not dither. */
    commitTime: 3,
    /** How much a survivor hauls before heading home. */
    carryCapacity: 90,
    /** Damage per gathering swing, and the interval between swings. */
    gatherDamage: 22,
    gatherInterval: 0.75,
    /** How close counts as "at" something. */
    reach: 40,
    /** Clan box levels the AI treats as comfortable. */
    targetWood: 600,
    targetStone: 500,
    targetMetalOre: 120,
    targetFood: 6,
    /** How far a survivor will travel from home for a goal. */
    workRadius: 760,
    /** Hunting ranges wider than ordinary work; game is thin on the ground. */
    huntRadius: 1250,
    lootRadius: 3000,
    /** Rest between monument runs, so looting does not crowd out real work. */
    lootCooldownSeconds: 150,
    /** Give up on a goal that has made no headway for this long. */
    stuckTimeout: 4.5,
    /** Seconds a clan member stays hostile after being provoked. */
    alertSeconds: 45,
    /** Anything within this of a provoked clansman gets told about it. */
    alertRadius: 420,
    /** Coarse progress for clans nobody is watching, per second. */
    offscreenGatherRate: 0.55,
    offscreenUpgradeChance: 0.004,
} as const;
