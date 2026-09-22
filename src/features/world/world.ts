import { regrowthSeconds } from 'src/features/world/utils/regrowth-seconds.util';
import { REGROWTH } from 'src/features/world/constants/regrowth.constant';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { ItemId } from 'src/features/items/types/item-id.type';
import { emptyContainer } from 'src/features/items/utils/empty-container.util';
import { BIOME_H } from 'src/features/world/constants/biome-h.constant';
import { BIOME_TILE } from 'src/features/world/constants/biome-tile.constant';
import { BIOME_W } from 'src/features/world/constants/biome-w.constant';
import { MONUMENT_COUNTS } from 'src/features/world/constants/monument-counts.constant';
import { MONUMENTS } from 'src/features/world/constants/monuments.constant';
import { NODES } from 'src/features/world/constants/nodes.constant';
import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';
import { Biome } from 'src/features/world/types/biome.type';
import { LootCrate } from 'src/features/world/types/loot-crate.interface';
import { MonumentDef } from 'src/features/world/types/monument-def.interface';
import { Monument } from 'src/features/world/types/monument.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { TAU } from 'src/shared/constants/tau.constant';
import { dist } from 'src/shared/utils/dist.util';
import { dist2 } from 'src/shared/utils/dist2.util';
import { makeRng } from 'src/shared/utils/make-rng.util';
import { randRange } from 'src/shared/utils/rand-range.util';

/** How much of the map is sea on each side, and how sharply the land falls off. */
/** How many biome tiles inland the sand reaches. */
const BEACH_WIDTH = 1;

const ISLAND_MARGIN = 0.03;
const ISLAND_FALLOFF = 0.11;

