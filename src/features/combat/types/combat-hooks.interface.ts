import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { Faction } from 'src/features/combat/types/faction.type';
import { Npc } from 'src/features/npcs/types/npc.interface';

export interface CombatHooks {
    npcs(): Npc[];
    playerPos(): { x: number; y: number; radius: number; alive: boolean };
    hitNpc(npc: Npc, damage: number, fromX: number, fromY: number): void;
    hitPlayer(damage: number, fromX: number, fromY: number): void;
    /** A round has struck a barrel or the like. */
    hitBreakable(node: ResourceNode, damage: number): void;
    hitStructure(id: number, damage: number, fromX: number, fromY: number, melee: boolean): void;
    explosion(
        x: number,
        y: number,
        radius: number,
        damage: number,
        owner: Faction,
        stuckTo: number | null,
        /** A blast that damages every piece it reaches, not only the one it is on. */
        splashStructures?: boolean,
    ): void;
}
