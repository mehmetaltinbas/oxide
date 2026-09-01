import { CLAN_SKILLS } from 'src/features/clans/constants/clan-skills.constant';
import { ClanState } from 'src/features/clans/types/clan-state.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { addItem } from 'src/features/items/utils/add-item.util';
import { countIn } from 'src/features/items/utils/count-in.util';
import { removeItem } from 'src/features/items/utils/remove-item.util';
import { NPCS } from 'src/features/npcs/constants/npcs.constant';
import { SURVIVOR } from 'src/features/npcs/constants/survivor.constant';
import { Npc } from 'src/features/npcs/types/npc.interface';
import { SurvivorGoal } from 'src/features/npcs/types/survivor-goal.type';
import { SurvivorWorld } from 'src/features/npcs/types/survivor-world.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { TAU } from 'src/shared/constants/tau.constant';
import { approachAngle } from 'src/shared/utils/approach-angle.util';

const GATHER_KINDS: Record<string, ResourceNode['kind'][]> = {
    wood: ['tree'],
    stone: ['stone_node'],
    metal_ore: ['metal_node'],
    sulfur_ore: ['sulfur_node'],
    cloth: ['hemp'],
};

export class SurvivorBrain {
    constructor(private w: SurvivorWorld) {}

    /** One survivor's tick. Returns false when this npc is not clan-run. */
    update(npc: Npc, dt: number, isRaiding: boolean): boolean {
        const clan = this.w.clanOf(npc.clan);
        if (!clan || clan.wiped) return false;

        const skill = CLAN_SKILLS[clan.skill];
        this.tickNeeds(npc, dt);

        // Watchdog: a survivor that has not moved while pursuing a goal gives up
        // on it rather than grinding into a wall forever.
        const moved = Math.hypot(npc.x - npc.lastX, npc.y - npc.lastY);
        npc.lastX = npc.x;
        npc.lastY = npc.y;
        const shouldBeMoving = npc.goal !== 'rest' && npc.goal !== 'idle' && npc.workTimer <= 0;
        if (shouldBeMoving && moved < 0.35 * dt * 60) {
            npc.stuckTimer += dt;
            if (npc.stuckTimer > SURVIVOR.stuckTimeout) {
                npc.stuckTimer = 0;
                if (npc.goal === 'hunt') npc.huntCooldown = 25;
                this.abandon(npc);
            }
        } else {
            npc.stuckTimer = Math.max(0, npc.stuckTimer - dt * 2);
        }

        npc.goalAge += dt;
        npc.blacklistUntil = Math.max(0, npc.blacklistUntil - dt);
        npc.huntCooldown = Math.max(0, npc.huntCooldown - dt);
        npc.lootCooldown = Math.max(0, npc.lootCooldown - dt);

        // Hard timeout. The movement watchdog above cannot see a survivor that is
        // busily circling something it will never reach, so every goal also has a
        // wall-clock limit after which it is abandoned.
        if (npc.goalAge > 26 && npc.goal !== 'rest' && npc.goal !== 'idle') {
            if (npc.goal === 'hunt') npc.huntCooldown = 40;
            if (npc.goal === 'loot') npc.lootCooldown = 60;
            this.abandon(npc);
        }

        npc.planTimer -= dt;
        npc.commit -= dt;
        if (npc.planTimer <= 0 || npc.goal === 'idle') {
            npc.planTimer = SURVIVOR.replanInterval / Math.max(0.6, skill.techPerDay);
            if (npc.commit <= 0) this.chooseGoal(npc, clan, isRaiding);
        }

        switch (npc.goal) {
            case 'gather':
                this.doGather(npc, clan, dt);
                break;
            case 'haul':
                this.doHaul(npc, clan, dt);
                break;
            case 'hunt':
                this.doHunt(npc, dt);
                break;
            case 'cook':
                this.doWorkstation(npc, clan, dt, 'campfire');
                break;
            case 'smelt':
                this.doWorkstation(npc, clan, dt, 'furnace');
                break;
            case 'eat':
                this.doEat(npc, clan, dt);
                break;
            case 'build':
                this.doBuild(npc, clan, dt);
                break;
            case 'expand':
                this.doExpand(npc, clan, dt);
                break;
            case 'loot':
                this.doLoot(npc, clan, dt);
                break;
            case 'rest':
                this.doRest(npc, clan, dt);
                break;
            case 'defend':
                this.doDefend(npc, clan, dt);
                break;
            default:
                this.doRest(npc, clan, dt);
                break;
        }
        return true;
    }

