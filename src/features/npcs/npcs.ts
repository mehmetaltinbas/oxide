import { BuildSystem } from 'src/features/building/building';
import { CELL } from 'src/features/building/constants/cell.constant';
import { Structure } from 'src/features/building/types/structure.interface';
import { CLAN_PURSUIT_LEASH } from 'src/features/clans/constants/clan-pursuit-leash.constant';
import { SHORE_GIVE_UP } from 'src/features/npcs/constants/shore-give-up.constant';
import { CLAN_PURSUIT_SECONDS } from 'src/features/clans/constants/clan-pursuit-seconds.constant';
import { CLAN_RETREAT_HEALTH } from 'src/features/clans/constants/clan-retreat-health.constant';
import { ItemId } from 'src/features/items/types/item-id.type';
import { NPC_ACTIVE_RADIUS } from 'src/features/npcs/constants/npc-active-radius.constant';
import { NPCS } from 'src/features/npcs/constants/npcs.constant';
import { REPATH_SECONDS } from 'src/features/npcs/constants/repath-seconds.constant';
import { SURVIVOR } from 'src/features/npcs/constants/survivor.constant';
import { Navigator } from 'src/features/npcs/navigation';
import { SurvivorBrain } from 'src/features/npcs/survivor';
import { NpcHooks } from 'src/features/npcs/types/npc-hooks.interface';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { Npc } from 'src/features/npcs/types/npc.interface';
import { SurvivorWorld } from 'src/features/npcs/types/survivor-world.interface';
import { initSurvivor } from 'src/features/npcs/utils/init-survivor.util';
import { isSurvivorKind } from 'src/features/npcs/utils/is-survivor-kind.util';
import { PlayerState } from 'src/features/survival/types/player-state.interface';
import { World } from 'src/features/world/world';
import { TAU } from 'src/shared/constants/tau.constant';
import { approachAngle } from 'src/shared/utils/approach-angle.util';
import { clamp } from 'src/shared/utils/clamp.util';
import { dist } from 'src/shared/utils/dist.util';
import { randRange } from 'src/shared/utils/rand-range.util';

export class NpcSystem {
    list: Npc[] = [];
    private nextId = 1;

    private brain: SurvivorBrain | null = null;
    private nav: Navigator;

    constructor(
        private world: World,
        private build: BuildSystem,
        private hooks: NpcHooks,
    ) {
        this.nav = new Navigator(world, build);
    }

    /** Wired after construction, because the brain needs the game around it. */
    attachSurvivorWorld(w: SurvivorWorld): void {
        this.brain = new SurvivorBrain(w);
    }

    /** Building went up or came down; the walkable grid is no longer valid. */
    invalidateNavigation(): void {
        this.nav.invalidate();
    }

    /**
     * One step toward a point, used by the survivor brain. Steers around whatever
     * is in the way instead of pressing into it, which is what used to leave
     * clan members grinding against their own walls until they starved.
     */
    stepToward(n: Npc, x: number, y: number, dt: number, speedScale = 1): number {
        const def = NPCS[n.kind];
        const d = dist(n.x, n.y, x, y);
        if (d <= 6) {
            this.move(n, 0, 0, dt);
            return d;
        }

        const slow = n.fatigue > 70 ? 0.75 : 1;
        const speed = def.speed * speedScale * slow;
        n.repathIn -= dt;

        // Close and unobstructed: just walk. Pathfinding every short hop is waste.
        const direct = !this.pathBlocked(
            n,
            Math.atan2(y - n.y, x - n.x),
            Math.min(d, n.radius + 46),
        );
        if (direct && d < CELL * 1.5) {
            n.path = null;
            this.walk(n, x, y, speed, dt);
            return d;
        }

        // Re-plan when the destination has moved, the route ran out, or the timer
        // came round. Everything else follows the route it already has.
        const goalMoved = dist(x, y, n.pathGoalX, n.pathGoalY) > CELL;
        // A search that found nothing must wait out the cooldown before trying
        // again. Treating "no path" as its own reason to re-plan re-ran A* every
        // frame, and one agent walled in somewhere ate the whole per-frame
        // search budget. An exhausted route still re-plans at once, because
        // that one is about to succeed.
        const exhausted = n.path !== null && n.pathIndex >= n.path.length;
        if (exhausted || goalMoved || n.repathIn <= 0) {
            n.repathIn = REPATH_SECONDS;
            n.pathGoalX = x;
            n.pathGoalY = y;
            n.path = this.nav.findPath(n.x, n.y, x, y, n.radius);
            n.pathIndex = 0;
        }

        if (!n.path || n.path.length === 0) {
            // No route exists. Walk at it anyway; the stuck watchdog will give up.
            this.walk(n, x, y, speed, dt);
            return d;
        }

        let waypoint = n.path[n.pathIndex];
        while (waypoint && dist(n.x, n.y, waypoint.x, waypoint.y) < 22) {
            n.pathIndex++;
            waypoint = n.path[n.pathIndex];
        }
        if (!waypoint) {
            n.path = null;
            this.walk(n, x, y, speed, dt);
            return d;
        }
        this.walk(n, waypoint.x, waypoint.y, speed, dt);
        return d;
    }

