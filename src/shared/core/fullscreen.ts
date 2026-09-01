/**
 * Fullscreen, as the browser exposes it.
 *
 * Entering has to happen inside a user gesture, which is why this is only ever
 * called from a click in the settings panel and never from the game loop. The
 * canvas resizes on its own: the window `resize` listener in main.ts fires when
 * the browser changes the viewport, and fullscreen is just another such change.
 */
export function isFullscreen(): boolean {
    return document.fullscreenElement !== null;
}

/** Returns the state it is heading toward, not the state it is in yet. */
export function toggleFullscreen(): boolean {
    if (isFullscreen()) {
        void document.exitFullscreen?.();
        return false;
    }
    // The whole document rather than the canvas, so the page background fills
    // the letterboxing instead of the browser's own black.
    void document.documentElement.requestFullscreen?.();
    return true;
}

/**
 * When fullscreen last ended, in milliseconds on the performance clock.
 *
 * The browser reserves [Esc] for leaving fullscreen and a page cannot prevent
 * it. Without this the one keypress did two jobs: it dropped out of fullscreen
 * **and** closed whatever menu was open, so turning fullscreen on from the
 * settings panel and pressing [Esc] to close the panel left you with neither.
 */
let leftFullscreenAt = -Infinity;

document.addEventListener('fullscreenchange', () => {
    if (!isFullscreen()) leftFullscreenAt = performance.now();
});

/** How long an [Esc] counts as having been spent on leaving fullscreen. */
const ESCAPE_GRACE_MS = 350;

/**
 * Did this [Esc] go on leaving fullscreen?
 *
 * Call it from the Escape handler and do nothing when it answers true. The
 * keypress is spent; the next one closes the menu, which is the behaviour a
 * player expects from a key that does one thing at a time.
 *
 * Single-shot: asking consumes the grace, so two menus cannot both swallow the
 * same keypress.
 */
export function escapeSpentOnFullscreen(): boolean {
    if (performance.now() - leftFullscreenAt > ESCAPE_GRACE_MS) return false;
    leftFullscreenAt = -Infinity;
    return true;
}
