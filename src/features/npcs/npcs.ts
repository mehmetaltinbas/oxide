import { BuildSystem } from 'src/features/building/building';
import { SHORE_GIVE_UP } from 'src/features/npcs/constants/shore-give-up.constant';
import { ItemId } from 'src/features/items/types/item-id.type';
import { NPC_ACTIVE_RADIUS } from 'src/features/npcs/constants/npc-active-radius.constant';
import { NPCS } from 'src/features/npcs/constants/npcs.constant';
import { REPATH_SECONDS } from 'src/features/npcs/constants/repath-seconds.constant';
import { Navigator } from 'src/features/npcs/navigation';
import { NpcHooks } from 'src/features/npcs/types/npc-hooks.interface';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { Npc } from 'src/features/npcs/types/npc.interface';
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

    private nav: Navigator;

    constructor(
        private world: World,
        private build: BuildSystem,
        private hooks: NpcHooks,
    ) {
        this.nav = new Navigator(world, build);
    }

    /** Building went up or came down; the walkable grid is no longer valid. */
    invalidateNavigation(): void {
        this.nav.invalidate();
    }

    clear(): void {
        this.list.length = 0;
    }

    spawn(
        kind: NpcKind,
        x: number,
        y: number,
        opts: { home?: { x: number; y: number }; leash?: number } = {},
    ): Npc {
        const def = NPCS[kind];
        // Armed npcs carry a real gun, so killing one gets you the gun.
        const weapon: ItemId | null = def.gun ? 'rifle' : null;
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
            weapon,
            path: null,
            pathIndex: 0,
            repathIn: randRange(Math.random, 0, REPATH_SECONDS),
            pathGoalX: 0,
            pathGoalY: 0,
        };
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
            );
        }
        const ideal = gun.range * 0.55;
        let speed = 0;
        if (toPlayer > ideal * 1.15) speed = def.speed;
        else if (toPlayer < ideal * 0.6) speed = -def.speed * 0.8;
        if (!clear) speed = def.speed;
        this.move(n, Math.cos(a) * speed, Math.sin(a) * speed, dt);
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

    damage(n: Npc, amount: number, fromX: number, fromY: number, knockback = 0): void {
        n.hp -= amount;
        n.flash = 0.12;
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
        // island.
        if (def.wild) this.hooks.scheduleRegrowth(n.kind);
        if (!def.loot) return;
        this.hooks.dropLoot(n.kind, n.x, n.y, def.loot, n.weapon);
    }
}