    /** Head straight at a point, with a light nudge around anything immediate. */
    private walk(n: Npc, x: number, y: number, speed: number, dt: number): void {
        const a = Math.atan2(y - n.y, x - n.x);
        let heading = a;
        const probe = n.radius + 30;
        if (this.pathBlocked(n, heading, probe)) {
            for (const off of [0.5, -0.5, 1.0, -1.0, 1.6, -1.6]) {
                if (!this.pathBlocked(n, a + off, probe)) {
                    heading = a + off;
                    break;
                }
            }
        }
        n.facing = approachAngle(n.facing, heading, 7 * dt);
        this.move(n, Math.cos(heading) * speed, Math.sin(heading) * speed, dt);
    }

    /** Is there a wall or a solid node just ahead on this heading? */
    private pathBlocked(n: Npc, heading: number, probe: number): boolean {
        const px = n.x + Math.cos(heading) * probe;
        const py = n.y + Math.sin(heading) * probe;
        if (this.build.blocksLine(n.x, n.y, px, py)) return true;
        for (const node of this.world.nodesNear(px, py, 40)) {
            if (node.hp <= 0 || node.kind === 'hemp') continue;
            if (dist(px, py, node.x, node.y) < node.radius * 0.6 + n.radius) return true;
        }
        return false;
    }

    clear(): void {
        this.list.length = 0;
    }

    spawn(
        kind: NpcKind,
        x: number,
        y: number,
        opts: {
            clan?: number;
            home?: { x: number; y: number };
            leash?: number;
            charges?: number;
            tech?: number;
        } = {},
    ): Npc {
        const def = NPCS[kind];
        // Armed npcs carry a real gun, so killing one gets you the gun.
        const weapon: ItemId | null = def.gun
            ? (opts.tech ?? 1) >= 4
                ? 'rifle'
                : 'revolver'
            : null;
        const n: Npc = {
            id: this.nextId++,
            kind,
            x,
            y,
            vx: 0,
            vy: 0,
            facing: Math.random() * TAU,
            hp: def.hp,
            maxHp: def.hp,
            radius: def.radius,
            state: 'wander',
            stateTime: 0,
            attackTimer: randRange(Math.random, 0, 1),
            gunTimer: randRange(Math.random, 0, 1.5),
            flash: 0,
            knockX: 0,
            knockY: 0,
            shoreWait: 0,
            animPhase: Math.random() * TAU,
            homeX: opts.home?.x ?? x,
            homeY: opts.home?.y ?? y,
            leash: opts.leash ?? 380,
            clan: opts.clan ?? 0,
            weapon,
            charges: opts.charges ?? 0,
            targetStructure: null,
            hunger: 0,
            fatigue: 0,
            carry: {},
            goal: 'idle',
            planTimer: 0,
            commit: 0,
            targetNodeId: null,
            targetNpcId: null,
            targetDeployableId: null,
            targetCrateId: null,
            workTimer: 0,
            alert: 0,
            stuckTimer: 0,
            lastX: x,
            lastY: y,
            blacklistUntil: 0,
            lastHitByClan: 0,
            goalAge: 0,
            huntCooldown: 0,
            lootCooldown: 0,
            pursuit: 0,
            disengage: 0,
            path: null,
            pathIndex: 0,
            repathIn: randRange(Math.random, 0, REPATH_SECONDS),
            pathGoalX: 0,
            pathGoalY: 0,
        };
        if (isSurvivorKind(kind)) initSurvivor(n);
        this.list.push(n);
        return n;
    }

