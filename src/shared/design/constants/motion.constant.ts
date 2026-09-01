/**
 * Motion tokens, in seconds. Animation lengths are picked from this scale so
 * the whole interface moves at a consistent tempo.
 * See docs/design-system/design-tokens.md.
 */
export const MOTION = {
    /** A flash on a hit, a swing, anything that should barely register. */
    instant: 0.12,
    /** Button and hover feedback. */
    fast: 0.2,
    /** Panels, fades, camera settles. */
    normal: 0.35,
    /** Day/night and other ambient transitions. */
    slow: 1.0,
} as const;
