/**
 * A tree you are standing behind: when it counts, and how see-through it goes.
 *
 * `height` and `halfWidth` are in tree radii and come off the tiers the tree is
 * painted from: the crown's apex sits 3.0 radii above the trunk, and the widest
 * tier spreads 1.55 radii to each side, before each tree's own variation.
 */
export const TREE_COVER = {
    // Grown to hold the widest and tallest a tree can come out now that each
    // one varies: up to 8% taller, and the bottom tier up to 12% wider plus
    // its lean.
    height: 3.3,
    halfWidth: 1.8,
    /** Solid enough to still read as a tree, clear enough to see who is in it. */
    alpha: 0.45,
} as const;