    update(dt: number, player: PlayerState): void {
        // On an island this size most of the population is nowhere near the player.
        // Anything outside the active radius is left frozen, which keeps the cost
        // of the simulation flat no matter how big the map gets.
        this.nav.beginFrame();
        this.active.length = 0;
        const r2 = NPC_ACTIVE_RADIUS * NPC_ACTIVE_RADIUS;
        for (let i = this.list.length - 1; i >= 0; i--) {
            const n = this.list[i];
            if (n.hp <= 0) {
                this.kill(n, i);
                continue;
            }
            const dx = n.x - player.x;
            const dy = n.y - player.y;
            if (dx * dx + dy * dy > r2) continue;
            this.active.push(n);
            this.updateOne(n, dt, player);
        }
        this.separate(dt);
    }

    /** Rebuilt each frame: the npcs close enough to matter. */
    private active: Npc[] = [];

    private updateOne(n: Npc, dt: number, player: PlayerState): void {
        const def = NPCS[n.kind];
        n.stateTime += dt;
        n.attackTimer -= dt;
        n.gunTimer -= dt;
        n.animPhase += dt * (2 + Math.hypot(n.vx, n.vy) / 40);
        if (n.flash > 0) n.flash -= dt;
        if (n.alert > 0) n.alert -= dt;
        if (n.disengage > 0) n.disengage -= dt;

        // Survivors of a wiped clan walk off the map rather than loitering on a
        // plot they no longer hold.
        if (n.state === 'flee') {
            const home = dist(n.x, n.y, n.homeX, n.homeY);
            if (home < 70) {
                const i = this.list.indexOf(n);
                if (i >= 0) this.list.splice(i, 1);
                return;
            }
            const a = Math.atan2(n.homeY - n.y, n.homeX - n.x);
            n.facing = approachAngle(n.facing, a, 5 * dt);
            this.move(
                n,
                Math.cos(n.facing) * def.speed * 1.1,
                Math.sin(n.facing) * def.speed * 1.1,
                dt,
            );
            return;
        }

        // Clan members live like survivors: see docs/systems/ai-survivor.md. They only
        // drop into the simple combat behaviours when something is on top of them
        // or the clan has ordered a raid.
        if (this.brain && n.clan > 0 && isSurvivorKind(n.kind)) {
            const raiding = n.kind === 'raider' && this.hooks.raidTarget() !== null;
            const toPlayer = player.alive ? dist(n.x, n.y, player.x, player.y) : Infinity;

            // You are not a neutral party to these people. They react to you near
            // their compound, and anyone who takes a hit tells the neighbours.
            const home = dist(n.x, n.y, n.homeX, n.homeY);
            const nearTheirGround = toPlayer < 340 && home < 520;
            // Someone who has just broken off is not talked back into the fight by
            // the same trespass they were already losing.
            if (nearTheirGround && n.alert <= 0 && n.disengage <= 0) this.raiseAlarm(n, 'trespass');

            const reach = def.gun ? def.gun.range * 0.85 : 260;
            const canSee = toPlayer < reach && !this.build.blocksLine(n.x, n.y, player.x, player.y);

            if (raiding) {
                this.updateRaider(n, dt, player, def);
                return;
            }

            if (n.alert > 0 && player.alive) {
                // These are people, not wolves. They break off a fight they are losing,
                // and they do not follow you across the island out of spite.
                const fromHome = dist(n.x, n.y, n.homeX, n.homeY);
                const hurt = n.hp < n.maxHp * CLAN_RETREAT_HEALTH;
                const overExtended =
                    fromHome > CLAN_PURSUIT_LEASH || n.pursuit > CLAN_PURSUIT_SECONDS;

                if (hurt || overExtended) {
                    n.alert = 0;
                    n.pursuit = 0;
                    n.disengage = hurt ? 18 : 8;
                    n.goal = 'idle';
                    n.commit = 0;
                    n.state = 'return';
                    // Shoot over their shoulder while backing off rather than turning tail.
                    if (def.gun && canSee && n.gunTimer <= 0 && !hurt) {
                        n.gunTimer = def.gun.cooldown * 1.4;
                        const a = Math.atan2(player.y - n.y, player.x - n.x);
                        this.hooks.fire(
                            n.x + Math.cos(a) * (n.radius + 8),
                            n.y + Math.sin(a) * (n.radius + 8),
                            a,
                            def.gun.damage,
                            def.gun.speed,
                            def.gun.range,
                            n.clan,
                        );
                    }
                    this.stepToward(n, n.homeX, n.homeY, dt);
                    return;
                }

                n.pursuit += dt;
                if (canSee) {
                    // Eyes on the target keeps the lock fresh, so a long chase does not
                    // quietly time out and leave them free to wander after wildlife.
                    n.alert = Math.max(n.alert, SURVIVOR.alertSeconds * 0.5);
                    if (def.gun) this.shootAt(n, def, player, dt, toPlayer);
                    else this.meleeChase(n, def, player, dt, toPlayer);
                    return;
                }
                // Lost sight: press toward the last known position for a while, then
                // give it up rather than tailing forever.
                if (toPlayer < 700) {
                    n.state = 'chase';
                    this.stepToward(n, player.x, player.y, dt);
                    return;
                }
                n.alert = Math.min(n.alert, 6);
            } else if (n.disengage > 0 && dist(n.x, n.y, n.homeX, n.homeY) > 90) {
                n.state = 'return';
                this.stepToward(n, n.homeX, n.homeY, dt);
                return;
            } else if (n.pursuit > 0) {
                n.pursuit = Math.max(0, n.pursuit - dt * 2);
            }

            if (this.brain.update(n, dt, raiding)) return;
        }

        if (n.kind === 'raider') {
            this.updateRaider(n, dt, player, def);
            return;
        }

        const toPlayer = player.alive ? dist(n.x, n.y, player.x, player.y) : Infinity;
        const fromHome = dist(n.x, n.y, n.homeX, n.homeY);

        // Wildlife that is not hostile only fights back once it is hurt.
        const provoked = n.state === 'chase' || n.state === 'attack';
        const wantsFight = def.hostile || provoked;
        const aggroRange = def.gun ? def.gun.range * 0.85 : 300;

        // Out in the water you are out of reach, and after a moment an animal
        // stops pretending otherwise: it drops the chase and goes home. It
        // will take you up again only once you are back on land.
        const swimming = player.alive && this.world.biomeAt(player.x, player.y) === 'water';
        if (!def.gun && swimming && provoked) {
            n.shoreWait += dt;
            if (n.shoreWait >= SHORE_GIVE_UP) {
                n.state = 'return';
                n.shoreWait = 0;
            }
        } else if (!swimming) {
            n.shoreWait = 0;
        }
        const givenUp = !def.gun && swimming && !(n.state === 'chase' || n.state === 'attack');

        if (
            wantsFight &&
            !givenUp &&
            player.alive &&
            toPlayer < aggroRange &&
            fromHome < n.leash * 2.4
        ) {
            if (def.gun) {
                this.shootAt(n, def, player, dt, toPlayer);
                return;
            }
            this.meleeChase(n, def, player, dt, toPlayer);
            return;
        }

        // Idle: drift around home.
        if (fromHome > n.leash) {
            const a = Math.atan2(n.homeY - n.y, n.homeX - n.x);
            n.facing = approachAngle(n.facing, a, 4 * dt);
            this.move(
                n,
                Math.cos(n.facing) * def.speed * 0.5,
                Math.sin(n.facing) * def.speed * 0.5,
                dt,
            );
            n.state = 'return';
            return;
        }
        n.state = 'wander';
        if (n.stateTime > randRange(Math.random, 1.5, 4)) {
            n.stateTime = 0;
            n.facing = Math.random() * TAU;
        }
        const drift = Math.random() < 0.6 ? def.speed * 0.3 : 0;
        this.move(n, Math.cos(n.facing) * drift, Math.sin(n.facing) * drift, dt);
    }

