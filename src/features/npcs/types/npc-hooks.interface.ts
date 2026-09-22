import { ItemId } from 'src/features/items/types/item-id.type';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';

export interface NpcHooks {
    /**
     * A wild animal died: put one back on the island a day from now, somewhere
     * else. The island keeps its population, but the stretch you hunted out
     * stays hunted out for the day.
     */
    scheduleRegrowth(kind: NpcKind): void;
    damagePlayer(amount: number, fromX: number, fromY: number): void;
    fire(x: number, y: number, angle: number, damage: number, speed: number, range: number): void;
    dropLoot(
        kind: NpcKind,
        x: number,
        y: number,
        loot: Partial<Record<ItemId, [number, number]>>,
        weapon: ItemId | null,
    ): void;
}
