import { Container } from 'src/features/items/types/container.interface';
import { ItemStack } from 'src/features/items/types/item-stack.interface';
import { PlayerState } from 'src/features/survival/types/player-state.interface';

/**
 * Staying alive: hunger, thirst, temperature, radiation, bleeding, healing,
 * and the channels that interrupt when you break into a run.
 *
 * Dying is not here. A death changes the game's phase, and phases belong to the
 * orchestrator. See docs/architecture/project-structure.md.
 */
export interface SurvivalHooks {
    player(): PlayerState;
    containers(): Container[];
    heldItem(): ItemStack | null;
    /** Creative mode: nothing drains and nothing hurts. */
    sandbox(): boolean;
    /** How dark it is right now, which is most of how cold it is. */
    darkness(): number;
    dropStack(stack: ItemStack, x: number, y: number): void;
    notify(text: string): void;
    /** The player ran out of health. */
    die(): void;
}