    private meleeChase(
        n: Npc,
        def: (typeof NPCS)[NpcKind],
        player: PlayerState,
        dt: number,
        toPlayer: number,
    ): void {
        n.state = toPlayer <= def.attackRange + n.radius ? 'attack' : 'chase';
        const a = Math.atan2(player.y - n.y, player.x - n.x);
        n.facing = approachAngle(n.facing, a, 8 * dt);
        if (n.state === 'attack') {
            if (n.attackTimer <= 0) {
                n.attackTimer = def.attackCooldown;
                this.hooks.damagePlayer(def.damage, n.x, n.y);
            }
            this.move(n, 0, 0, dt);
        } else {
            this.move(n, Math.cos(n.facing) * def.speed, Math.sin(n.facing) * def.speed, dt);
        }
    }

    private shootAt(
        n: Npc,
        def: (typeof NPCS)[NpcKind],
        player: PlayerState,
        dt: number,
        toPlayer: number,
    ): void {
        const gun = def.gun!;
        const a = Math.atan2(player.y - n.y, player.x - n.x);
        n.facing = approachAngle(n.facing, a, 6 * dt);
        n.state = 'chase';

        // Keep a firing distance, and do not shoot through walls.
        const clear = !this.build.blocksLine(n.x, n.y, player.x, player.y);
        if (clear && n.gunTimer <= 0 && toPlayer < gun.range) {
            n.gunTimer = gun.cooldown * randRange(Math.random, 0.85, 1.2);
            const spread = randRange(Math.random, -gun.spread, gun.spread);
            this.hooks.fire(
                n.x + Math.cos(a) * (n.radius + 8),
                n.y + Math.sin(a) * (n.radius + 8),
                a + spread,
                gun.damage,
                gun.speed,
                gun.range,
                n.clan,
            );
        }
        const ideal = gun.range * 0.55;
        let speed = 0;
        if (toPlayer > ideal * 1.15) speed = def.speed;
        else if (toPlayer < ideal * 0.6) speed = -def.speed * 0.8;
        if (!clear) speed = def.speed;
        this.move(n, Math.cos(a) * speed, Math.sin(a) * speed, dt);
    }