    // ------------------------------------------------------------------ needs

    private tickNeeds(npc: Npc, dt: number): void {
        npc.hunger = Math.min(100, npc.hunger + SURVIVOR.hungerRate * dt);
        const nightMul = this.w.isNight() ? SURVIVOR.fatigueNightMultiplier : 1;
        npc.fatigue = Math.min(100, npc.fatigue + SURVIVOR.fatigueRate * nightMul * dt);
        if (npc.hunger >= SURVIVOR.starvingAt) npc.hp -= 0.35 * dt;
    }

    private abandon(npc: Npc): void {
        npc.targetNodeId = null;
        npc.targetNpcId = null;
        npc.targetCrateId = null;
        npc.goal = 'idle';
        npc.commit = 0;
        npc.goalAge = 0;
        npc.blacklistUntil = 6;
        npc.stuckTimer = 0;
    }

    private carryTotal(npc: Npc): number {
        let n = 0;
        for (const v of Object.values(npc.carry)) n += v ?? 0;
        return n;
    }

    // ------------------------------------------------------------------ plan

    /**
     * Score every goal and take the best. Scores are deliberately coarse: the
     * point is a believable ordering of priorities, not a fine-grained optimum.
     */
    private chooseGoal(npc: Npc, clan: ClanState, isRaiding: boolean): void {
        const skill = CLAN_SKILLS[clan.skill];
        const stock = this.w.clanStock(clan.index + 1);
        const scores: [SurvivorGoal, number][] = [];

        // Someone who has deliberately broken off does not get sent straight back
        // in by the defend goal. They go home, patch up, and rejoin later.
        const stockCooked = stock ? countIn(stock, 'meat_cooked') : 0;
        const cookedOnHand = (npc.carry.meat_cooked ?? 0) > 0 || stockCooked > 0;
        const rawInHand = (npc.carry.meat_raw ?? 0) > 0;

        const threat = this.w.threatNear(clan.x, clan.y, clan.index + 1, 420);
        const fighting = (threat !== null || npc.alert > 0) && npc.disengage <= 0;
        if (fighting) scores.push(['defend', 2.2]);

        /*
         * Someone in a fight does not wander off after a boar. Only the goals that
         * a person would genuinely interrupt a fight for stay on the table:
         * eating when starving, and falling back when hurt. Everything else waits.
         *
         * Without this, a whole party chasing the player would spot an animal and
         * all switch to hunting it at once.
         */
        if (fighting) {
            if (npc.hunger > 80 && (cookedOnHand || rawInHand)) scores.push(['eat', 1.4]);
            scores.sort((a, b) => b[1] - a[1]);
            this.commitTo(npc, scores[0][0]);
            return;
        }
        if (isRaiding && npc.kind === 'raider') scores.push(['raid', 2.6]);

        // Eating beats almost everything once it is pressing.
        if (npc.hunger > 45 && cookedOnHand) scores.push(['eat', 0.8 + (npc.hunger / 100) * 1.4]);
        // Only when genuinely desperate do they eat it raw and take their chances.
        else if (npc.hunger > 90 && rawInHand) scores.push(['eat', 2.0]);

        // Raw meat plus a fire is worth a trip.
        const rawOnHand =
            (npc.carry.meat_raw ?? 0) > 0 || (stock ? countIn(stock, 'meat_raw') : 0) > 0;
        if (rawOnHand && this.w.deployableOf(clan.index + 1, 'campfire')) {
            // Cooking outranks working once they are properly hungry, so a camp with
            // a fire actually uses it instead of chewing everything raw.
            scores.push(['cook', npc.hunger > 55 ? 1.6 : npc.hunger > 30 ? 0.9 : 0.6]);
        }

        // Coming off a fight badly hurt, getting back behind the walls beats work.
        if (npc.hp < npc.maxHp * 0.5) scores.push(['rest', 1.25 + (1 - npc.hp / npc.maxHp)]);
        if (this.w.isNight() && npc.fatigue > 40) scores.push(['rest', 0.9 + npc.fatigue / 120]);
        else if (npc.fatigue > SURVIVOR.tiredAt) scores.push(['rest', 0.85]);

        // Hunt when the larder is thin, but only if there is actually something
        // to hunt, otherwise they wander after nothing instead of working.
        const food =
            (stock ? countIn(stock, 'meat_raw') + countIn(stock, 'meat_cooked') : 0) +
            (npc.carry.meat_raw ?? 0) +
            (npc.carry.meat_cooked ?? 0);
        if (
            food < SURVIVOR.targetFood &&
            npc.huntCooldown <= 0 &&
            this.w.nearestAnimal(npc.x, npc.y, SURVIVOR.huntRadius)
        ) {
            // Scales with how bare the larder is, so an empty one outranks fetching
            // more wood while a nearly-full one does not.
            const need = 1 - food / SURVIVOR.targetFood;
            scores.push(['hunt', 0.6 + need * 0.55 + (npc.kind === 'raider' ? 0.1 : 0)]);
        }

        // Ore sitting in the box is wasted until it is fragments.
        const ore = stock ? countIn(stock, 'metal_ore') + countIn(stock, 'sulfur_ore') : 0;
        if (ore >= 10 && this.w.deployableOf(clan.index + 1, 'furnace'))
            scores.push(['smelt', 0.8]);

        // Spend the bank on walls when they can afford it.
        if (this.w.upgradeCandidate(clan)) scores.push(['build', 0.72 + skill.techPerDay * 0.1]);

        // Only the better outfits make the trip to a monument, and only when they
        // have room to carry what they find.
        if (
            (clan.skill === 'veteran' || clan.skill === 'expert') &&
            npc.lootCooldown <= 0 &&
            this.carryTotal(npc) < SURVIVOR.carryCapacity * 0.5 &&
            this.w.nearestCrate(npc.x, npc.y, SURVIVOR.lootRadius)
        ) {
            scores.push(['loot', 0.7]);
        }

        // With the bank full and the walls already up, they put up more building.
        if (this.w.expansionSite(clan)) scores.push(['expand', 0.74]);

        // Full pockets mean a trip home regardless of anything else.
        if (this.carryTotal(npc) >= SURVIVOR.carryCapacity) scores.push(['haul', 1.5]);

        scores.push(['gather', 0.66]);

        scores.sort((a, b) => b[1] - a[1]);
        this.commitTo(npc, scores[0][0]);
    }

