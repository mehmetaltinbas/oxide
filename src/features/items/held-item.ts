import { FIST_FRACTION } from 'src/features/items/constants/fists.constant';
import { regrowthSeconds } from 'src/features/world/utils/regrowth-seconds.util';
import { BuildSystem } from 'src/features/building/building';
import { CELL } from 'src/features/building/constants/cell.constant';
import { TIER_DEFS } from 'src/features/building/constants/tier-defs.constant';
import { BuildKind } from 'src/features/building/types/build-kind.type';
import { BuildTier } from 'src/features/building/types/build-tier.type';
import { DEPLOY_REACH } from 'src/features/items/constants/deploy-reach.constant';
import { DeployableKind } from 'src/features/building/types/deployable-kind.type';
import { EdgeSide } from 'src/features/building/types/edge-side.type';
import { Structure } from 'src/features/building/types/structure.interface';
import { centerOf } from 'src/features/building/utils/center-of.util';
import { edgeCenter } from 'src/features/building/utils/edge-center.util';
import { nearestEdge } from 'src/features/building/utils/nearest-edge.util';
import { Combat } from 'src/features/combat/combat';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { ItemStack } from 'src/features/items/types/item-stack.interface';
import { acquire } from 'src/features/items/utils/acquire.util';
import { countAcross } from 'src/features/items/utils/count-across.util';
import { removeAcross } from 'src/features/items/utils/remove-across.util';
import { NpcSystem } from 'src/features/npcs/npcs';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { SurvivalSystem } from 'src/features/survival/survival';
import { PlayerState } from 'src/features/survival/types/player-state.interface';
import { NODES } from 'src/features/world/constants/nodes.constant';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { World } from 'src/features/world/world';
import { Audio } from 'src/shared/core/audio';
import { Camera } from 'src/shared/core/camera';
import { Input } from 'src/shared/core/input';
import { Particles } from 'src/shared/core/particles';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { clamp } from 'src/shared/utils/clamp.util';
import { dist } from 'src/shared/utils/dist.util';
import { inCone } from 'src/shared/utils/in-cone.util';
import { randRange } from 'src/shared/utils/rand-range.util';

/**
 * What happens when you use what you are holding: swinging it, firing it,
 * gathering with it, hammering with it, throwing it, or putting it down.
 *
 * `primaryUse` is the one entry point. It reads the held item and dispatches;
 * everything below it is one way of using one kind of thing. See
 * docs/architecture/project-structure.md.
 */
export interface HeldItemHooks {
    player(): PlayerState;
    containers(): Container[];
    heldItem(): ItemStack | null;
    /** Running keeps your hands busy. */
    sprinting(): boolean;
    /** True on the first simulation step of a frame, so a click fires once. */
    edgeStep(): boolean;
    /** Which piece the building plan is set to. */
    buildKind(): BuildKind;
    input(): Input;
    /** Creative mode: everything is free. */
    sandbox(): boolean;
    /** Consumables and magazines are the survival system's business. */
    survival(): SurvivalSystem;
    damageBuilt(id: number, dmg: number, fx: number, fy: number, melee: boolean): void;
    notify(text: string): void;
    /**
     * Tell the server what was just put down. Placement is optimistic: the
     * piece is already on the local map by the time this is called, and the
     * server answers by confirming it or taking it back.
     */
    reportBuild(kind: string, gx: number, gy: number, side?: 'n' | 'w'): void;
}

export class HeldItemSystem {
    constructor(
        private world: World,
        private build: BuildSystem,
        private combat: Combat,
        private npcs: NpcSystem,
        private particles: Particles,
        private audio: Audio,
        private camera: Camera,
        private hooks: HeldItemHooks,
    ) {}

    primaryUse(mx: number, my: number): void {
        const p = this.hooks.player();
        const held = this.hooks.heldItem();

        // Running keeps your hands busy. Medical kit is the exception, you can
        // always patch yourself up on the move.
        const medical = held ? ITEMS[held.id].category === 'consumable' : false;
        if (this.hooks.sprinting() && !medical) {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked)
                this.hooks.notify('You cannot use that while running.');
            return;
        }