    /**
     * Raiders walk to your base, chew or blow a hole in the nearest weak point,
     * and shoot whatever gets in the way.
     */
    private updateRaider(
        n: Npc,
        dt: number,
        player: PlayerState,
        def: (typeof NPCS)[NpcKind],
    ): void {
        const toPlayer = player.alive ? dist(n.x, n.y, player.x, player.y) : Infinity;
        if (toPlayer < 260 && !this.build.blocksLine(n.x, n.y, player.x, player.y)) {
            this.shootAt(n, def, player, dt, toPlayer);
            return;
        }

        const target = this.hooks.raidTarget();
        if (!target) {
            n.state = 'wander';
            this.move(n, 0, 0, dt);
            return;
        }

        const d = dist(n.x, n.y, target.x, target.y);
        if (d > 240) {
            n.state = 'raid';
            const a = Math.atan2(target.y - n.y, target.x - n.x);
            n.facing = approachAngle(n.facing, a, 6 * dt);
            this.move(n, Math.cos(n.facing) * def.speed, Math.sin(n.facing) * def.speed, dt);
            return;
        }

        const weak = this.hooks.weakestStructureNear(n.x, n.y, 300);
        if (!weak) {
            // Nothing left to break: push in on the player.
            n.state = 'raid';
            if (player.alive) {
                const a = Math.atan2(player.y - n.y, player.x - n.x);
                n.facing = approachAngle(n.facing, a, 6 * dt);
                this.move(n, Math.cos(n.facing) * def.speed, Math.sin(n.facing) * def.speed, dt);
            }
            return;
        }

        const c = structureCenter(weak);
        const dd = dist(n.x, n.y, c.x, c.y);
        n.targetStructure = weak.id;
        const a = Math.atan2(c.y - n.y, c.x - n.x);
        n.facing = approachAngle(n.facing, a, 7 * dt);
        if (dd > 44) {
            n.state = 'raid';
            this.move(n, Math.cos(n.facing) * def.speed, Math.sin(n.facing) * def.speed, dt);
            return;
        }
        n.state = 'attack';
        this.move(n, 0, 0, dt);
        if (n.attackTimer > 0) return;
        n.attackTimer = 1.6;
        if (n.charges > 0 && weak.maxHp > 260) {
            // Anything meaningful gets a charge rather than a beating.
            n.charges--;
            this.hooks.plantCharge(n, weak);
        } else {
            this.hooks.attackStructure(weak.id, 28, n.x, n.y);
        }
    }

    private move(n: Npc, vx: number, vy: number, dt: number): void {
        n.vx = vx + n.knockX;
        n.vy = vy + n.knockY;
        n.knockX *= 1 - clamp(9 * dt, 0, 1);
        n.knockY *= 1 - clamp(9 * dt, 0, 1);
        let nx = n.x + n.vx * dt;
        let ny = n.y + n.vy * dt;
        const world = this.world.clampToWorld(nx, ny, n.radius);
        nx = world.x;
        ny = world.y;
        const solid = this.build.resolve(nx, ny, n.radius);
        const ashore = this.keepAshore(n, solid.x, solid.y);
        n.x = ashore.x;
        n.y = ashore.y;
    }

