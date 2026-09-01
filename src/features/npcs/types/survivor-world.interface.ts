import { Deployable } from 'src/features/building/types/deployable.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { ClanState } from 'src/features/clans/types/clan-state.interface';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { Npc } from 'src/features/npcs/types/npc.interface';
import { LootCrate } from 'src/features/world/types/loot-crate.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';

/**
 * Everything the survivor brain needs from the rest of the game. Keeping it
 * behind an interface means the AI cannot reach for anything a player could not.
 * See docs/systems/ai-survivor.md for the behaviour contract this implements.
 */
export interface SurvivorWorld {
    clanOf(clanId: number): ClanState | undefined;
    /** The clan's box: their stockpile, and what raiders come for. */
    clanStock(clanId: number): Container | null;
    deployableOf(clanId: number, kind: Deployable['kind']): Deployable | null;
    nearestNode(
        x: number,
        y: number,
        kinds: ResourceNode['kind'][],
        maxDist: number,
    ): ResourceNode | null;
    nearestAnimal(x: number, y: number, maxDist: number): Npc | null;
    nodeById(id: number): ResourceNode | null;
    npcById(id: number): Npc | null;
    nearestCrate(x: number, y: number, maxDist: number): LootCrate | null;
    /** Empty a crate into the survivor's hands. Returns what was taken. */
    lootCrate(crate: LootCrate): Partial<Record<ItemId, number>>;
    /** Somewhere the clan could sensibly extend its compound, if it can afford to. */
    expansionSite(
        clan: ClanState,
    ): { gx: number; gy: number; cost: Partial<Record<ItemId, number>> } | null;
    applyExpansion(
        clan: ClanState,
        gx: number,
        gy: number,
        cost: Partial<Record<ItemId, number>>,
    ): void;
    /** Anything actively hostile to this clan within range. */
    threatNear(
        x: number,
        y: number,
        clanId: number,
        maxDist: number,
    ): { x: number; y: number } | null;
    /** The clan piece most worth spending materials on, if they can afford it. */
    upgradeCandidate(
        clan: ClanState,
    ): { piece: Structure; cost: Partial<Record<ItemId, number>> } | null;
    applyUpgrade(clan: ClanState, piece: Structure, cost: Partial<Record<ItemId, number>>): void;
    damageNode(node: ResourceNode, amount: number): Partial<Record<ItemId, number>>;
    attackAnimal(attacker: Npc, target: Npc): void;
    isNight(): boolean;
    /** Move one step toward a point; returns the distance left. */
    step(npc: Npc, x: number, y: number, dt: number, speedScale?: number): number;
    say(npc: Npc, text: string, color: string): void;
}
