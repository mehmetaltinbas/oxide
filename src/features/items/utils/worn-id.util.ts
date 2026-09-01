import { ItemId } from 'src/features/items/types/item-id.type';
import { PlayerState } from 'src/features/survival/types/player-state.interface';

/** What the player has on, or null. */
export function wornId(p: PlayerState): ItemId | null {
    return p.worn.slots[0]?.id ?? null;
}