    /**
     * Nothing walks into the sea.
     *
     * Steering here is a straight line at whatever the agent wants, so a goal
     * across a bay walks it in. Stepping along one axis at a time lets it run
     * the shoreline instead, which is what someone who can swim but would
     * rather not actually does. A body already in the water is left free to
     * move, or it would be stuck there for good.
     */
    private keepAshore(n: Npc, x: number, y: number): { x: number; y: number } {
        if (this.world.biomeAt(x, y) !== 'water') return { x, y };
        if (this.world.biomeAt(n.x, n.y) === 'water') return { x, y };
        if (this.world.biomeAt(x, n.y) !== 'water') return { x, y: n.y };
        if (this.world.biomeAt(n.x, y) !== 'water') return { x: n.x, y };
        return { x: n.x, y: n.y };
    }

    private separate(dt: number): void {
        for (let i = 0; i < this.active.length; i++) {
            for (let j = i + 1; j < this.active.length; j++) {
                const a = this.active[i];
                const b = this.active[j];
                const dx = b.x - a.x;
                const dy = b.y - a.y;
                const min = a.radius + b.radius;
                const d2 = dx * dx + dy * dy;
                if (d2 > min * min || d2 < 0.0001) continue;
                const d = Math.sqrt(d2);
                const push = ((min - d) / d) * 0.5 * Math.min(1, dt * 20);
                a.x -= dx * push;
                a.y -= dy * push;
                b.x += dx * push;
                b.y += dy * push;
            }
        }
    }

    /** Provoke a clansman and everyone standing near him. */
    raiseAlarm(n: Npc, reason: 'hit' | 'trespass'): void {
        if (n.clan <= 0) return;
        n.alert = SURVIVOR.alertSeconds;
        n.goal = 'defend';
        n.commit = 0;
        if (reason === 'hit') n.alert += 15;
        for (const other of this.list) {
            if (other === n || other.clan !== n.clan) continue;
            if (dist(other.x, other.y, n.x, n.y) > SURVIVOR.alertRadius) continue;
            other.alert = Math.max(other.alert, SURVIVOR.alertSeconds);
            other.goal = 'defend';
            other.commit = 0;
        }
    }

    damage(n: Npc, amount: number, fromX: number, fromY: number, knockback = 0, byClan = 0): void {
        n.hp -= amount;
        if (byClan > 0) n.lastHitByClan = byClan;
        n.flash = 0.12;
        // Hitting someone's clanmate makes you their problem.
        if (n.clan > 0) {
            n.disengage = 0;
            this.raiseAlarm(n, 'hit');
        }
        const a = Math.atan2(n.y - fromY, n.x - fromX);
        const scale = clamp(60 / (n.radius * 3), 0.25, 1);
        n.knockX += Math.cos(a) * knockback * scale;
        n.knockY += Math.sin(a) * knockback * scale;
        if (n.state === 'wander' || n.state === 'return') n.state = 'chase';
    }

    private kill(n: Npc, index: number): void {
        this.list.splice(index, 1);
        const def = NPCS[n.kind];
        // Wildlife comes back a day later, like every other resource on the
        // island. Survivors do not: a clan's people are replaced by their clan,
        // on the clan's own timer, which is a different thing entirely.
        if (def.wild) this.hooks.scheduleRegrowth(n.kind);
        if (!def.loot) return;

        // A clansman who made the kill carries it home rather than leaving it on
        // the ground, otherwise armed hunters never bring any meat back.
        if (n.lastHitByClan > 0) {
            let taker: Npc | null = null;
            let bestD = 700;
            for (const other of this.list) {
                if (other.clan !== n.lastHitByClan || !isSurvivorKind(other.kind)) continue;
                const d = dist(other.x, other.y, n.x, n.y);
                if (d < bestD) {
                    bestD = d;
                    taker = other;
                }
            }
            if (taker) {
                for (const [id, range] of Object.entries(def.loot) as [
                    ItemId,
                    [number, number],
                ][]) {
                    const amount = Math.round(randRange(Math.random, range[0], range[1]));
                    if (amount > 0) taker.carry[id] = (taker.carry[id] ?? 0) + amount;
                }
                return;
            }
        }
        this.hooks.dropLoot(n.kind, n.x, n.y, def.loot, n.weapon);
    }
}

function structureCenter(s: Structure): { x: number; y: number } {
    if (s.kind === 'foundation') return { x: s.gx * CELL + CELL / 2, y: s.gy * CELL + CELL / 2 };
    const x = s.gx * CELL;
    const y = s.gy * CELL;
    return s.side === 'n' ? { x: x + CELL / 2, y } : { x, y: y + CELL / 2 };
}
