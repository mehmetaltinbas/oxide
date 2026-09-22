/**
 * How a bow is held, in figure units (forward is negative y): the bow arm
 * straight out ahead of the face, and the string hand at the nock, which
 * starts just behind the grip and comes back `pull` units at full draw.
 * `nockRest` and `pull` match the string in `drawBow`.
 */
export const BOW_HOLD = {
    grip: [1.5, -17] as [number, number],
    nockRest: 4.5,
    pull: 9,
};
