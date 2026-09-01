import { BuildSystem } from 'src/features/building/building';
import { Structure } from 'src/features/building/types/structure.interface';
import { centerOf } from 'src/features/building/utils/center-of.util';
import { CombatHooks } from 'src/features/combat/types/combat-hooks.interface';
import { Faction } from 'src/features/combat/types/faction.type';
import { Projectile } from 'src/features/combat/types/projectile.interface';
import { ThrownExplosive } from 'src/features/combat/types/thrown-explosive.interface';
import { segmentHitsCircle } from 'src/features/combat/utils/segment-hits-circle.util';
import { ItemId } from 'src/features/items/types/item-id.type';
import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';
import { World } from 'src/features/world/world';
import { dist } from 'src/shared/utils/dist.util';

export class Combat {
    shots: Projectile[] = [];
    thrown: ThrownExplosive[] = [];
    private nextId = 1;

    constructor(
        private world: World,
        private build: BuildSystem,
        private hooks: CombatHooks,
    ) {}

    clear(): void {
        this.shots.length = 0;
        this.thrown.length = 0;
    }

    fire(
        x: number,
        y: number,
        angle: number,
        damage: number,
        speed: number,
        range: number,
        faction: Faction,
        color = '#ffe9a8',
        shooterClan = 0,
    ): void {
        this.shots.push({
            x,
            y,
            vx: Math.cos(angle) * speed,
            vy: Math.sin(angle) * speed,
            life: range / speed,
            damage,
            faction,
            shooterClan,
            color,
            length: faction === 'player' ? 14 : 12,
        });
    }

    throwExplosive(
        item: ItemId,
        x: number,
        y: number,
        angle: number,
        power: number,
        damage: number,
        radius: number,
        fuse: number,
        owner: Faction,
    ): ThrownExplosive {
        const t: ThrownExplosive = {
            id: this.nextId++,
            item,
            x,
            y,
            vx: Math.cos(angle) * power,
            vy: Math.sin(angle) * power,
            fuse,
            damage,
            radius,
            stuckTo: null,
            owner,
        };
        this.thrown.push(t);
        return t;
    }

    /** Stick a charge straight onto a piece, the way a raider places one. */
    attachExplosive(
        item: ItemId,
        s: Structure,
        damage: number,
        radius: number,
        fuse: number,
        owner: Faction,
    ): ThrownExplosive {
        const c = centerOf(s);
        const t: ThrownExplosive = {
            id: this.nextId++,
            item,
            x: c.x,
            y: c.y,
            vx: 0,
            vy: 0,
            fuse,
            damage,
            radius,
            stuckTo: s.id,
            owner,
        };
        this.thrown.push(t);
        return t;
    }

    update(dt: number): void {
        this.updateShots(dt);
        this.updateThrown(dt);
    }

    private updateShots(dt: number): void {
        const npcs = this.hooks.npcs();
        const player = this.hooks.playerPos();

        for (let i = this.shots.length - 1; i >= 0; i--) {
            const p = this.shots[i];
            p.life -= dt;
            const px = p.x;
            const py = p.y;
            p.x += p.vx * dt;
            p.y += p.vy * dt;

            if (p.life <= 0 || p.x < 0 || p.y < 0 || p.x > WORLD_W || p.y > WORLD_H) {
                this.shots.splice(i, 1);
                continue;
            }

            // Walls and tree trunks stop bullets, so cover is real for both sides.
            if (this.build.blocksLine(px, py, p.x, p.y)) {
                this.shots.splice(i, 1);
                continue;
            }
            let blocked = false;
            for (const n of this.world.nodesNear(p.x, p.y, 30)) {
                if (n.hp <= 0 || n.kind === 'hemp') continue;
                if (dist(p.x, p.y, n.x, n.y) < n.radius * 0.5) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                this.shots.splice(i, 1);
                continue;
            }

            // A shot hits whatever it meets that is not on the shooter's side. A
            // clansman's bullet can therefore kill a boar, and friendly fire between
            // clanmates is impossible.
            let consumed = false;
            for (const n of npcs) {
                if (p.faction !== 'player' && n.clan === p.shooterClan && n.clan !== 0) continue;
                if (p.faction !== 'player' && p.shooterClan === 0 && n.clan === 0) continue;
                if (!segmentHitsCircle(px, py, p.x, p.y, n.x, n.y, n.radius)) continue;
                this.hooks.hitNpc(n, p.damage, px, py, p.shooterClan);
                consumed = true;
                break;
            }
            if (
                !consumed &&
                p.faction !== 'player' &&
                player.alive &&
                segmentHitsCircle(px, py, p.x, p.y, player.x, player.y, player.radius)
            ) {
                this.hooks.hitPlayer(p.damage, px, py);
                consumed = true;
            }
            if (consumed) this.shots.splice(i, 1);
        }
    }

    private updateThrown(dt: number): void {
        for (let i = this.thrown.length - 1; i >= 0; i--) {
            const t = this.thrown[i];
            t.fuse -= dt;
            if (t.stuckTo === null) {
                t.x += t.vx * dt;
                t.y += t.vy * dt;
                t.vx *= 1 - 2.6 * dt;
                t.vy *= 1 - 2.6 * dt;
                // Once it slows down it has landed; a charge that hits a wall sticks.
                if (Math.hypot(t.vx, t.vy) > 30) {
                    const near = this.build.inBlast(t.x, t.y, 26);
                    const wall = near.find(
                        (s) => 'side' in s && (s as Structure).side !== undefined,
                    ) as Structure | undefined;
                    if (wall) {
                        t.stuckTo = wall.id;
                        const c = centerOf(wall);
                        t.x = c.x;
                        t.y = c.y;
                        t.vx = 0;
                        t.vy = 0;
                    }
                }
            }
            if (t.fuse <= 0) {
                this.hooks.explosion(t.x, t.y, t.radius, t.damage, t.owner, t.stuckTo ?? null);
                this.thrown.splice(i, 1);
            }
        }
    }
}