        if (!held) {
            // Bare hands: half a rock, in damage and in what they bring back.
            const rock = ITEMS.rock.melee;
            if (rock) {
                this.melee(
                    rock.damage * FIST_FRACTION,
                    rock.gather * FIST_FRACTION,
                    rock.cooldown,
                    rock.reach,
                );
            }
            return;
        }
        const def = ITEMS[held.id];

        if (held.id === 'building_plan') {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked)
                this.placeBuilding(mx, my);
            return;
        }
        if (held.id === 'hammer') {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked) this.hammerUse(mx, my);
            else p.attackTimer = 0.2;
            return;
        }
        if (def.category === 'deployable') {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked) this.deployHeld(held.id);
            return;
        }
        if (def.category === 'explosive') {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked)
                this.throwHeld(held.id, mx, my);
            return;
        }
        if (def.category === 'consumable') {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked)
                this.hooks.survival().consume(held.id);
            return;
        }
        if (def.category === 'clothing') {
            if (this.hooks.edgeStep() && this.hooks.input().mouseClicked)
                this.hooks.survival().wear(held.id);
            return;
        }
        if (def.gun) {
            this.shoot(held.id);
            return;
        }
        if (def.melee) {
            this.melee(
                def.melee.damage,
                def.melee.gather,
                def.melee.cooldown,
                def.melee.reach,
                held.id,
            );
        }
    }

    melee(damage: number, gather: number, cooldown: number, reach: number, tool?: ItemId): void {
        const p = this.hooks.player();
        p.attackTimer = cooldown;
        p.swingAnim = 0.2;

        // Living things first.
        for (const n of this.npcs.list) {
            if (dist(p.x, p.y, n.x, n.y) > reach + n.radius) continue;
            if (!inCone(p.x, p.y, p.facing, 0.9, n.x, n.y)) continue;
            this.npcs.damage(n, damage, p.x, p.y);
            this.particles.burst(n.x, n.y, 6, '#8c1f1f', { speed: 150, life: 0.35, size: 2.5 });
            this.particles.text(n.x, n.y - n.radius - 8, String(Math.round(damage)), '#ffd9d9');
            this.audio.hitFlesh();
            return;
        }

        // Then someone else's building.
        const tipX = p.x + Math.cos(p.facing) * reach * 0.75;
        const tipY = p.y + Math.sin(p.facing) * reach * 0.75;
        const near = this.build.inBlast(tipX, tipY, 26);
        const hostile = near.find((t) => t.owner !== 0);
        if (hostile) {
            this.hooks.damageBuilt(hostile.id, damage, p.x, p.y, true);
            return;
        }

        // Then resources. Plants are ignored: you pick those up, not swing at them.
        for (const node of this.world.nodesNear(p.x, p.y, reach + 40)) {
            if (node.hp <= 0 || node.kind === 'hemp') continue;
            if (dist(p.x, p.y, node.x, node.y) > reach + node.radius) continue;
            if (!inCone(p.x, p.y, p.facing, 0.9, node.x, node.y)) continue;
            this.gather(node.id, damage, gather, tool);
            return;
        }
        // A swing at nothing is silent: the sound is for landing a blow.
        this.particles.burst(tipX, tipY, 3, '#6b7a5c', { speed: 70, life: 0.2, size: 1.8 });
    }

    gather(nodeId: number, damage: number, gather: number, tool?: ItemId): void {
        const node = this.world.nodeById_(nodeId);
        if (!node) return;
        const def = NODES[node.kind];

        // Hemp is not chopped at all, it is collected with E, like a plant in Rust.
        if (node.kind === 'hemp') return;

        // The right tool for the job: hatchets on wood, pickaxes on rock.
        let mult = gather;
        if (tool === 'hatchet') mult *= def.prefers === 'chop' ? 1.6 : 0.45;
        if (tool === 'pickaxe') mult *= def.prefers === 'mine' ? 1.6 : 0.45;
        if (def.prefers === 'pick') mult = Math.max(1, gather * 0.8);

        node.hp -= damage;
        node.shake = 0.16;
        // Each material sounds like itself.
        if (node.kind === 'tree') this.audio.chopWood();
        else if (node.kind === 'metal_node') this.audio.hitMetal();
        else if (node.kind === 'sulfur_node') this.audio.hitSulfur();
        else this.audio.hitStone();
        this.particles.burst(node.x, node.y - 4, 6, def.color, {
            speed: 130,
            life: 0.4,
            size: 2.4,
            gravity: 220,
        });

        for (const [id, per] of Object.entries(def.yield) as [ItemId, number][]) {
            const amount = Math.max(1, Math.round(per * mult * 0.35));
            const left = acquire(
                this.hooks.player().hotbar,
                this.hooks.player().inventory,
                id,
                amount,
            );
            const got = amount - left;
            if (got > 0)
                this.particles.text(
                    node.x,
                    node.y - 24,
                    `+${got} ${ITEMS[id].name}`,
                    ITEMS[id].color,
                );
        }

        if (node.hp <= 0) {
            node.respawn = regrowthSeconds();
            this.npcs.invalidateNavigation();
            this.particles.burst(node.x, node.y, 16, def.color, {
                speed: 160,
                life: 0.8,
                size: 3.4,
                gravity: 200,
            });
            this.audio.treeFall();
        }
    }

    shoot(id: ItemId): void {
        const p = this.hooks.player();
        const def = ITEMS[id].gun!;
        const held = this.hooks.heldItem();

        if (def.magazine && held && held.id === id) {
            // Magazine weapons fire what is in them. Running dry is a reload, not a
            // trip to the inventory.
            if ((held.loaded ?? 0) <= 0) {
                p.attackTimer = 0.3;
                this.audio.deny();
                this.hooks.survival().reloadHeld();
                return;
            }
            if (!this.hooks.sandbox()) held.loaded = (held.loaded ?? 0) - 1;
        } else {
            if (countAcross(this.hooks.containers(), def.ammo) <= 0) {
                p.attackTimer = 0.4;
                this.audio.deny();
                this.hooks.notify(`Out of ${ITEMS[def.ammo].name.toLowerCase()}.`);
                return;
            }
            if (!this.hooks.sandbox()) removeAcross(this.hooks.containers(), def.ammo, 1);
        }
        p.attackTimer = def.cooldown;
        p.swingAnim = 0.1;
        const a = p.facing + randRange(Math.random, -def.spread, def.spread);
        if (def.rocket) {
            this.combat.launchRocket(
                def.ammo,
                p.x + Math.cos(a) * 30,
                p.y + Math.sin(a) * 30,
                a,
                def.speed,
                def.range,
                def.rocket.damage,
                def.rocket.radius,
                'player',
            );
            this.camera.shake(6, 0.2);
            this.audio.roar();
            return;
        }
        this.combat.fire(
            p.x + Math.cos(a) * 20,
            p.y + Math.sin(a) * 20,
            a,
            def.damage,
            def.speed,
            def.range,
            'player',
            // An arrow is a shaft in its own colour; everything else is a round.
            id === 'bow' ? ITEMS[def.ammo].color : WORLD.tracer,
        );
        this.particles.burst(p.x + Math.cos(a) * 22, p.y + Math.sin(a) * 22, 5, '#ffd28a', {
            speed: 180,
            life: 0.15,
            size: 2.4,
            dir: a,
            spread: 0.7,
        });
        this.camera.shake(id === 'rifle' ? 3 : 2, 0.12);
        this.audio.hit();
    }

    throwHeld(id: ItemId, mx: number, my: number): void {
        const p = this.hooks.player();
        const boom = ITEMS[id].boom;
        if (!boom) return;
        removeAcross(this.hooks.containers(), id, 1);
        const a = Math.atan2(my - p.y, mx - p.x);
        const power = clamp(dist(p.x, p.y, mx, my) * 2.4, 180, 520);
        this.combat.throwExplosive(
            id,
            p.x + Math.cos(a) * 18,
            p.y + Math.sin(a) * 18,
            a,
            power,
            boom.damage,
            boom.radius,
            boom.fuse,
            'player',
        );
        p.attackTimer = 0.8;
        this.audio.build();
        this.hooks.notify(`${ITEMS[id].name} thrown. ${boom.fuse.toFixed(1)}s.`);
    }

    buildPreview(): {
        kind: BuildKind;
        gx: number;
        gy: number;
        side?: EdgeSide;
        valid: boolean;
        reason: string;
    } | null {
        if (this.hooks.heldItem()?.id !== 'building_plan') return null;
        const m = this.camera.screenToWorld(this.hooks.input().mouseX, this.hooks.input().mouseY);
        if (this.hooks.buildKind() === 'foundation') {
            const gx = Math.floor(m.x / CELL);
            const gy = Math.floor(m.y / CELL);
            let reason = this.build.canPlaceFoundation(gx, gy, 0) ?? '';
            if (
                !reason &&
                dist(
                    this.hooks.player().x,
                    this.hooks.player().y,
                    gx * CELL + CELL / 2,
                    gy * CELL + CELL / 2,
                ) > 220
            ) {
                reason = 'Too far';
            }
            if (!reason && !this.canPayTier('twig')) reason = 'Need 10 wood';
            return { kind: 'foundation', gx, gy, valid: !reason, reason };
        }
        const edge = nearestEdge(m.x, m.y);
        let reason =
            this.build.canPlaceEdge(edge.gx, edge.gy, edge.side, 0, this.hooks.buildKind()) ?? '';
        const c = edgeCenter(edge.gx, edge.gy, edge.side);
        if (!reason && dist(this.hooks.player().x, this.hooks.player().y, c.x, c.y) > 220)
            reason = 'Too far';
        if (!reason && this.hooks.buildKind() !== 'door' && !this.canPayTier('twig'))
            reason = 'Need 10 wood';
        return {
            kind: this.hooks.buildKind(),
            gx: edge.gx,
            gy: edge.gy,
            side: edge.side,
            valid: !reason,
            reason,
        };
    }

    canPayTier(tier: BuildTier): boolean {
        if (this.hooks.sandbox()) return true;
        for (const [id, n] of Object.entries(TIER_DEFS[tier].cost) as [ItemId, number][]) {
            if (countAcross(this.hooks.containers(), id) < n) return false;
        }
        return true;
    }

    payTier(tier: BuildTier): void {
        if (this.hooks.sandbox()) return;
        for (const [id, n] of Object.entries(TIER_DEFS[tier].cost) as [ItemId, number][]) {
            removeAcross(this.hooks.containers(), id, n);
        }
    }

    placeBuilding(mx: number, my: number): void {
        const prev = this.buildPreview();
        if (!prev) return;
        if (!prev.valid) {
            this.audio.deny();
            if (prev.reason) this.hooks.notify(prev.reason);
            return;
        }
        if (prev.kind === 'foundation') {
            this.payTier('twig');
            this.build.placeFoundation(prev.gx, prev.gy, 0);
            this.hooks.reportBuild('foundation', prev.gx, prev.gy);
            this.npcs.invalidateNavigation();
        } else if (prev.kind === 'door') {
            const doorway = this.build.edgeAt(prev.gx, prev.gy, prev.side!);
            if (!doorway) return;
            this.payTier('twig');
            this.build.installDoor(doorway, 0);
            this.hooks.reportBuild('door', prev.gx, prev.gy, prev.side as 'n' | 'w');
        } else {
            this.payTier('twig');
            this.build.placeEdge(prev.gx, prev.gy, prev.side!, prev.kind as 'wall' | 'doorway', 0, {
                x: this.hooks.player().x,
                y: this.hooks.player().y,
            });
            this.hooks.reportBuild(prev.kind, prev.gx, prev.gy, prev.side as 'n' | 'w');
            this.npcs.invalidateNavigation();
        }
        this.audio.build();
        this.particles.burst(mx, my, 10, WORLD.gold, { speed: 110, life: 0.5, size: 2.8 });
        const solid = this.build.resolve(
            this.hooks.player().x,
            this.hooks.player().y,
            PLAYER.radius,
        );
        this.hooks.player().x = solid.x;
        this.hooks.player().y = solid.y;
    }

    /** The hammer upgrades your own pieces one tier at a time. */
    hammerUse(mx: number, my: number): void {
        const p = this.hooks.player();
        p.attackTimer = 0.35;
        // Pick the closest of your own pieces to the cursor, not just the first
        // one in the list: a foundation and its walls overlap heavily.
        let target: Structure | undefined;
        let bestD = Infinity;
        for (const t of this.build.inBlast(mx, my, 30)) {
            if (t.owner !== 0 || !('tier' in t)) continue;
            const c = centerOf(t);
            const d = dist(mx, my, c.x, c.y);
            if (d < bestD) {
                bestD = d;
                target = t as Structure;
            }
        }
        if (!target) {
            this.audio.deny();
            return;
        }
        if (dist(p.x, p.y, centerOf(target).x, centerOf(target).y) > 140) {
            this.hooks.notify('Too far to work on that.');
            return;
        }
        if (target.hp < target.maxHp) {
            const cost = Math.max(1, Math.ceil((target.maxHp - target.hp) * 0.06));
            if (countAcross(this.hooks.containers(), 'wood') < cost) {
                this.audio.deny();
                this.hooks.notify(`Need ${cost} wood to repair.`);
                return;
            }
            removeAcross(this.hooks.containers(), 'wood', cost);
            target.hp = target.maxHp;
            this.audio.build();
            this.particles.text(centerOf(target).x, centerOf(target).y - 20, 'repaired', '#c9e08a');
            return;
        }
        const next = this.build.nextTier(target);
        if (!next) {
            this.hooks.notify('Already sheet metal.');
            return;
        }
        const def = TIER_DEFS[next];
        for (const [id, n] of Object.entries(def.cost) as [ItemId, number][]) {
            if (countAcross(this.hooks.containers(), id) < n) {
                this.audio.deny();
                this.hooks.notify(`Need ${n} ${ITEMS[id].name.toLowerCase()}.`);
                return;
            }
        }
        for (const [id, n] of Object.entries(def.cost) as [ItemId, number][])
            removeAcross(this.hooks.containers(), id, n);
        this.build.upgrade(target, next);
        this.audio.build();
        this.particles.burst(centerOf(target).x, centerOf(target).y, 12, def.color, {
            speed: 110,
            life: 0.5,
            size: 3,
        });
        this.hooks.notify(`Upgraded to ${def.name}.`);
    }

    /**
     * Where the deployable in your hand would go if you clicked now, and
     * whether it can. The one answer both the click and the ghost read, so
     * what the ghost shows is exactly what a click does.
     */
    deployPreview(): {
        id: ItemId;
        gx: number;
        gy: number;
        valid: boolean;
        reason: string;
    } | null {
        const held = this.hooks.heldItem();
        if (!held || ITEMS[held.id].category !== 'deployable') return null;
        const m = this.camera.screenToWorld(this.hooks.input().mouseX, this.hooks.input().mouseY);
        const gx = Math.floor(m.x / CELL);
        const gy = Math.floor(m.y / CELL);
        let reason = this.build.canDeploy(gx, gy, 0, held.id as DeployableKind) ?? '';
        const p = this.hooks.player();
        if (!reason && dist(p.x, p.y, gx * CELL + CELL / 2, gy * CELL + CELL / 2) > DEPLOY_REACH)
            reason = 'Too far';
        return { id: held.id, gx, gy, valid: !reason, reason };
    }

    deployHeld(id: ItemId): void {
        const prev = this.deployPreview();
        if (!prev || prev.id !== id) return;
        if (!prev.valid) {
            this.audio.deny();
            this.hooks.notify(prev.reason);
            return;
        }
        const { gx, gy } = prev;
        removeAcross(this.hooks.containers(), id, 1);
        const hp = id === 'tool_cupboard' ? 400 : id === 'wooden_box' ? 200 : 250;
        this.build.deploy(id as DeployableKind, gx, gy, 0, hp);
        this.hooks.reportBuild(id, gx, gy);
        this.npcs.invalidateNavigation();
        this.hooks.player().attackTimer = 0.4;
        this.audio.build();
        this.hooks.notify(`${ITEMS[id].name} placed.`);
    }

    collectPlant(node: ResourceNode): void {
        const def = NODES[node.kind];
        node.hp = 0;
        node.respawn = randRange(Math.random, 200, 320);
        for (const [id, per] of Object.entries(def.yield) as [ItemId, number][]) {
            const left = acquire(
                this.hooks.player().hotbar,
                this.hooks.player().inventory,
                id,
                per,
            );
            const got = per - left;
            if (got > 0)
                this.particles.text(
                    node.x,
                    node.y - 20,
                    `+${got} ${ITEMS[id].name}`,
                    ITEMS[id].color,
                );
        }
        this.audio.pluck();
        this.particles.burst(node.x, node.y, 8, def.color, { speed: 110, life: 0.5, size: 2.4 });
    }
}
