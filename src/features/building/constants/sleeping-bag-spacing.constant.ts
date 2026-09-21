/**
 * How far apart two sleeping bags must be, in world units.
 *
 * Rust's rule: you may own as many bags as you like, but not two in the same
 * spot, so a bag is a claim on a place rather than a stack of extra lives in
 * one room. 320 is five building cells: a bag per base, or one each side of a
 * big one. A first guess, not a measurement; tune it by playing.
 */
export const SLEEPING_BAG_SPACING = 320;
