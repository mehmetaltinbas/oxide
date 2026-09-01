import { Game } from 'src/app/game';

/**
 * A read-only view of the game, for code that draws the world.
 *
 * Drawing reads state; it must never advance it. Passing `Game` itself made
 * that a convention nobody could check, so the renderer takes this instead and
 * the compiler refuses any assignment to a top-level field. It is a shallow
 * readonly, so it does not stop someone reaching into a nested object, but it
 * does stop the mistake that actually happens: quietly moving the simulation
 * on during a draw call.
 *
 * See docs/architecture/code-conventions.md.
 */
export type GameView = Readonly<Game>;
