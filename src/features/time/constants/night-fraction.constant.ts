/**
 * Share of the cycle that counts as night. Both the night flag and the
 * darkness overlay derive from this one number: they used to be computed
 * separately and disagreed by well over an hour in each direction.
 */
export const NIGHT_FRACTION = 0.3;
