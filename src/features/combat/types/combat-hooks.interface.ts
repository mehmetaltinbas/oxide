import { Faction } from 'src/features/combat/types/faction.type';
import { Npc } from 'src/features/npcs/types/npc.interface';

export interface CombatHooks {
    npcs(): Npc[];
    playerPos(): { x: number; y: number; radius: number; alive: boolean };
    hitNpc(npc: Npc, damage: number, fromX: number, fromY: number, byClan: number): void;
    hitPlayer(damage: number, fromX: number, fromY: number): void;
    hitStructure(id: number, damage: number, fromX: number, fromY: number, melee: boolean): void;
    explosion(
        x: number,
        y: number,
        radius: number,
        damage: number,
        owner: Faction,
        stuckTo: number | null,
    ): void;
}
