/**
 * How far in the past other players are drawn, in seconds.
 *
 * The server sends 30 states a second, so an update is up to 33ms stale before
 * the next arrives. Drawing whatever came last means everybody else jumps 33ms
 * at a time while your own survivor, predicted locally, glides. Stepped motion
 * reads far worse than smooth motion at the same delay.
 *
 * Holding the picture back by about three ticks means there is nearly always a
 * state on both sides of the moment being drawn, so it can be smoothed between
 * them. The cost is seeing other players 100ms behind where they are, on top of
 * the network delay, which is the trade every game with moving players makes.
 */
export const INTERPOLATION_DELAY = 0.1;

/** How many states to keep, enough that a burst of jitter cannot empty it. */
export const SNAPSHOT_BUFFER = 12;
