import { SURVIVOR } from 'src/features/npcs/constants/survivor.constant';
import { Npc } from 'src/features/npcs/types/npc.interface';
import { randRange } from 'src/shared/utils/rand-range.util';

/** Fresh survivor state for a newly spawned clan member. */
export function initSurvivor(npc: Npc): void {
    npc.hunger = randRange(Math.random, 10, 45);
    npc.fatigue = randRange(Math.random, 0, 35);
    npc.carry = {};
    npc.goal = 'idle';
    npc.planTimer = randRange(Math.random, 0, SURVIVOR.replanInterval);
    npc.commit = 0;
    npc.targetNodeId = null;
    npc.targetNpcId = null;
    npc.targetDeployableId = null;
    npc.targetCrateId = null;
    npc.workTimer = 0;
}
