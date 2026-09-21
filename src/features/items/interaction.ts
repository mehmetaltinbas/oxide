import { BuildSystem } from 'src/features/building/building';
import { TC_RADIUS } from 'src/features/building/constants/tc-radius.constant';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { centerOf } from 'src/features/building/utils/center-of.util';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { DEPLOYABLE_REACH_BONUS } from 'src/features/items/constants/deployable-reach-bonus.constant';
import { InteractTarget } from 'src/features/items/types/interact-target.type';
import { NODES } from 'src/features/world/constants/nodes.constant';
import { HeldItemSystem } from 'src/features/items/held-item';
import { Container } from 'src/features/items/types/container.interface';
import { GroundItem } from 'src/features/items/types/ground-item.interface';
import { ItemStack } from 'src/features/items/types/item-stack.interface';
import { acquire } from 'src/features/items/utils/acquire.util';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { PlayerState } from 'src/features/survival/types/player-state.interface';
import { LootCrate } from 'src/features/world/types/loot-crate.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { World } from 'src/features/world/world';
import { TAU } from 'src/shared/constants/tau.constant';
import { Audio } from 'src/shared/core/audio';
import { Particles } from 'src/shared/core/particles';
import { dist } from 'src/shared/utils/dist.util';
import { randRange } from 'src/shared/utils/rand-range.util';

/**
 * Reaching for the world around you: the interact key, picking things up,
 * putting things down, and the loose items lying on the ground.
 *
 * It owns the ground pile, because nothing else has any business appending to
 * it. See docs/architecture/project-structure.md.
 */
export interface InteractionHooks {
    player(): PlayerState;
    /** Tell the server a door was opened or shut. Everybody shares the door. */
    reportDoor(id: number, open: boolean): void;
    heldItem(): ItemStack | null;
    /** Using what you hold, for the interactions that are really a use. */
    held(): HeldItemSystem;
    /** Whether a point is reachable, or sealed inside somebody's base. */
    canReach(x: number, y: number): boolean;
    /** A plant within arm's reach that can be picked rather than hit. */
    harvestableNear(): ResourceNode | null;
    /** Whether this point is on the shore of a lake, so drinking is possible. */
    nearWater(x: number, y: number): boolean;
    /** Show this container's contents. */
    openContainer(oc: {
        container: Container;
        title: string;
        source: Deployable | LootCrate | null;
    }): void;
    /** Select a slot in the inventory panel. */
    inspect(container: Container, index: number): void;
    notify(text: string): void;
}

export class InteractionSystem {
    /** Everything lying loose on the floor. */
    ground: GroundItem[] = [];

    private nextGroundId = 1;

    constructor(
        private world: World,
        private build: BuildSystem,
        private particles: Particles,
        private audio: Audio,
        private hooks: InteractionHooks,
    ) {}

    /** Move a stack from the pack onto the first free belt slot. */
    moveToBelt(from: Container, index: number): void {
        const stack = from.slots[index];
        if (!stack) return;
        const free = this.hooks.player().hotbar.slots.findIndex((sl) => sl === null);
        if (free < 0) {
            this.audio.deny();
            this.hooks.notify('Belt is full.');
            return;
        }
        this.hooks.player().hotbar.slots[free] = stack;
        from.slots[index] = null;
        this.hooks.inspect(this.hooks.player().hotbar, free);
        this.hooks.player().activeSlot = free;
    }

    dropHeld(): void {
        const held = this.hooks.heldItem();
        if (!held) return;
        this.dropStack(
            { ...held },
            this.hooks.player().x + Math.cos(this.hooks.player().facing) * 34,
            this.hooks.player().y + Math.sin(this.hooks.player().facing) * 34,
        );
        this.hooks.player().hotbar.slots[this.hooks.player().activeSlot] = null;
    }