    private commitTo(npc: Npc, next: SurvivorGoal): void {
        if (next === npc.goal) return;
        npc.goal = next;
        npc.goalAge = 0;
        npc.commit = SURVIVOR.commitTime;
        npc.targetNodeId = null;
        npc.targetNpcId = null;
        npc.targetCrateId = null;
        npc.workTimer = 0;
    }

    /** Which resource this clan is shortest on right now. */
    private scarcest(clan: ClanState): ItemId {
        const stock = this.w.clanStock(clan.index + 1);
        const have = (id: ItemId): number => (stock ? countIn(stock, id) : 0);
        const gaps: [ItemId, number][] = [
            ['wood', SURVIVOR.targetWood - have('wood')],
            ['stone', SURVIVOR.targetStone - have('stone')],
            ['metal_ore', (SURVIVOR.targetMetalOre - have('metal_ore')) * 2.2],
            ['sulfur_ore', (SURVIVOR.targetMetalOre - have('sulfur_ore')) * 1.6],
            ['cloth', 120 - have('cloth')],
        ];
        gaps.sort((a, b) => b[1] - a[1]);
        return gaps[0][1] > 0 ? gaps[0][0] : 'wood';
    }

    // -------------------------------------------------------------- behaviours

    private doGather(npc: Npc, clan: ClanState, dt: number): void {
        if (this.carryTotal(npc) >= SURVIVOR.carryCapacity) {
            npc.goal = 'haul';
            return;
        }
        let node = npc.targetNodeId !== null ? this.w.nodeById(npc.targetNodeId) : null;
        if (!node || node.hp <= 0) {
            const want = this.scarcest(clan);
            node = this.w.nearestNode(
                npc.x,
                npc.y,
                GATHER_KINDS[want] ?? ['tree'],
                SURVIVOR.workRadius,
            );
            npc.targetNodeId = node ? node.id : null;
        }
        if (!node) {
            npc.goal = 'idle';
            return;
        }
        npc.state = 'gather';
        const left = this.w.step(npc, node.x, node.y, dt);
        if (left > node.radius + SURVIVOR.reach) return;

        npc.workTimer -= dt;
        if (npc.workTimer > 0) return;
        npc.workTimer = SURVIVOR.gatherInterval;
        const got = this.w.damageNode(node, SURVIVOR.gatherDamage);
        for (const [id, n] of Object.entries(got) as [ItemId, number][]) {
            npc.carry[id] = (npc.carry[id] ?? 0) + n;
        }
    }

