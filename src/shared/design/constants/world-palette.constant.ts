/**
 * Colour tokens for the world layer, as opposed to the interface chrome in
 * `UI`. These are the values the renderer reuses across many different things:
 * the shadow under every actor, the backing behind every health bar, the red
 * that always means hostile.
 *
 * Per-entity colours: an item's colour, a biome's colour, a clan's colour -
 * are NOT tokens. They live on that entity's definition, which is already the
 * single source for them. See docs/design-system/design-tokens.md.
 */
export const WORLD = {
    /** Contact shadow under actors, trees and rocks. */
    shadow: 'rgba(0,0,0,0.3)',
    /** Deeper shadow for large objects. */
    shadowDeep: 'rgba(0,0,0,0.35)',
    /** Backing plate behind a health bar or a progress sliver. */
    barTrack: 'rgba(0,0,0,0.65)',
    /** Anything hostile: enemy health, damage flashes, danger markers. */
    hostile: '#d8483a',
    /** Fire, muzzle flash, explosions. */
    fire: '#ff8c2e',
    emberHot: '#ffd98a',
    /** Blood and gore bursts. */
    blood: '#7a1c1c',
    /** Wood splinters and structural debris. */
    debris: '#6d4a28',
    /** The roof over a sealed room, and its shingle hatching. */
    roof: '#2b2721',
    roofHatch: 'rgba(0,0,0,0.22)',
    /** Name tag above an actor, and its drop shadow. */
    nameTag: '#e8b0a0',
    nameTagShadow: 'rgba(0,0,0,0.8)',
    /** Healing, growth, anything restorative. */
    vital: '#8cf08c',

    /** The void behind everything, before the terrain is painted. */
    voidBackdrop: '#0a0c0a',
    /** Sand, dry grass, anything bleached. */
    sand: '#d9b98a',
    /**
     * Bare skin.
     *
     * Deliberately not `sand`: the head used to borrow it, and once the torso
     * did too an unclothed player standing on a beach was the same colour as
     * the beach. Warm and a shade darker, so a person reads against sand and
     * grass alike.
     */
    skin: '#c08a5e',
    /** The shadowed side of skin, for the line under a chin or a shoulder. */
    skinShade: '#9c6a44',
    /** Struck metal, sparks, anything that reads as valuable. */
    gold: '#c9a227',
    /** Warm lamplight and lit windows. */
    lamp: '#e8c87a',
    /** Radiation, and anything else chemically wrong. */
    hazard: '#b4e65a',
    /** A hostile muzzle flash and the tracer behind it. */
    hostileTracer: '#ff9a6a',
    /** The glow around something about to go off. */
    fuse: '#ff6b4a',
    /** Milled timber and planking. */
    timber: '#8a6034',
    /** The two browns everything wooden is built from. */
    woodDark: '#3f2f22',
    woodShade: '#2f2b22',
    /** Fallback for anything owned by a faction with no colour of its own. */
    unclaimed: '#8a8a8a',
    unclaimedWarm: '#c86a4a',
} as const;
