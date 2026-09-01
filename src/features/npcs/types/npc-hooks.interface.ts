import { Structure } from 'src/features/building/types/structure.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { Npc } from 'src/features/npcs/types/npc.interface';

export interface NpcHooks {
    /**
     * A wild animal died: put one back on the island a day from now, somewhere
     * else. The island keeps its population, but the stretch you hunted out
     * stays hunted out for the day.
     */
    scheduleRegrowth(kind: NpcKind): void;
    damagePlayer(amount: number, fromX: number, fromY: number): void;
    fire(
        x: number,
        y: number,
        angle: number,
        damage: number,
        speed: number,
        range: number,
        clan: number,
    ): void;
    /** A raider swinging at, or planting a charge on, one of your pieces. */
    attackStructure(id: number, amount: number, fromX: number, fromY: number): void;
    plantCharge(npc: Npc, structure: Structure): void;
    dropLoot(
        kind: NpcKind,
        x: number,
        y: number,
        loot: Partial<Record<ItemId, [number, number]>>,
        weapon: ItemId | null,
    ): void;
    /** Where the raid should be heading, if a raid is on. */
    raidTarget(): { x: number; y: number } | null;
    weakestStructureNear(x: number, y: number, radius: number): Structure | null;
}
