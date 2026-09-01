/**
 * Corner-radius scale. Pick from the scale; never write a raw radius in a
 * draw call. See docs/design-system/design-tokens.md.
 */
export const RADIUS = {
    /** Belt slots, small chips, inline buttons. */
    sm: 6,
    /** Standard buttons, item wells, tab strips. */
    md: 10,
    /** Full panels and cards. */
    lg: 16,
} as const;
