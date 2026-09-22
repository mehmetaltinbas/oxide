import { ItemId } from 'src/features/items/types/item-id.type';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';

/**
 * Something alive that is not you: the wildlife, and the scientists who hold
 * the monuments. They wander near where they belong, chase what provokes them,
 * and go home again.
 */
export interface Npc {
    id: number;
    kind: NpcKind;
    x: number;
    y: number;
    vx: number;
    vy: number;
    facing: number;
    hp: number;
    maxHp: number;
    radius: number;
    state: 'idle' | 'wander' | 'chase' | 'attack' | 'flee' | 'return';
    stateTime: number;
    attackTimer: number;
    gunTimer: number;
    flash: number;
    knockX: number;
    knockY: number;
    /** How long it has been waiting at the shore for you to come out. */
    shoreWait: number;
    animPhase: number;
    /** Where this one belongs: a monument, or where it was born. */
    homeX: number;
    homeY: number;
    leash: number;
    /**
     * The gun this one is actually carrying, which is what falls off the body.
     * Its shooting stats still come from the NPC definition, not from the item:
     * a scientist with a rifle does not hit like a player with a rifle, and
     * balancing them together is a separate job.
     */
    weapon: ItemId | null;
    /** Current route, its cursor, and when it may next be recomputed. */
    path: { x: number; y: number }[] | null;
    pathIndex: number;
    repathIn: number;
    pathGoalX: number;
    pathGoalY: number;
}
