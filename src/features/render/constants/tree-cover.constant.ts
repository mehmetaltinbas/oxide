/**
 * A tree you are standing behind: when it counts, and how see-through it goes.
 *
 * `height` and `halfWidth` are in tree radii and come off the tiers the tree is
 * painted from: the crown's apex sits 3.0 radii above the trunk, and the widest
 * tier spreads 1.55 radii to each side.
 */
export const TREE_COVER = {
    height: 3.0,
    halfWidth: 1.55,
    /** Solid enough to still read as a tree, clear enough to see who is in it. */
    alpha: 0.45,
} as const;
