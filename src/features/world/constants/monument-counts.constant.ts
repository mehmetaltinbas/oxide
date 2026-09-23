/**
 * How many of each monument an island gets.
 *
 * The four named places are one each: an island has one Airfield, one Power
 * Plant, one Military Base and one town, so "meet at the Airfield" means
 * something. Cabins and lighthouses are scenery you come across, so there are
 * several of them.
 */
export const MONUMENT_COUNTS: Record<string, number> = {
    cabins: 8,
    lighthouse: 6,
    airfield: 1,
    powerplant: 1,
    military: 1,
    town: 1,
};
