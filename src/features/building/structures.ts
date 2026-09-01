import { BuildSystem } from 'src/features/building/building';
import { DECAY_PER_HOUR } from 'src/features/building/constants/decay-per-hour.constant';
import { TIER_DEFS } from 'src/features/building/constants/tier-defs.constant';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { centerOf } from 'src/features/building/utils/center-of.util';
import { Faction } from 'src/features/combat/types/faction.type';
import { COOK_SECONDS } from 'src/features/items/constants/cook-seconds.constant';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { SMELT } from 'src/features/items/constants/smelt.constant';
import { InteractionSystem } from 'src/features/items/interaction';
import { ItemId } from 'src/features/items/types/item-id.type';
import { addItem } from 'src/features/items/utils/add-item.util';
import { removeAcross } from 'src/features/items/utils/remove-across.util';
import { NpcSystem } from 'src/features/npcs/npcs';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { SurvivalSystem } from 'src/features/survival/survival';
import { PlayerState } from 'src/features/survival/types/player-state.interface';
import { Audio } from 'src/shared/core/audio';
import { Camera } from 'src/shared/core/camera';
import { Particles } from 'src/shared/core/particles';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { dist } from 'src/shared/utils/dist.util';
import { randRange } from 'src/shared/utils/rand-range.util';

/**
 * What happens to things that have been built: taking damage, being destroyed,
 * decaying while you are away, and the per-tick life of a furnace or a campfire.
 * Explosions live here too, because what a charge mostly does is break a piece.
 *
 * See docs/architecture/project-structure.md.
 */
export interface StructureHooks {
    player(): PlayerState;
    /** Loot and rubble land on the ground pile. */
    interaction(): InteractionSystem;
    /** Blast damage to the player goes through the survival system. */
    survival(): SurvivalSystem;
    /** Is the open container panel showing this deployable's contents? */
    viewingContainerOf(d: Deployable): boolean;
    /** Shut the container panel, because what it was showing is gone. */
    closeContainer(): void;
}

export class StructureSystem {
    constructor(
        private build: BuildSystem,
        private npcs: NpcSystem,
        private particles: Particles,
        private audio: Audio,
        private camera: Camera,
        private hooks: StructureHooks,
    ) {}

    updateDeployables(dt: number): void {
        for (let i = this.build.deployables.length - 1; i >= 0; i--) {
            const d = this.build.deployables[i];
            if (d.hp <= 0) {
                this.destroyDeployable(d);
                continue;
            }
            if (!d.lit || !d.container) continue;

            if (Math.random() < dt * 20) this.particles.ember(d.x, d.y - 4);

            // Burn wood for fuel.
            if ((d.fuel ?? 0) <= 0) {
                const took = removeAcross([d.container], 'wood', 1);
                if (took > 0) d.fuel = 4;
                else {
                    d.lit = false;
                    continue;
                }
            }
            d.fuel = (d.fuel ?? 0) - dt;
            d.progress = (d.progress ?? 0) + dt;

            if (d.kind === 'furnace' && d.progress >= SMELT.secondsPerUnit) {
                d.progress = 0;
                if (removeAcross([d.container], 'metal_ore', 1) > 0)
                    addItem(d.container, 'metal', 1);
                else if (removeAcross([d.container], 'sulfur_ore', 1) > 0)
                    addItem(d.container, 'sulfur', 1);
                if (Math.random() < SMELT.charcoalPer) addItem(d.container, 'charcoal', 1);
            }
            if (d.kind === 'campfire' && d.progress >= COOK_SECONDS) {
                d.progress = 0;
                if (removeAcross([d.container], 'meat_raw', 1) > 0)
                    addItem(d.container, 'meat_cooked', 1);
            }
        }
    }

    dropLootTable(
        kind: NpcKind,
        x: number,
        y: number,
        loot: Partial<Record<ItemId, [number, number]>>,
        weapon: ItemId | null,
    ): void {
        this.particles.burst(x, y, 18, WORLD.blood, { speed: 180, life: 0.8, size: 3.4 });
        this.audio.enemyDie();
        for (const [id, [lo, hi]] of Object.entries(loot) as [ItemId, [number, number]][]) {
            const n = Math.round(randRange(Math.random, lo, hi));
            if (n > 0) this.hooks.interaction().dropStack({ id, count: n }, x, y);
        }
        // Whatever they were shooting at you with is on the ground now, with
        // whatever was left in it.
        if (weapon) {
            const mag = ITEMS[weapon].gun?.magazine;
            this.hooks.interaction().dropStack(
                {
                    id: weapon,
                    count: 1,
                    loaded: mag ? Math.round(randRange(Math.random, 0, mag)) : undefined,
                },
                x,
                y,
            );
        }
        void kind;
    }

