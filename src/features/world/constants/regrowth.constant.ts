import { DAY_SECONDS } from 'src/features/time/constants/day-seconds.constant';

/**
 * How long anything harvested takes to come back: **one full in-game day**.
 *
 * One rule for every resource, so you never have to remember which. Fell a
 * tree, break a node, shoot a boar, empty a crate: that thing is gone until
 * this time tomorrow. It is what makes a stretch of coast you worked yesterday
 * worth walking past today, and what stops a base being parked on top of an ore
 * field and farmed on a five-minute loop.
 *
 * Spread is a small jitter either side, so a field cleared in one sweep does
 * not all pop back on the same tick.
 */
export const REGROWTH = {
    seconds: DAY_SECONDS,
    /** Fraction of a day of random slack, applied either way. */
    spread: 0.08,
    /**
     * Retry delay when the spot is still built over. Short on purpose: this is
     * not the resource's timer, it is a check to run again once the base that
     * is standing on it might have come down.
     */
    blockedRetrySeconds: 90,
} as const;