    /** The dropped stack under the player's nose, if any. */
    groundItemNear(): GroundItem | null {
        let best: GroundItem | null = null;
        let bestD: number = PLAYER.interact;
        for (const g of this.ground) {
            const d = dist(this.hooks.player().x, this.hooks.player().y, g.x, g.y);
            if (d < bestD) {
                bestD = d;
                best = g;
            }
        }
        return best;
    }

    pickUp(g: GroundItem): void {
        const left = acquire(
            this.hooks.player().hotbar,
            this.hooks.player().inventory,
            g.item.id,
            g.item.count,
        );
        const got = g.item.count - left;
        if (got > 0) {
            this.particles.text(
                this.hooks.player().x,
                this.hooks.player().y - 26,
                `+${got} ${ITEMS[g.item.id].name}`,
                ITEMS[g.item.id].color,
            );
            this.audio.pickup();
        } else {
            this.audio.deny();
            this.hooks.notify('No room for that.');
        }
        if (left <= 0) {
            const i = this.ground.indexOf(g);
            if (i >= 0) this.ground.splice(i, 1);
        } else {
            g.item.count = left;
        }
    }

    /**
     * The one thing E would act on right now, or nothing.
     *
     * This is the single source of truth for "what is in reach": `interact`
     * acts on it and the HUD prints its prompt, so the two cannot disagree.
     * They used to be two separate scans with their own ranges, and anything
     * one knew about and the other did not (water, a box at the edge of reach,
     * a door measured from a different point) worked without a prompt, or
     * prompted and then did nothing. Adding a new kind of interactable means
     * adding it here, and its prompt comes with it.
     */
    target(): InteractTarget | null {
        const p = this.hooks.player();

        // Loose items first, then plants: both are "press E to pick that up".
        const loose = this.groundItemNear();
        if (loose) return { kind: 'item', item: loose };
        const plant = this.hooks.harvestableNear();
        if (plant) return { kind: 'plant', node: plant };

        // Doors next: opening one is the most common thing you do.
        let bestDoor: Structure | null = null;
        let bestD: number = PLAYER.interact;
        for (const s of this.build.structures) {
            if (s.kind !== 'door') continue;
            const c = centerOf(s);
            const d = dist(p.x, p.y, c.x, c.y);
            if (d < bestD && this.hooks.canReach(c.x, c.y)) {
                bestD = d;
                bestDoor = s;
            }
        }
        if (bestDoor) return { kind: 'door', door: bestDoor };

        // Then containers and workstations.
        let bestDep: Deployable | null = null;
        bestD = PLAYER.interact + DEPLOYABLE_REACH_BONUS;
        for (const d of this.build.deployables) {
            const dd = dist(p.x, p.y, d.x, d.y);
            if (dd < bestD && this.hooks.canReach(d.x, d.y)) {
                bestD = dd;
                bestDep = d;
            }
        }
        if (bestDep) return { kind: 'deployable', deployable: bestDep };

        // Then loot crates.
        for (const c of this.world.crates) {
            if (c.looted) continue;
            if (dist(p.x, p.y, c.x, c.y) > PLAYER.interact) continue;
            if (!this.hooks.canReach(c.x, c.y)) continue;
            return { kind: 'crate', crate: c };
        }

        // Then fresh water. The sea is salt and nobody drinks it.
        if (this.world.freshAt(p.x, p.y) || this.hooks.nearWater(p.x, p.y)) {
            return { kind: 'water' };
        }
        return null;
    }

    /** What the prompt at the bottom of the screen says for a target. */
    prompt(t: InteractTarget): string {
        switch (t.kind) {
            case 'item': {
                const n = t.item.item.count > 1 ? ` x${t.item.item.count}` : '';
                return `e  pick up ${ITEMS[t.item.item.id].name}${n}`;
            }
            case 'plant':
                return `e  pick ${NODES[t.node.kind].name.toLowerCase()}`;
            case 'door':
                return t.door.open ? 'e  close door' : 'e  open door';
            case 'deployable': {
                const d = t.deployable;
                return `e  ${ITEMS[d.kind].name}${d.owner !== 0 ? ' (not yours)' : ''}`;
            }
            case 'crate':
                return 'e  loot crate';
            case 'water':
                return 'e  drink';
        }
    }

