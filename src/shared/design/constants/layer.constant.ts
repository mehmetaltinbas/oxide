/**
 * Draw order, low to high. A canvas has no z-index: what is painted last wins,
 * so the ordering lives in the sequence of draw calls. Naming the layers here
 * makes that order reviewable in one place rather than inferred by reading the
 * whole renderer, and gives a new layer somewhere deliberate to slot in.
 *
 * See docs/design-system/design-tokens.md.
 */
export const LAYER = {
    terrain: 0,
    groundDecal: 100,
    structure: 200,
    deployable: 300,
    groundItem: 400,
    actor: 500,
    projectile: 600,
    roof: 700,
    particle: 800,
    nameTag: 900,
    worldOverlay: 1000,
    hudPanel: 1100,
    hudOverlay: 1200,
    dragged: 1300,
} as const;