    /** Bank what has been gathered into the clan box. */
    private doHaul(npc: Npc, clan: ClanState, dt: number): void {
        const box = this.w.deployableOf(clan.index + 1, 'wooden_box');
        const target = box ?? { x: clan.x, y: clan.y };
        npc.state = 'return';
        const left = this.w.step(npc, target.x, target.y, dt);
        if (left > SURVIVOR.reach + 20) return;

        const stock = this.w.clanStock(clan.index + 1);
        if (stock) {
            for (const [id, n] of Object.entries(npc.carry) as [ItemId, number][]) {
                if (!n) continue;
                // Keep a couple of meals on their person and bank the surplus. Banking
                // food matters: the clan's larder level is what stops everyone hunting
                // forever instead of doing any actual work.
                const keep = id === 'meat_raw' || id === 'meat_cooked' ? 2 : 0;
                const give = n - keep;
                if (give <= 0) continue;
                const leftOver = addItem(stock, id, give);
                npc.carry[id] = keep + leftOver;
            }
        }
        npc.goal = 'idle';
        npc.commit = 0;
    }

    private doHunt(npc: Npc, dt: number): void {
        let prey = npc.targetNpcId !== null ? this.w.npcById(npc.targetNpcId) : null;
        if (!prey || prey.hp <= 0) {
            prey = this.w.nearestAnimal(npc.x, npc.y, SURVIVOR.huntRadius);
            npc.targetNpcId = prey ? prey.id : null;
        }
        if (!prey) {
            npc.goal = 'idle';
            return;
        }
        npc.state = 'chase';
        const def = NPCS[npc.kind];
        const gap = Math.hypot(prey.x - npc.x, prey.y - npc.y);

        /*
         * Someone with a gun stops at a comfortable firing distance instead of
         * walking onto the animal. Standing on top of the target meant the round
         * spawned past it and the sweep tested backwards, so point-blank shots
         * simply never connected.
         */
        if (def.gun) {
            const standoff = Math.min(def.gun.range * 0.5, 220);
            if (gap > standoff) {
                this.w.step(npc, prey.x, prey.y, dt);
                return;
            }
            // Too close to shoot cleanly: give ground, then fire.
            if (gap < 70) this.w.step(npc, npc.x * 2 - prey.x, npc.y * 2 - prey.y, dt, 0.8);
        } else {
            // Melee closes all the way in and keeps swinging.
            this.w.step(npc, prey.x, prey.y, dt);
            if (gap > def.attackRange + npc.radius) return;
        }
        npc.facing = approachAngle(npc.facing, Math.atan2(prey.y - npc.y, prey.x - npc.x), 8 * dt);
        this.w.attackAnimal(npc, prey);
    }

