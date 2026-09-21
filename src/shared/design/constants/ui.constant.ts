/**
 * Colour tokens: the single source for every UI colour in the game.
 *
 * Nothing that draws chrome picks a colour of its own: it names one from here.
 * Changing the look of the whole interface is changing values in this file.
 * See docs/design-system/design-tokens.md for the rule and its exceptions.
 */

export const UI = {
    /** Lines between the squares of the map grid: visible, never louder than the map. */
    mapGrid: 'rgba(255, 255, 255, 0.28)',
    /**
     * Panels are dark translucent, the same material as the belt slots, so the
     * whole interface reads as one thing and you can still see the world through
     * an open menu.
     */
    surface: 'rgba(16,18,16,0.82)',
    surfaceGlass: 'rgba(16,18,16,0.72)',
    surfaceAlt: 'rgba(255,255,255,0.06)',
    hairline: 'rgba(255,255,255,0.10)',
    hover: 'rgba(255,255,255,0.10)',
    selected: 'rgba(10,110,235,0.30)',
    slotEdge: 'rgba(255,255,255,0.08)',
    slotEdgeHover: 'rgba(120,180,255,0.7)',
    ink: '#f2f1ec',
    subtle: 'rgba(242,241,236,0.62)',
    disabled: 'rgba(242,241,236,0.30)',
    disabledFill: 'rgba(255,255,255,0.05)',
    accent: '#0a6eeb',
    accentHover: '#2b84f2',
    accentInk: '#8fbcff',
    buttonOff: 'rgba(255,255,255,0.08)',
    ok: '#5cd07a',
    warn: '#ff7a62',
    hot: '#ffa64a',
    scrim: 'rgba(0,0,0,0.34)',

    // ---- overlay context: HUD furniture floating on the dark world.
    // Transparent, frameless, light text. See docs/systems/ui.md.
    glass: 'rgba(18, 20, 18, 0.30)',
    glassEdge: 'rgba(255,255,255,0.07)',
    onDark: '#f2f1ec',
    onDarkSubtle: 'rgba(242,241,236,0.60)',
    onDarkFaint: 'rgba(242,241,236,0.34)',
    track: 'rgba(0,0,0,0.38)',
} as const;
