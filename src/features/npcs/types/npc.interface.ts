import { ItemId } from 'src/features/items/types/item-id.type';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { SurvivorGoal } from 'src/features/npcs/types/survivor-goal.type';

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
    state: 'idle' | 'wander' | 'chase' | 'attack' | 'flee' | 'raid' | 'gather' | 'return';
    stateTime: number;
    attackTimer: number;
    gunTimer: number;
    flash: number;
    knockX: number;
    knockY: number;
    /** How long it has been waiting at the shore for you to come out. */
    shoreWait: number;
    animPhase: number;
    /** Where this npc belongs: a monument, a clan base, or nothing. */
    homeX: number;
    homeY: number;
    leash: number;
    /** Clan index + 1 for clan units, 0 for wildlife and scientists. */
    clan: number;
    /**
     * The gun this one is actually carrying, which is what falls off the body.
     * Its shooting stats still come from the NPC definition, not from the item:
     * a raider with a rifle slung on their back does not hit like a player with
     * a rifle, and balancing them together is a separate job.
     */
    weapon: ItemId | null;
    /** Raiders carry charges and a target structure. */
    charges: number;
    targetStructure: number | null;

    // ---- survivor simulation (clan members only). See docs/systems/ai-survivor.md.
    hunger: number;
    fatigue: number;
    /** What this survivor is carrying but has not banked yet. */
    carry: Partial<Record<ItemId, number>>;
    goal: SurvivorGoal;
    /** Seconds until the next re-plan, and how long the current goal is locked. */
    planTimer: number;
    commit: number;
    /** Whatever the current goal is working on. */
    targetNodeId: number | null;
    targetNpcId: number | null;
    targetDeployableId: number | null;
    targetCrateId: number | null;
    workTimer: number;
    /** Seconds this clansman stays hostile after being provoked. */
    alert: number;
    /** Progress watchdog: survivors abandon a goal they cannot reach. */
    stuckTimer: number;
    lastX: number;
    lastY: number;
    /** Brief cooldown after giving up, so they do not re-pick the same target. */
    blacklistUntil: number;
    /** How long the current goal has been running, so it can time out. */
    goalAge: number;
    /** Seconds before this survivor will consider hunting again. */
    huntCooldown: number;
    /** Same, for monument runs, otherwise looting crowds out everything else. */
    lootCooldown: number;
    /** How long the current pursuit has run, so it can be given up on. */
    pursuit: number;
    /** Seconds spent deliberately breaking off; alarms do not re-trigger during it. */
    disengage: number;
    /** Clan of whoever last hurt this npc, so a kill can credit the killer. */
    lastHitByClan: number;
    /** Current route, its cursor, and when it may next be recomputed. */
    path: { x: number; y: number }[] | null;
    pathIndex: number;
    repathIn: number;
    pathGoalX: number;
    pathGoalY: number;
}
