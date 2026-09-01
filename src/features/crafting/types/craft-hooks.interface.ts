import { Container } from 'src/features/items/types/container.interface';
import { ItemStack } from 'src/features/items/types/item-stack.interface';
import { PlayerState } from 'src/features/survival/types/player-state.interface';

/**
 * The crafting queue: what is being made, what is waiting, and what it cost.
 *
 * See docs/systems/inventory.md for the queue's contract, and
 * docs/architecture/project-structure.md for why this is a feature system
 * rather than another few hundred lines of the orchestrator.
 */
export interface CraftHooks {
    /** Everything the player can pay a recipe from. */
    containers(): Container[];
    /** Creative mode: no cost, no wait, no bench. */
    sandbox(): boolean;
    /** Where finished goods go, and who to hand overflow to. */
    player(): PlayerState;
    dropStack(stack: ItemStack, x: number, y: number): void;
    notify(text: string): void;
}