    /**
     * Cook or smelt. Same shape either way: carry the input to the station, load
     * it with fuel, light it, and come back for the output.
     */
    private doWorkstation(
        npc: Npc,
        clan: ClanState,
        dt: number,
        kind: 'campfire' | 'furnace',
    ): void {
        const station = this.w.deployableOf(clan.index + 1, kind);
        if (!station || !station.container) {
            npc.goal = 'idle';
            return;
        }
        npc.state = 'gather';
        const left = this.w.step(npc, station.x, station.y, dt);
        if (left > SURVIVOR.reach + 16) return;

        const stock = this.w.clanStock(clan.index + 1);
        const input: ItemId[] = kind === 'campfire' ? ['meat_raw'] : ['metal_ore', 'sulfur_ore'];
        const output: ItemId = kind === 'campfire' ? 'meat_cooked' : 'metal';

        // Load whatever they are carrying, then top up from the clan box.
        for (const id of input) {
            const carried = npc.carry[id] ?? 0;
            if (carried > 0) {
                npc.carry[id] = addItem(station.container, id, carried);
            }
            if (stock) {
                const banked = Math.min(20, countIn(stock, id));
                if (banked > 0) {
                    const moved = banked - addItem(station.container, id, banked);
                    removeItem(stock, id, moved);
                }
            }
        }
        // Fuel it.
        if (countIn(station.container, 'wood') < 10) {
            const fromCarry = Math.min(30, npc.carry.wood ?? 0);
            if (fromCarry > 0) {
                npc.carry.wood =
                    (npc.carry.wood ?? 0) -
                    fromCarry +
                    addItem(station.container, 'wood', fromCarry);
            } else if (stock) {
                const banked = Math.min(30, countIn(stock, 'wood'));
                if (banked > 0) {
                    const moved = banked - addItem(station.container, 'wood', banked);
                    removeItem(stock, 'wood', moved);
                }
            }
        }
        if (!station.lit && countIn(station.container, 'wood') > 0) {
            station.lit = true;
            this.w.say(npc, kind === 'campfire' ? 'lit the fire' : 'fired the furnace', '#ffb347');
        }

        // Collect anything ready.
        const ready = countIn(station.container, output);
        if (ready > 0) {
            const taken = removeItem(station.container, output, ready);
            if (output === 'meat_cooked')
                npc.carry.meat_cooked = (npc.carry.meat_cooked ?? 0) + taken;
            else if (stock) addItem(stock, output, taken);
            npc.goal = 'idle';
            npc.commit = 0;
            return;
        }
        // Otherwise wait by the fire, which is exactly what a player does.
        npc.workTimer -= dt;
        if (npc.workTimer <= 0) {
            npc.workTimer = 3;
            if (station.container.slots.every((sl) => sl === null)) {
                npc.goal = 'idle';
                npc.commit = 0;
            }
        }
    }

    private doEat(npc: Npc, clan: ClanState, dt: number): void {
        if ((npc.carry.meat_cooked ?? 0) <= 0 && npc.hunger > 90 && (npc.carry.meat_raw ?? 0) > 0) {
            npc.carry.meat_raw = (npc.carry.meat_raw ?? 0) - 1;
            npc.hunger = Math.max(0, npc.hunger - SURVIVOR.eatValue * 0.6);
            npc.hp = Math.max(1, npc.hp - 4);
            this.w.say(npc, 'ate it raw', '#c9a06a');
            npc.goal = 'idle';
            npc.commit = 0;
            return;
        }
        if ((npc.carry.meat_cooked ?? 0) > 0) {
            npc.carry.meat_cooked = (npc.carry.meat_cooked ?? 0) - 1;
            npc.hunger = Math.max(0, npc.hunger - SURVIVOR.eatValue);
            npc.hp = Math.min(npc.maxHp, npc.hp + 8);
            this.w.say(npc, 'ate', '#8cf08c');
            npc.goal = 'idle';
            npc.commit = 0;
            return;
        }
        const box = this.w.deployableOf(clan.index + 1, 'wooden_box');
        if (!box) {
            npc.goal = 'idle';
            return;
        }
        npc.state = 'return';
        const left = this.w.step(npc, box.x, box.y, dt);
        if (left > SURVIVOR.reach + 20) return;
        const stock = this.w.clanStock(clan.index + 1);
        if (stock && removeItem(stock, 'meat_cooked', 1) > 0) {
            npc.carry.meat_cooked = (npc.carry.meat_cooked ?? 0) + 1;
        } else {
            npc.goal = 'idle';
            npc.commit = 0;
        }
    }

