/**
 * The lettering.
 *
 * Two faces, because one cannot do both jobs here. Most of this game's text is
 * drawn at ten to twelve pixels, and a proper comic display face is illegible
 * at that size: it is built to shout across a panel, not to label a stack of
 * eight iron.
 *
 * So `display` shouts, on the few things meant to be read from across the room,
 * and `body` carries everything you actually have to read. Both are bundled
 * with the game rather than fetched; see public/fonts/LICENCE.md.
 *
 * These are font-family fragments, to be used as `${weight} ${size}px ${face}`.
 * The fallbacks matter: the first frame can be drawn before a font has loaded,
 * and the game should look ordinary rather than broken in that moment.
 */
export const TYPE = {
    /** Panel titles, the contract line, cash, damage numbers, nametags. */
    display: "'Bangers', 'Comic Neue', ui-monospace, monospace",
    /** Item names, tooltips, counts, hints: anything read rather than glanced. */
    body: "'Comic Neue', ui-monospace, monospace",
    /**
     * Kept for anything that has to line up in columns.
     *
     * Comic Neue is proportional, so a changing number jitters its neighbours
     * about. Where digits sit in a row and are read as a table, monospace is
     * still the right answer and style comes second.
     */
    mono: 'ui-monospace, monospace',
} as const;