    damageBuilt(id: number, amount: number, fromX: number, fromY: number, melee: boolean): void {
        const target = this.build.byId(id);
        if (!target) return;
        const dealt = this.build.damage(target, amount, fromX, fromY, melee);
        const c = centerOf(target);
        this.particles.burst(c.x, c.y, 5, '#b0a68a', { speed: 120, life: 0.35, size: 2.4 });
        if (dealt < amount * 0.5) this.particles.text(c.x, c.y - 18, 'hard side', '#c9c0a0');
        // Twig and wood thud; stone and sheet metal ring.
        const tier = 'tier' in target ? (target as Structure).tier : 'wood';
        if (tier === 'metal') this.audio.hitStructureMetal();
        else if (tier === 'stone') this.audio.hitStone();
        else this.audio.hitStructureWood();
        if (target.hp <= 0) {
            if ('tier' in target) this.destroyStructure(target as Structure);
            else this.destroyDeployable(target as Deployable);
        }
    }

    destroyStructure(s: Structure): void {
        const c = centerOf(s);
        this.build.removeStructure(s);
        this.npcs.invalidateNavigation();
        this.particles.burst(c.x, c.y, 20, TIER_DEFS[s.tier].color, {
            speed: 190,
            life: 0.8,
            size: 3.6,
            gravity: 140,
        });
        this.audio.treeFall();
        this.camera.shake(5, 0.25);
    }

    destroyDeployable(d: Deployable): void {
        this.npcs.invalidateNavigation();
        // Anything inside a broken box spills out for whoever is standing there.
        if (d.container) {
            for (const s of d.container.slots)
                if (s) this.hooks.interaction().dropStack(s, d.x, d.y);
        }
        this.build.removeDeployable(d);
        if (this.hooks.viewingContainerOf(d)) {
            this.hooks.closeContainer();
        }
        this.particles.burst(d.x, d.y, 18, WORLD.timber, { speed: 170, life: 0.8, size: 3.4 });
        this.audio.treeFall();
    }

    /**
     * A charge blows up. Anything alive nearby catches the blast, but structures
     * do not: a satchel breaks the wall it is stuck to and nothing else. That is
     * what makes raiding a matter of placing charges rather than lobbing them at
     * a compound and watching it fall down. `stuckTo` is the piece it was on;
     * a charge that never stuck to anything (thrown into open ground) hurts only
     * what is standing there.
     */
    explode(
        x: number,
        y: number,
        radius: number,
        damage: number,
        owner: Faction,
        stuckTo: number | null = null,
    ): void {
        this.particles.burst(x, y, 46, WORLD.fire, { speed: 300, life: 1, size: 5 });
        this.particles.burst(x, y, 24, WORLD.emberHot, { speed: 200, life: 0.7, size: 3.6 });
        this.camera.shake(14, 0.5);
        this.audio.roar();

        if (stuckTo !== null) {
            const target = this.build.inBlast(x, y, radius).find((t) => t.id === stuckTo);
            if (target) {
                target.hp -= damage;
                target.flash = 0.2;
                if (target.hp <= 0) {
                    if ('tier' in target) this.destroyStructure(target as Structure);
                    else this.destroyDeployable(target as Deployable);
                }
            }
        }
        for (const n of this.npcs.list) {
            const d = dist(x, y, n.x, n.y);
            if (d > radius + n.radius) continue;
            this.npcs.damage(n, damage * 0.25 * (1 - d / (radius + n.radius)), x, y, 300);
        }
        const p = this.hooks.player();
        if (p.alive) {
            const d = dist(x, y, p.x, p.y);
            if (d < radius + PLAYER.radius) {
                this.hooks
                    .survival()
                    .hurtPlayer(damage * 0.18 * (1 - d / (radius + PLAYER.radius)), x, y);
            }
        }
        void owner;
    }

    /**
     * The piece a raider should be hitting: always an edge, never a floor.
     * Floors are usually the weakest thing in a base, but they sit inside the
     * walls, so a raider that targets one just mills about outside forever.
     */
    weakestStructureNear(x: number, y: number, radius: number): Structure | null {
        let best: Structure | null = null;
        let bestScore = Infinity;
        let fallback: Structure | null = null;
        let fallbackScore = Infinity;

        for (const s of this.build.structures) {
            if (s.owner !== 0) continue;
            const c = centerOf(s);
            const d = dist(x, y, c.x, c.y);
            if (d > radius) continue;
            if (s.side) {
                // Break the cheapest wall, tie-broken by how close it is.
                const score = s.hp + d * 0.5;
                if (score < bestScore) {
                    bestScore = score;
                    best = s;
                }
            } else if (s.hp + d < fallbackScore) {
                fallbackScore = s.hp + d;
                fallback = s;
            }
        }
        return best ?? fallback;
    }

    /** Structures rot when nobody keeps a cupboard supplied, as in Rust. */
    applyDecay(hours: number): void {
        if (hours <= 0) return;
        const loss = DECAY_PER_HOUR * hours;
        for (const s of this.build.structures) {
            if (s.owner !== 0) continue;
            s.hp -= s.maxHp * loss;
        }
        for (let i = this.build.structures.length - 1; i >= 0; i--) {
            const s = this.build.structures[i];
            if (s.hp <= 0) this.build.removeStructure(s);
        }
    }
}
