import { PlayerState } from 'src/features/survival/types/player-state.interface';

/**
 * The AI clans: where they settle, what they build, how they grow, and when
 * they come for you.
 *
 * This used to live in the orchestrator, which is how the orchestrator got to
 * two and a half thousand lines. Everything about a clan's life is here now,
 * and the four things it needs from outside the feature arrive through
 * `ClanHooks`. See docs/architecture/project-structure.md.
 */
export interface ClanHooks {
    /** Seconds since the world began, for tech and raid pacing. */
    clock(): number;
    /** Clans behave differently after dark. */
    isNight(): boolean;
    /** Put a line on the player's screen. */
    notify(text: string): void;
    /** Where the player is, and whether they are still standing. */
    player(): PlayerState;
}