function clamp01(v: number): number {
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

const HASH_CELL = 160;
const HASH_COLS = Math.ceil(WORLD_W / HASH_CELL);
const HASH_ROWS = Math.ceil(WORLD_H / HASH_CELL);

/** Uniform grid over static scenery so lookups near the player stay cheap. */
class Hash<T extends { x: number; y: number }> {
    private cells: T[][] = Array.from({ length: HASH_COLS * HASH_ROWS }, () => []);

    insert(item: T): void {
        const cx = Math.floor(item.x / HASH_CELL);
        const cy = Math.floor(item.y / HASH_CELL);
        if (cx < 0 || cy < 0 || cx >= HASH_COLS || cy >= HASH_ROWS) return;
        this.cells[cy * HASH_COLS + cx].push(item);
    }

    query(x: number, y: number, radius: number, out: T[]): T[] {
        out.length = 0;
        const x0 = Math.max(0, Math.floor((x - radius) / HASH_CELL));
        const x1 = Math.min(HASH_COLS - 1, Math.floor((x + radius) / HASH_CELL));
        const y0 = Math.max(0, Math.floor((y - radius) / HASH_CELL));
        const y1 = Math.min(HASH_ROWS - 1, Math.floor((y + radius) / HASH_CELL));
        for (let cy = y0; cy <= y1; cy++) {
            for (let cx = x0; cx <= x1; cx++) {
                const cell = this.cells[cy * HASH_COLS + cx];
                for (let i = 0; i < cell.length; i++) out.push(cell[i]);
            }
        }
        return out;
    }

    rect(x0: number, y0: number, x1: number, y1: number, out: T[]): T[] {
        out.length = 0;
        const cx0 = Math.max(0, Math.floor(x0 / HASH_CELL));
        const cx1 = Math.min(HASH_COLS - 1, Math.floor(x1 / HASH_CELL));
        const cy0 = Math.max(0, Math.floor(y0 / HASH_CELL));
        const cy1 = Math.min(HASH_ROWS - 1, Math.floor(y1 / HASH_CELL));
        for (let cy = cy0; cy <= cy1; cy++) {
            for (let cx = cx0; cx <= cx1; cx++) {
                const cell = this.cells[cy * HASH_COLS + cx];
                for (let i = 0; i < cell.length; i++) out.push(cell[i]);
            }
        }
        return out;
    }
}

export class World {
    seed: number;
    biomes: Biome[] = [];
    /**
     * Which water tiles are fresh: 1 for a lake, 0 for the sea or dry land.
     *
     * The biome grid has one kind of water, but you can drink a lake and not
     * the sea. Sea is whatever water joins up with the rim; anything the flood
     * from the rim cannot reach is landlocked, and landlocked water is fresh.
     */
    fresh: Uint8Array = new Uint8Array(0);
    nodes: ResourceNode[] = [];
    monuments: Monument[] = [];
    crates: LootCrate[] = [];
    /** Spawn beach for a fresh character. */
    spawn = { x: WORLD_W / 2, y: WORLD_H / 2 };
    /** Every usable tile along the southern shore. */
    beach: { x: number; y: number }[] = [];

    /**
     * Set by the game: true when a point is inside somebody's base, so a felled
     * tree never grows back up through a floor.
     */
    respawnBlocked: ((x: number, y: number) => boolean) | null = null;

    private nodeHash = new Hash<ResourceNode>();
    private nodeById = new Map<number, ResourceNode>();
    private scratch: ResourceNode[] = [];
    private nextId = 1;

    constructor(seed: number) {
        this.seed = seed;
        this.generate();
    }

    newId(): number {
        return this.nextId++;
    }

    // -------------------------------------------------------------- worldgen

    private generate(): void {
        const rng = makeRng(this.seed);
        this.generateBiomes(rng);
        this.markFreshWater();
        this.placeMonuments(rng);
        this.scatterNodes(rng);
        this.pickSpawn(rng);
    }

    /**
     * Biomes come from a couple of coarse value-noise fields: one for moisture,
     * one for latitude-ish banding, which gives snow at one end and desert at
     * the other with forest in between.
     */
    private generateBiomes(rng: () => number): void {
        const coarse = 12;
        const field: number[] = [];
        for (let i = 0; i < coarse * coarse; i++) field.push(rng());

        const sample = (fx: number, fy: number): number => {
            const gx = Math.min(coarse - 2, Math.floor(fx * (coarse - 1)));
            const gy = Math.min(coarse - 2, Math.floor(fy * (coarse - 1)));
            const tx = fx * (coarse - 1) - gx;
            const ty = fy * (coarse - 1) - gy;
            const a = field[gy * coarse + gx];
            const b = field[gy * coarse + gx + 1];
            const c = field[(gy + 1) * coarse + gx];
            const d = field[(gy + 1) * coarse + gx + 1];
            const sx = tx * tx * (3 - 2 * tx);
            const sy = ty * ty * (3 - 2 * ty);
            return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy;
        };

        this.biomes = new Array(BIOME_W * BIOME_H);
        for (let y = 0; y < BIOME_H; y++) {
            for (let x = 0; x < BIOME_W; x++) {
                const fx = x / BIOME_W;
                const fy = y / BIOME_H;
                const n = sample(fx, fy);
                // Latitude band: north is cold, south is hot.
                const band = fy;

                // Island falloff: push the value down towards the rim so the map ends
                // in open sea on every side rather than a hard wall.
                const edge = Math.min(fx, 1 - fx, fy, 1 - fy);
                const shore = clamp01((edge - ISLAND_MARGIN) / ISLAND_FALLOFF);
                const height = n * (0.5 + 0.5 * shore) - (1 - shore) * 0.45;

                let b: Biome;
                if (height < 0.17) b = 'water';
                // The bands are ragged, not stripes: dry ground in the south turns to
                // desert and wet ground stays green, so the far coast is a mix.
                else if (band < 0.24 && height < 0.6 && n > 0.34 - (0.24 - band)) b = 'snow';
                else if (band > 0.76 && height < 0.6 && n > 0.34 - (band - 0.76)) b = 'desert';
                else if (height > 0.6) b = 'forest';
                else b = 'grass';
                this.biomes[y * BIOME_W + x] = b;
            }
        }

        // Sand where the temperate land meets the sea. Snow runs straight into
        // the water and desert is already sand, so neither grows a beach; the
        // grass and forest coasts get one, and that is where people wash up.
        const base = this.biomes.slice();
        for (let y = 0; y < BIOME_H; y++) {
            for (let x = 0; x < BIOME_W; x++) {
                const here = base[y * BIOME_W + x];
                if (here !== 'grass' && here !== 'forest') continue;
                let coastal = false;
                for (let oy = -BEACH_WIDTH; oy <= BEACH_WIDTH && !coastal; oy++) {
                    for (let ox = -BEACH_WIDTH; ox <= BEACH_WIDTH; ox++) {
                        const tx = x + ox;
                        const ty = y + oy;
                        if (tx < 0 || ty < 0 || tx >= BIOME_W || ty >= BIOME_H) continue;
                        if (base[ty * BIOME_W + tx] === 'water') {
                            coastal = true;
                            break;
                        }
                    }
                }
                if (coastal) this.biomes[y * BIOME_W + x] = 'beach';
            }
        }

        // Hard sea border, so nothing can be built or stranded on the very rim.
        const rim = 2;
        for (let y = 0; y < BIOME_H; y++) {
            for (let x = 0; x < BIOME_W; x++) {
                if (x < rim || y < rim || x >= BIOME_W - rim || y >= BIOME_H - rim) {
                    this.biomes[y * BIOME_W + x] = 'water';
                }
            }
        }

        // A rough ring road, which is where scrap and traffic naturally go.
        const cx = BIOME_W / 2;
        const cy = BIOME_H / 2;
        const rr = Math.min(BIOME_W, BIOME_H) * 0.34;
        for (let a = 0; a < 720; a++) {
            const ang = (a / 720) * TAU;
            const wobble = 1 + Math.sin(ang * 3) * 0.08;
            const x = Math.round(cx + Math.cos(ang) * rr * wobble);
            const y = Math.round(cy + Math.sin(ang) * rr * wobble * 0.9);
            for (let ox = -1; ox <= 1; ox++) {
                for (let oy = -1; oy <= 1; oy++) {
                    const tx = x + ox;
                    const ty = y + oy;
                    if (tx < 0 || ty < 0 || tx >= BIOME_W || ty >= BIOME_H) continue;
                    if (this.biomes[ty * BIOME_W + tx] !== 'water')
                        this.biomes[ty * BIOME_W + tx] = 'road';
                }
            }
        }
    }

    /** Mark every water tile that does not join the open sea as a lake. */
    private markFreshWater(): void {
        const sea = new Uint8Array(BIOME_W * BIOME_H);
        const stack: number[] = [];
        for (let x = 0; x < BIOME_W; x++) stack.push(x, (BIOME_H - 1) * BIOME_W + x);
        for (let y = 0; y < BIOME_H; y++) stack.push(y * BIOME_W, y * BIOME_W + BIOME_W - 1);
        while (stack.length) {
            const i = stack.pop()!;
            if (sea[i] || this.biomes[i] !== 'water') continue;
            sea[i] = 1;
            const x = i % BIOME_W;
            const y = (i - x) / BIOME_W;
            if (x > 0) stack.push(i - 1);
            if (x < BIOME_W - 1) stack.push(i + 1);
            if (y > 0) stack.push(i - BIOME_W);
            if (y < BIOME_H - 1) stack.push(i + BIOME_W);
        }
        this.fresh = new Uint8Array(BIOME_W * BIOME_H);
        for (let i = 0; i < this.fresh.length; i++) {
            if (this.biomes[i] === 'water' && !sea[i]) this.fresh[i] = 1;
        }
    }

    /** Whether this point is in a lake, as opposed to the sea or on land. */
    freshAt(x: number, y: number): boolean {
        const bx = Math.floor(x / BIOME_TILE);
        const by = Math.floor(y / BIOME_TILE);
        if (bx < 0 || by < 0 || bx >= BIOME_W || by >= BIOME_H) return false;
        return this.fresh[by * BIOME_W + bx] === 1;
    }

    biomeAt(x: number, y: number): Biome {
        const bx = Math.floor(x / BIOME_TILE);
        const by = Math.floor(y / BIOME_TILE);
        if (bx < 0 || by < 0 || bx >= BIOME_W || by >= BIOME_H) return 'grass';
        return this.biomes[by * BIOME_W + bx];
    }

    private placeMonuments(rng: () => number): void {
        let serial = 0;
        for (const def of MONUMENTS) {
            const wanted = MONUMENT_COUNTS[def.id] ?? 1;
            for (let n = 0; n < wanted; n++) {
                for (let attempt = 0; attempt < 400; attempt++) {
                    const x = randRange(rng, def.radius + 160, WORLD_W - def.radius - 160);
                    const y = randRange(rng, def.radius + 160, WORLD_H - def.radius - 160);
                    if (this.biomeAt(x, y) === 'water') continue;
                    let clash = false;
                    for (const m of this.monuments) {
                        if (dist(x, y, m.x, m.y) < m.radius + def.radius + 900) clash = true;
                    }
                    if (clash) continue;
                    this.monuments.push({
                        id: `${def.id}#${serial++}`,
                        defId: def.id,
                        x,
                        y,
                        radius: def.radius,
                    });
                    this.spawnCrates(def, x, y, rng);
                    break;
                }
            }
        }
    }

    private spawnCrates(def: MonumentDef, mx: number, my: number, rng: () => number): void {
        for (let i = 0; i < def.crates; i++) {
            const a = rng() * TAU;
            const r = randRange(rng, def.radius * 0.2, def.radius * 0.78);
            const crate: LootCrate = {
                id: this.newId(),
                x: mx + Math.cos(a) * r,
                y: my + Math.sin(a) * r,
                monumentId: def.id,
                container: emptyContainer(6),
                looted: false,
                respawn: 0,
            };
            this.crates.push(crate);
            this.fillCrate(crate);
        }
    }

    /** Roll a crate's contents from its monument's loot table. */
    fillCrate(crate: LootCrate): void {
        const def = MONUMENTS.find((m) => m.id === crate.monumentId);
        if (!def) return;
        crate.container = emptyContainer(6);
        const entries = Object.entries(def.loot) as [ItemId, [number, number]][];
        let slot = 0;
        for (const [id, [lo, hi]] of entries) {
            if (slot >= crate.container.slots.length) break;
            if (Math.random() < 0.45) continue;
            const n = Math.round(randRange(Math.random, lo, hi));
            if (n <= 0) continue;
            crate.container.slots[slot++] = { id, count: Math.min(n, ITEMS[id].stack) };
        }
        if (slot === 0 && entries.length > 0) {
            const [id, [lo, hi]] = entries[Math.floor(Math.random() * entries.length)];
            crate.container.slots[0] = {
                id,
                count: Math.max(1, Math.round(randRange(Math.random, lo, hi))),
            };
        }
        crate.looted = false;
    }

    private scatterNodes(rng: () => number): void {
        const push = (kind: ResourceNode['kind'], x: number, y: number): void => {
            const def = NODES[kind];
            const node: ResourceNode = {
                id: this.newId(),
                kind,
                x,
                y,
                radius: def.radius * randRange(rng, 0.85, 1.2),
                hp: def.hp,
                maxHp: def.hp,
                respawn: 0,
                shake: 0,
                seed: Math.floor(rng() * 1e5),
            };
            this.nodes.push(node);
            this.nodeHash.insert(node);
            this.nodeById.set(node.id, node);
        };

        const free = (x: number, y: number, min: number): boolean => {
            if (this.biomeAt(x, y) === 'water') return false;
            // Nothing grows or sits on a road, not even overhanging one: the
            // centre and a node's width either side all have to be off it.
            const edge = min * 0.5;
            for (const [dx, dy] of [
                [0, 0],
                [-edge, 0],
                [edge, 0],
                [0, -edge],
                [0, edge],
            ] as const) {
                if (this.biomeAt(x + dx, y + dy) === 'road') return false;
            }
            for (const m of this.monuments) {
                if (dist(x, y, m.x, m.y) < m.radius + 40) return false;
            }
            const m2 = min * min;
            for (const n of this.nodeHash.query(x, y, min, this.scratch)) {
                if (dist2(x, y, n.x, n.y) < m2) return false;
            }
            return true;
        };

        // Trees cluster in forest, thin out elsewhere, and skip sand entirely.
        for (let i = 0; i < 176000; i++) {
            const x = randRange(rng, 30, WORLD_W - 30);
            const y = randRange(rng, 30, WORLD_H - 30);
            const b = this.biomeAt(x, y);
            const chance = b === 'forest' ? 0.9 : b === 'grass' ? 0.3 : b === 'snow' ? 0.35 : 0.03;
            if (rng() > chance) continue;
            if (!free(x, y, 58)) continue;
            push('tree', x, y);
        }
        for (let i = 0; i < 50000; i++) {
            const x = randRange(rng, 30, WORLD_W - 30);
            const y = randRange(rng, 30, WORLD_H - 30);
            if (!free(x, y, 76)) continue;
            const roll = rng();
            push(roll < 0.6 ? 'stone_node' : roll < 0.83 ? 'metal_node' : 'sulfur_node', x, y);
        }
        for (let i = 0; i < 42000; i++) {
            const x = randRange(rng, 30, WORLD_W - 30);
            const y = randRange(rng, 30, WORLD_H - 30);
            const b = this.biomeAt(x, y);
            if (b !== 'grass' && b !== 'forest') continue;
            if (!free(x, y, 52)) continue;
            push('nettle', x, y);
        }
    }

    /**
     * Everyone washes up on sand, anywhere on the island. Collected once at
     * worldgen so a respawn can pick a different stretch of the same coast.
     */
    private pickSpawn(rng: () => number): void {
        this.beach = [];
        for (let by = 3; by < BIOME_H - 3; by++) {
            for (let bx = 3; bx < BIOME_W - 3; bx++) {
                if (this.biomes[by * BIOME_W + bx] !== 'beach') continue;
                this.beach.push({
                    x: bx * BIOME_TILE + BIOME_TILE / 2,
                    y: by * BIOME_TILE + BIOME_TILE / 2,
                });
            }
        }
        if (this.beach.length === 0) {
            // No sand on this seed at all; fall back to any dry land.
            for (let i = 0; i < 500; i++) {
                const x = randRange(rng, 400, WORLD_W - 400);
                const y = randRange(rng, 400, WORLD_H - 400);
                if (this.biomeAt(x, y) === 'water') continue;
                this.beach.push({ x, y });
                break;
            }
        }
        this.spawn = this.beach[Math.floor(rng() * this.beach.length)] ?? {
            x: WORLD_W / 2,
            y: WORLD_H / 2,
        };
    }

    /**
     * A random stretch of beach that nobody has claimed. `blocked` lets the
     * caller reject anything inside a tool cupboard's radius, so you never wake
     * up inside somebody's base.
     */
    pickBeachSpawn(blocked?: (x: number, y: number) => boolean): { x: number; y: number } {
        if (this.beach.length === 0) return this.spawn;
        for (let tries = 0; tries < 120; tries++) {
            const spot = this.beach[Math.floor(Math.random() * this.beach.length)];
            const x = spot.x + randRange(Math.random, -40, 40);
            const y = spot.y + randRange(Math.random, -40, 40);
            if (this.biomeAt(x, y) === 'water') continue;
            if (blocked?.(x, y)) continue;
            return { x, y };
        }
        return this.spawn;
    }

    // --------------------------------------------------------------- queries

    nodeById_(id: number): ResourceNode | undefined {
        return this.nodeById.get(id);
    }

    nodesNear(x: number, y: number, radius: number): ResourceNode[] {
        return this.nodeHash.query(x, y, radius, this.scratch);
    }

    /**
     * The first live node standing on a spot, if any. Building asks this before
     * it lets anything go up, so nothing is ever raised through a tree.
     */
    naturalAt(x: number, y: number, radius: number): ResourceNode | null {
        for (const n of this.nodesNear(x, y, radius + 24)) {
            if (n.hp <= 0) continue;
            if (dist(x, y, n.x, n.y) < radius + n.radius * 0.8) return n;
        }
        return null;
    }

    /** Clear the ground: knock out every live node in a box. */
    clearNaturalIn(x0: number, y0: number, x1: number, y1: number): number {
        const found: ResourceNode[] = [];
        this.nodesInRect(x0, y0, x1, y1, found);
        let cleared = 0;
        for (const n of found) {
            if (n.hp <= 0) continue;
            n.hp = 0;
            // Normal respawn timer, not a permanent deletion: `respawnBlocked` keeps
            // it down while a building stands there, and lets the land grow back if
            // the building ever comes down.
            n.respawn = regrowthSeconds();
            cleared++;
        }
        return cleared;
    }

    nodesInRect(
        x0: number,
        y0: number,
        x1: number,
        y1: number,
        out: ResourceNode[],
    ): ResourceNode[] {
        return this.nodeHash.rect(x0, y0, x1, y1, out);
    }

    monumentAt(x: number, y: number): { monument: Monument; def: MonumentDef; t: number } | null {
        for (const m of this.monuments) {
            const d = dist(x, y, m.x, m.y);
            if (d > m.radius) continue;
            const def = MONUMENTS.find((z) => z.id === m.defId);
            if (!def) continue;
            return { monument: m, def, t: 1 - d / m.radius };
        }
        return null;
    }

    /** Radiation per second at a point, from whichever monument covers it. */
    radiationAt(x: number, y: number): number {
        const hit = this.monumentAt(x, y);
        if (!hit || hit.def.rads <= 0) return 0;
        return hit.def.rads * Math.min(1, hit.t * 1.6);
    }

    /** Would this node, at its full size, overlap anything built? */
    private blockedForNode(n: ResourceNode): boolean {
        if (!this.respawnBlocked) return false;
        const r = n.radius;
        if (this.respawnBlocked(n.x, n.y)) return true;
        for (const [dx, dy] of [
            [-r, 0],
            [r, 0],
            [0, -r],
            [0, r],
        ] as [number, number][]) {
            if (this.respawnBlocked(n.x + dx, n.y + dy)) return true;
        }
        return false;
    }

    update(dt: number): void {
        for (const n of this.nodes) {
            if (n.shake > 0) n.shake -= dt;
            if (n.respawn > 0) {
                n.respawn -= dt;
                if (n.respawn > 0) continue;
                // Check again later rather than losing the node for good, in case the
                // base standing on it gets levelled.
                // Its trunk, not just its centre: a tree that grew back half over
                // somebody's floor still looks like a tree in their living room.
                if (this.blockedForNode(n)) {
                    n.respawn = REGROWTH.blockedRetrySeconds;
                    continue;
                }
                n.hp = n.maxHp;
            }
        }
        for (const c of this.crates) {
            if (!c.looted) continue;
            c.respawn -= dt;
            if (c.respawn > 0) continue;
            if (this.respawnBlocked?.(c.x, c.y)) {
                c.respawn = REGROWTH.blockedRetrySeconds;
                continue;
            }
            this.fillCrate(c);
        }
    }

    /** Push a circle out of solid nodes and the world edge. */
    clampToWorld(x: number, y: number, radius: number): { x: number; y: number } {
        for (const n of this.nodesNear(x, y, radius + 40)) {
            if (n.hp <= 0 || n.kind === 'nettle') continue;
            const d = dist(x, y, n.x, n.y);
            const min = radius + n.radius * 0.55;
            if (d < min && d > 0.0001) {
                const push = (min - d) / d;
                x += (x - n.x) * push;
                y += (y - n.y) * push;
            }
        }
        return {
            x: Math.max(radius, Math.min(WORLD_W - radius, x)),
            y: Math.max(radius, Math.min(WORLD_H - radius, y)),
        };
    }
}