    private doBuild(npc: Npc, clan: ClanState, dt: number): void {
        const candidate = this.w.upgradeCandidate(clan);
        if (!candidate) {
            npc.goal = 'idle';
            return;
        }
        npc.state = 'gather';
        const c = candidate.piece;
        const CELL = 64;
        const cx =
            c.kind === 'foundation'
                ? c.gx * CELL + CELL / 2
                : c.side === 'n'
                  ? c.gx * CELL + CELL / 2
                  : c.gx * CELL;
        const cy =
            c.kind === 'foundation'
                ? c.gy * CELL + CELL / 2
                : c.side === 'n'
                  ? c.gy * CELL
                  : c.gy * CELL + CELL / 2;
        const left = this.w.step(npc, cx, cy, dt);
        if (left > SURVIVOR.reach + 24) return;
        npc.workTimer -= dt;
        if (npc.workTimer > 0) return;
        npc.workTimer = 2.5;
        this.w.applyUpgrade(clan, candidate.piece, candidate.cost);
        this.w.say(npc, 'upgraded a wall', '#c9e08a');
        npc.goal = 'idle';
        npc.commit = 0;
    }

    /** Move to whatever set the camp off; the combat path takes over on sight. */
    private doDefend(npc: Npc, clan: ClanState, dt: number): void {
        if (npc.disengage > 0) {
            npc.goal = 'rest';
            npc.commit = 0;
            return;
        }
        const threat =
            this.w.threatNear(npc.x, npc.y, clan.index + 1, 900) ??
            this.w.threatNear(clan.x, clan.y, clan.index + 1, 700);
        if (!threat) {
            npc.goal = 'idle';
            npc.commit = 0;
            return;
        }
        npc.state = 'chase';
        this.w.step(npc, threat.x, threat.y, dt);
    }

    /** Walk to a monument crate, empty it, and carry the contents home. */
    private doLoot(npc: Npc, clan: ClanState, dt: number): void {
        const crate = this.w.nearestCrate(npc.x, npc.y, SURVIVOR.lootRadius);
        if (!crate) {
            npc.goal = 'idle';
            npc.commit = 0;
            return;
        }
        npc.state = 'gather';
        const left = this.w.step(npc, crate.x, crate.y, dt);
        if (left > SURVIVOR.reach + 16) return;

        const taken = this.w.lootCrate(crate);
        let got = 0;
        for (const [id, n] of Object.entries(taken) as [ItemId, number][]) {
            npc.carry[id] = (npc.carry[id] ?? 0) + n;
            got += n;
        }
        if (got > 0) this.w.say(npc, 'looted a crate', '#e8c87a');
        // A monument run is a trip, not a treadmill, go and do some work now.
        npc.lootCooldown = SURVIVOR.lootCooldownSeconds;
        // Straight home with it, the way anyone would.
        npc.goal = 'haul';
        npc.goalAge = 0;
        npc.commit = SURVIVOR.commitTime;
        void clan;
    }

    /** Put up another piece of compound, not just upgrade what is there. */
    private doExpand(npc: Npc, clan: ClanState, dt: number): void {
        const site = this.w.expansionSite(clan);
        if (!site) {
            npc.goal = 'idle';
            npc.commit = 0;
            return;
        }
        const CELL = 64;
        const cx = site.gx * CELL + CELL / 2;
        const cy = site.gy * CELL + CELL / 2;
        npc.state = 'gather';
        const left = this.w.step(npc, cx, cy, dt);
        if (left > SURVIVOR.reach + 24) return;
        npc.workTimer -= dt;
        if (npc.workTimer > 0) return;
        npc.workTimer = 3;
        this.w.applyExpansion(clan, site.gx, site.gy, site.cost);
        this.w.say(npc, 'put up a wall', '#c9e08a');
        npc.goal = 'idle';
        npc.commit = 0;
    }

    private doRest(npc: Npc, clan: ClanState, dt: number): void {
        npc.state = 'idle';
        const left = this.w.step(npc, clan.x, clan.y, dt, 0.7);
        if (left > 90) return;
        npc.fatigue = Math.max(0, npc.fatigue - SURVIVOR.restRecovery * dt);
        npc.hp = Math.min(npc.maxHp, npc.hp + 1.2 * dt);
        // Shuffle about a bit so a resting camp is not a row of statues.
        if (Math.random() < dt * 0.35) {
            npc.facing = Math.random() * TAU;
        }
        if (npc.fatigue <= 5 && !this.w.isNight()) {
            npc.goal = 'idle';
            npc.commit = 0;
        }
    }
}