    interact(): void {
        const p = this.hooks.player();
        const t = this.target();
        if (!t) {
            this.audio.deny();
            return;
        }
        switch (t.kind) {
            case 'item':
                this.pickUp(t.item);
                return;
            case 'plant':
                this.hooks.held().collectPlant(t.node);
                return;
            case 'door': {
                const door = t.door;
                if (door.owner !== 0 && door.locked) {
                    this.audio.deny();
                    this.hooks.notify('Locked. You will have to break it.');
                    return;
                }
                this.build.setDoorOpen(door, !door.open);
                this.hooks.reportDoor(door.id, !!door.open);
                this.audio.build();
                return;
            }
            case 'deployable':
                this.openDeployable(t.deployable);
                return;
            case 'crate':
                this.hooks.openContainer({
                    container: t.crate.container,
                    title: 'Crate',
                    source: t.crate,
                });
                this.audio.build();
                return;
            case 'water':
                p.hydration = Math.min(PLAYER.maxHydration, p.hydration + 34);
                this.audio.pickup();
                this.particles.text(p.x, p.y - 26, 'drank', '#5aa8d8');
                return;
        }
    }

    openDeployable(d: Deployable): void {
        if (d.owner !== 0 && d.kind !== 'wooden_box') {
            this.audio.deny();
            this.hooks.notify('Not yours.');
            return;
        }
        if (d.kind === 'campfire' || d.kind === 'furnace') {
            d.lit = !d.lit;
            this.audio.build();
            this.hooks.notify(`${ITEMS[d.kind].name} ${d.lit ? 'lit' : 'out'}.`);
            if (d.container) {
                this.hooks.openContainer({
                    container: d.container,
                    title: ITEMS[d.kind].name,
                    source: d,
                });
            }
            return;
        }
        if (d.container) {
            this.hooks.openContainer({
                container: d.container,
                title: ITEMS[d.kind].name,
                source: d,
            });
            this.audio.build();
            return;
        }
        if (d.kind === 'tool_cupboard') {
            this.hooks.notify(`Tool cupboard: claims ${Math.round(TC_RADIUS)} units around it.`);
            return;
        }
        this.hooks.notify(ITEMS[d.kind].name);
    }

    /** Throw a whole slot on the floor in front of the player. */
    dropSlot(container: Container, index: number): void {
        const stack = container.slots[index];
        if (!stack) return;
        container.slots[index] = null;
        this.dropStack(
            stack,
            this.hooks.player().x + Math.cos(this.hooks.player().facing) * 36,
            this.hooks.player().y + Math.sin(this.hooks.player().facing) * 36,
        );
        this.audio.pickup();
        this.hooks.notify(`Dropped ${ITEMS[stack.id].name}.`);
    }

    dropStack(stack: ItemStack, x: number, y: number): void {
        const a = Math.random() * TAU;
        this.ground.push({
            id: this.nextGroundId++,
            item: stack,
            x,
            y,
            vx: Math.cos(a) * randRange(Math.random, 30, 90),
            vy: Math.sin(a) * randRange(Math.random, 30, 90),
            life: 600,
            bob: Math.random() * TAU,
        });
    }

    updateGround(dt: number): void {
        for (let i = this.ground.length - 1; i >= 0; i--) {
            const g = this.ground[i];
            g.life -= dt;
            g.bob += dt * 4;
            g.x += g.vx * dt;
            g.y += g.vy * dt;
            g.vx *= 1 - 4 * dt;
            g.vy *= 1 - 4 * dt;
            if (g.life <= 0) {
                this.ground.splice(i, 1);
                continue;
            }
            // Nothing is picked up by walking over it. See `interact`.
        }
    }
}
