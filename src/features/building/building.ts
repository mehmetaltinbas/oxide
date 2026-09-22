import BUILD_NUMBERS from 'shared/building.json';
import { CELL } from 'src/features/building/constants/cell.constant';
import { DOOR_HP_MUL } from 'src/features/building/constants/door-hp-mul.constant';
import { FOUNDATION_HP_MUL } from 'src/features/building/constants/foundation-hp-mul.constant';
import { GRID_H } from 'src/features/building/constants/grid-h.constant';
import { GRID_W } from 'src/features/building/constants/grid-w.constant';
import { HARD_SIDE_MELEE_MUL } from 'src/features/building/constants/hard-side-melee-mul.constant';
import { TC_RADIUS } from 'src/features/building/constants/tc-radius.constant';
import { TIER_DEFS } from 'src/features/building/constants/tier-defs.constant';
import { TIERS } from 'src/features/building/constants/tiers.constant';
import { BuildKind } from 'src/features/building/types/build-kind.type';
import { BuildTier } from 'src/features/building/types/build-tier.type';
import { SLEEPING_BAG_SPACING } from 'src/features/building/constants/sleeping-bag-spacing.constant';
import { DeployableKind } from 'src/features/building/types/deployable-kind.type';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { EdgeSide } from 'src/features/building/types/edge-side.type';
import { Structure } from 'src/features/building/types/structure.interface';
import { centerOf } from 'src/features/building/utils/center-of.util';
import { closestOnSegment } from 'src/features/building/utils/closest-on-segment.util';
import { edgeCells } from 'src/features/building/utils/edge-cells.util';
import { edgeCenter } from 'src/features/building/utils/edge-center.util';
import { edgeSegment } from 'src/features/building/utils/edge-segment.util';
import { clamp } from 'src/shared/utils/clamp.util';
import { dist } from 'src/shared/utils/dist.util';

const WALL_THICKNESS = BUILD_NUMBERS.wallThickness;
/** Half-width of a deployable's solid box. Shared: the server pushes players out of these too. */
const DEPLOY_HALF = BUILD_NUMBERS.deployHalf;

function cellKey(gx: number, gy: number): string {
    return `${gx},${gy}`;
}

function edgeKey(gx: number, gy: number, side: EdgeSide): string {
    return `${gx},${gy},${side}`;
}

export class BuildSystem {
    structures: Structure[] = [];
    deployables: Deployable[] = [];

    private foundations = new Map<string, Structure>();
    private edges = new Map<string, Structure>();
    private deployCells = new Map<string, Deployable>();
    private nextId = 1;

    setNextId(id: number): void {
        this.nextId = id;
    }

    get nextIdValue(): number {
        return this.nextId;
    }

    // ------------------------------------------------------------- accessors

    foundationAt(gx: number, gy: number): Structure | undefined {
        return this.foundations.get(cellKey(gx, gy));
    }

    edgeAt(gx: number, gy: number, side: EdgeSide): Structure | undefined {
        return this.edges.get(edgeKey(gx, gy, side));
    }

    deployableAt(gx: number, gy: number): Deployable | undefined {
        return this.deployCells.get(cellKey(gx, gy));
    }

    byId(id: number): Structure | Deployable | undefined {
        return (
            this.structures.find((s) => s.id === id) ?? this.deployables.find((d) => d.id === id)
        );
    }

    /** Tool cupboards define who may build where. */
    toolCupboards(): Deployable[] {
        return this.deployables.filter((d) => d.kind === 'tool_cupboard');
    }

    /**
     * Who owns the ground at a point: an owner id, or null when it is unclaimed.
     * Nearest cupboard wins so overlapping claims resolve consistently.
     */
    privilegeAt(x: number, y: number): number | null {
        let best: Deployable | null = null;
        let bestD = TC_RADIUS;
        for (const tc of this.toolCupboards()) {
            const d = dist(x, y, tc.x, tc.y);
            if (d < bestD) {
                bestD = d;
                best = tc;
            }
        }
        return best ? best.owner : null;
    }

    canBuildAt(x: number, y: number, owner: number): boolean {
        const claim = this.privilegeAt(x, y);
        return claim === null || claim === owner;
    }

    // -------------------------------------------------------------- placement

    /** Whether a foundation may go on this cell, and why not if it may not. */
    /**
     * Set by the game: is there a tree, rock or cotton still standing on this
     * spot? Nothing gets built on top of natural cover: you clear the ground
     * first, the same as anyone would. The build system does not know the world
     * exists, so this arrives as a hook, like `World.respawnBlocked`.
     */
    naturalBlocked: ((x: number, y: number, radius: number) => string | null) | null = null;

    /**
     * Set by the game: is this point on monument ground?
     *
     * Monuments are the looting locations, and they belong to everybody. Walling
     * one in would make a shared landmark somebody's private stash, so the ban
     * covers the approach as well as the ground itself. Same hook shape as
     * `naturalBlocked`, for the same reason: the build system does not know the
     * world exists.
     */
    monumentBlocked: ((x: number, y: number) => string | null) | null = null;

    canPlaceFoundation(gx: number, gy: number, owner: number): string | null {
        if (gx < 1 || gy < 1 || gx >= GRID_W - 1 || gy >= GRID_H - 1) return 'Outside the map';
        if (this.foundations.has(cellKey(gx, gy))) return 'Already a foundation here';
        const cx = gx * CELL + CELL / 2;
        const cy = gy * CELL + CELL / 2;
        if (!this.canBuildAt(cx, cy, owner)) return 'Building blocked by a tool cupboard';
        const monument = this.monumentBlocked?.(cx, cy);
        if (monument) return monument;
        const natural = this.naturalBlocked?.(cx, cy, CELL * 0.5);
        if (natural) return natural;
        return null;
    }

    /**
     * Walls need something to stand on: at least one of the two cells the edge
     * separates must already be your foundation.
     */
    canPlaceEdge(
        gx: number,
        gy: number,
        side: EdgeSide,
        owner: number,
        kind: BuildKind,
    ): string | null {
        if (gx < 1 || gy < 1 || gx >= GRID_W - 1 || gy >= GRID_H - 1) return 'Outside the map';
        const existing = this.edges.get(edgeKey(gx, gy, side));
        if (kind === 'door') {
            if (!existing) return 'Doors go in a doorway';
            if (existing.kind !== 'doorway') return 'That is not a doorway';
            if (existing.owner !== owner) return 'Not your doorway';
            return null;
        }
        if (existing) return 'Already something on this edge';
        const [[ax, ay], [bx, by]] = edgeCells(gx, gy, side);
        const a = this.foundationAt(ax, ay);
        const b = this.foundationAt(bx, by);
        const supported = (a && a.owner === owner) || (b && b.owner === owner);
        if (!supported) return 'Needs a foundation beside it';
        const c = edgeCenter(gx, gy, side);
        if (!this.canBuildAt(c.x, c.y, owner)) return 'Building blocked by a tool cupboard';
        const monument = this.monumentBlocked?.(c.x, c.y);
        if (monument) return monument;
        const natural = this.naturalBlocked?.(c.x, c.y, CELL * 0.25);
        if (natural) return natural;
        return null;
    }

    placeFoundation(gx: number, gy: number, owner: number, tier: BuildTier = 'twig'): Structure {
        const def = TIER_DEFS[tier];
        const hp = Math.round(def.hp * FOUNDATION_HP_MUL);
        const s: Structure = {
            id: this.nextId++,
            kind: 'foundation',
            tier,
            gx,
            gy,
            hp,
            maxHp: hp,
            owner,
            softX: 0,
            softY: 0,
            flash: 0,
        };
        this.structures.push(s);
        this.foundations.set(cellKey(gx, gy), s);
        this.markLayoutChanged();
        return s;
    }

    placeEdge(
        gx: number,
        gy: number,
        side: EdgeSide,
        kind: 'wall' | 'doorway',
        owner: number,
        softFrom: { x: number; y: number },
        tier: BuildTier = 'twig',
    ): Structure {
        const def = TIER_DEFS[tier];
        const c = edgeCenter(gx, gy, side);
        // The side the builder stood on becomes the soft side, as in Rust.
        const dx = softFrom.x - c.x;
        const dy = softFrom.y - c.y;
        const len = Math.hypot(dx, dy) || 1;
        const s: Structure = {
            id: this.nextId++,
            kind,
            tier,
            gx,
            gy,
            side,
            hp: def.hp,
            maxHp: def.hp,
            owner,
            softX: dx / len,
            softY: dy / len,
            flash: 0,
        };
        this.structures.push(s);
        this.edges.set(edgeKey(gx, gy, side), s);
        this.markLayoutChanged();
        return s;
    }

    /** Turn an existing doorway into a door. */
    installDoor(doorway: Structure, owner: number): Structure {
        const def = TIER_DEFS[doorway.tier];
        doorway.kind = 'door';
        doorway.open = false;
        doorway.locked = false;
        doorway.owner = owner;
        doorway.maxHp = Math.round(def.hp * DOOR_HP_MUL);
        doorway.hp = doorway.maxHp;
        this.markLayoutChanged();
        return doorway;
    }

    upgrade(s: Structure, tier: BuildTier): void {
        const def = TIER_DEFS[tier];
        const ratio = s.hp / s.maxHp;
        s.tier = tier;
        s.maxHp = Math.round(
            def.hp *
                (s.kind === 'foundation' ? FOUNDATION_HP_MUL : s.kind === 'door' ? DOOR_HP_MUL : 1),
        );
        s.hp = Math.max(1, Math.round(s.maxHp * ratio));
    }

    nextTier(s: Structure): BuildTier | null {
        const i = TIERS.indexOf(s.tier);
        return i >= 0 && i < TIERS.length - 1 ? TIERS[i + 1] : null;
    }

    // ------------------------------------------------------------ deployables

    canDeploy(gx: number, gy: number, owner: number, kind?: DeployableKind): string | null {
        if (this.deployCells.has(cellKey(gx, gy))) return 'Something is already here';
        const cx = gx * CELL + CELL / 2;
        const cy = gy * CELL + CELL / 2;
        if (kind === 'sleeping_bag') {
            // Anyone's bag, not only yours: a spot has room for one.
            for (const d of this.deployables) {
                if (d.kind !== 'sleeping_bag') continue;
                if (Math.hypot(d.x - cx, d.y - cy) < SLEEPING_BAG_SPACING)
                    return 'Too close to another sleeping bag';
            }
        }
        if (!this.canBuildAt(cx, cy, owner)) return 'Blocked by a tool cupboard';
        const natural = this.naturalBlocked?.(cx, cy, CELL * 0.4);
        if (natural) return natural;
        return null;
    }

    deploy(kind: DeployableKind, gx: number, gy: number, owner: number, hp = 300): Deployable {
        const d: Deployable = {
            id: this.nextId++,
            kind,
            x: gx * CELL + CELL / 2,
            y: gy * CELL + CELL / 2,
            hp,
            maxHp: hp,
            owner,
            flash: 0,
        };
        if (kind === 'wooden_box') d.container = { slots: new Array(12).fill(null) };
        if (kind === 'furnace') {
            d.container = { slots: new Array(6).fill(null) };
            d.lit = false;
            d.fuel = 0;
            d.progress = 0;
        }
        if (kind === 'campfire') {
            d.container = { slots: new Array(4).fill(null) };
            d.lit = false;
            d.fuel = 0;
            d.progress = 0;
        }
        this.deployables.push(d);
        this.deployCells.set(cellKey(gx, gy), d);
        this.markLayoutChanged();
        return d;
    }

    removeDeployable(d: Deployable): void {
        this.markLayoutChanged();
        const i = this.deployables.indexOf(d);
        if (i >= 0) this.deployables.splice(i, 1);
        for (const [key, value] of this.deployCells) {
            if (value === d) {
                this.deployCells.delete(key);
                break;
            }
        }
    }

    removeStructure(s: Structure): void {
        this.markLayoutChanged();
        const i = this.structures.indexOf(s);
        if (i >= 0) this.structures.splice(i, 1);
        if (s.kind === 'foundation') this.foundations.delete(cellKey(s.gx, s.gy));
        else if (s.side) this.edges.delete(edgeKey(s.gx, s.gy, s.side));
    }

    /** Rebuild the lookup maps after loading a save. */
    reindex(): void {
        this.markLayoutChanged();
        this.foundations.clear();
        this.edges.clear();
        this.deployCells.clear();
        for (const s of this.structures) {
            if (s.kind === 'foundation') this.foundations.set(cellKey(s.gx, s.gy), s);
            else if (s.side) this.edges.set(edgeKey(s.gx, s.gy, s.side), s);
        }
        for (const d of this.deployables) {
            this.deployCells.set(cellKey(Math.floor(d.x / CELL), Math.floor(d.y / CELL)), d);
        }
        let max = 1;
        for (const s of this.structures) max = Math.max(max, s.id + 1);
        for (const d of this.deployables) max = Math.max(max, d.id + 1);
        this.nextId = max;
    }

    // --------------------------------------------------------------- damage

    /**
     * Apply damage. Melee barely scratches the hard side of a wall, which is why
     * base layouts put the soft side inward.
     */
    damage(
        target: Structure | Deployable,
        amount: number,
        fromX: number,
        fromY: number,
        melee: boolean,
    ): number {
        let dealt = amount;
        if (melee && 'softX' in target && target.kind !== 'foundation') {
            const dx = fromX - centerOf(target).x;
            const dy = fromY - centerOf(target).y;
            const len = Math.hypot(dx, dy) || 1;
            const dot = (dx / len) * target.softX + (dy / len) * target.softY;
            if (dot < 0.15) dealt *= HARD_SIDE_MELEE_MUL;
        }
        target.hp -= dealt;
        target.flash = 0.14;
        return dealt;
    }

    /** Everything a blast can touch inside a radius. */
    inBlast(x: number, y: number, radius: number): (Structure | Deployable)[] {
        const out: (Structure | Deployable)[] = [];
        for (const s of this.structures) {
            if (dist(x, y, centerOf(s).x, centerOf(s).y) <= radius + CELL * 0.4) out.push(s);
        }
        for (const d of this.deployables) {
            if (dist(x, y, d.x, d.y) <= radius + 20) out.push(d);
        }
        return out;
    }

    // ------------------------------------------------------------- collision

    /**
     * Slide a circle against walls and closed doors. `owner` may pass through
     * their own open doors; nobody walks through a closed one.
     */
    resolve(x: number, y: number, radius: number): { x: number; y: number } {
        const g0x = Math.floor((x - radius - CELL) / CELL);
        const g1x = Math.floor((x + radius + CELL) / CELL);
        const g0y = Math.floor((y - radius - CELL) / CELL);
        const g1y = Math.floor((y + radius + CELL) / CELL);

        for (let gy = g0y; gy <= g1y; gy++) {
            for (let gx = g0x; gx <= g1x; gx++) {
                for (const side of ['n', 'w'] as EdgeSide[]) {
                    const s = this.edges.get(edgeKey(gx, gy, side));
                    if (!s) continue;
                    if (s.kind === 'doorway') continue;
                    if (s.kind === 'door' && s.open) continue;
                    const [ax, ay, bx, by] = edgeSegment(gx, gy, side);
                    const hit = closestOnSegment(x, y, ax, ay, bx, by);
                    const d = dist(x, y, hit.x, hit.y);
                    const min = radius + WALL_THICKNESS;
                    if (d < min && d > 0.0001) {
                        const push = (min - d) / d;
                        x += (x - hit.x) * push;
                        y += (y - hit.y) * push;
                    }
                }
                const dep = this.deployCells.get(cellKey(gx, gy));
                if (dep && dep.kind !== 'sleeping_bag') {
                    const half = DEPLOY_HALF;
                    const nx = clamp(x, dep.x - half, dep.x + half);
                    const ny = clamp(y, dep.y - half, dep.y + half);
                    const d = dist(x, y, nx, ny);
                    if (d < radius && d > 0.0001) {
                        const push = (radius - d) / d;
                        x += (x - nx) * push;
                        y += (y - ny) * push;
                    }
                }
            }
        }
        return { x, y };
    }

    /** True when a straight line between two points crosses a solid piece. */
    blocksLine(x0: number, y0: number, x1: number, y1: number): boolean {
        const steps = Math.ceil(dist(x0, y0, x1, y1) / (CELL * 0.4));
        for (let i = 1; i <= steps; i++) {
            const t = i / steps;
            const px = x0 + (x1 - x0) * t;
            const py = y0 + (y1 - y0) * t;
            const gx = Math.floor(px / CELL);
            const gy = Math.floor(py / CELL);
            for (let ox = -1; ox <= 1; ox++) {
                for (let oy = -1; oy <= 1; oy++) {
                    for (const side of ['n', 'w'] as EdgeSide[]) {
                        const s = this.edges.get(edgeKey(gx + ox, gy + oy, side));
                        if (!s || s.kind === 'doorway' || (s.kind === 'door' && s.open)) continue;
                        const [ax, ay, bx, by] = edgeSegment(gx + ox, gy + oy, side);
                        const hit = closestOnSegment(px, py, ax, ay, bx, by);
                        if (dist(px, py, hit.x, hit.y) < WALL_THICKNESS + 3) return true;
                    }
                }
            }
        }
        return false;
    }

    update(dt: number): void {
        for (const s of this.structures) if (s.flash > 0) s.flash -= dt;
        for (const d of this.deployables) if (d.flash > 0) d.flash -= dt;
    }

    /**
     * Flood filling every room is not cheap, so it only reruns when the layout
     * actually changes: a piece placed, destroyed, or a door swung. Marking it
     * dirty every tick meant paying for it on every single frame.
     */
    private markLayoutChanged(): void {
        this.enclosureDirty = true;
    }

    // ------------------------------------------------------------- enclosure

    private enclosure = new Map<string, number>();
    private enclosureDirty = true;
    private enclosureOwner = new Map<number, number>();

    /**
     * Flood fill the build grid, treating walls and closed doors as boundaries.
     * Cells that cannot reach open ground form a sealed room, and each room is
     * tagged with whoever owns the pieces around it. Used both for interaction
     * rules and for hiding the inside of a building you have no business seeing.
     */
    private rebuildEnclosure(): void {
        this.enclosure.clear();
        this.enclosureOwner.clear();
        this.enclosureDirty = false;
        if (this.structures.length === 0) return;

        // One flood per building, not one for the whole island. Bases sit far
        // apart, so a single box around all of them spans tens of thousands of
        // empty cells and costs over 20ms to fill; a box per clump of touching
        // foundations costs about as much as the buildings themselves.
        let region = 1;
        for (const group of this.foundationClusters()) {
            region = this.floodEnclosure(group, region);
        }
    }

    /** Groups of foundations that touch, each with a padded bounding box. */
    private foundationClusters(): { minX: number; minY: number; maxX: number; maxY: number }[] {
        const seen = new Set<string>();
        const out: { minX: number; minY: number; maxX: number; maxY: number }[] = [];
        for (const [key, f] of this.foundations) {
            if (seen.has(key)) continue;
            seen.add(key);
            let minX = f.gx;
            let maxX = f.gx;
            let minY = f.gy;
            let maxY = f.gy;
            const stack: number[] = [f.gx, f.gy];
            while (stack.length > 0) {
                const cy = stack.pop()!;
                const cx = stack.pop()!;
                if (cx < minX) minX = cx;
                if (cx > maxX) maxX = cx;
                if (cy < minY) minY = cy;
                if (cy > maxY) maxY = cy;
                // Eight-way, so foundations that only touch at a corner still count as
                // one building.
                for (let oy = -1; oy <= 1; oy++) {
                    for (let ox = -1; ox <= 1; ox++) {
                        if (ox === 0 && oy === 0) continue;
                        const nk = cellKey(cx + ox, cy + oy);
                        if (seen.has(nk) || !this.foundations.has(nk)) continue;
                        seen.add(nk);
                        stack.push(cx + ox, cy + oy);
                    }
                }
            }
            // Two cells of pad, so the border ring of the box is guaranteed to be
            // open ground: that ring is where the outside flood starts.
            out.push({ minX: minX - 2, minY: minY - 2, maxX: maxX + 2, maxY: maxY + 2 });
        }
        return out;
    }

    /**
     * Work out which cells of one building are sealed off from the open air, and
     * tag each room with an id.
     *
     * It floods from the outside in, rather than flooding out from every cell
     * and asking "did I escape". That earlier version shared one visited set
     * between floods, so whether a room counted as sealed depended on the order
     * cells happened to be scanned in: a breached room whose surrounding ground
     * had already been visited looked sealed, because the way out was marked
     * seen and never followed. Blowing a wall left the roof on.
     */
    private floodEnclosure(
        box: { minX: number; minY: number; maxX: number; maxY: number },
        firstRegion: number,
    ): number {
        const { minX, minY, maxX, maxY } = box;
        const w = maxX - minX + 1;
        const h = maxY - minY + 1;
        const idx = (gx: number, gy: number): number => (gy - minY) * w + (gx - minX);

        /** Can you step from one cell to its neighbour, or is there a wall between? */
        const open = (cx: number, cy: number, nx: number, ny: number): boolean => {
            const side: EdgeSide = cx === nx ? 'n' : 'w';
            const ex = cx === nx ? cx : Math.max(cx, nx);
            const ey = cx === nx ? Math.max(cy, ny) : cy;
            const blocker = this.edges.get(edgeKey(ex, ey, side));
            if (!blocker) return true;
            return blocker.kind === 'doorway' || (blocker.kind === 'door' && !!blocker.open);
        };

        // Pass one: everything the open air can walk to. However much wall is
        // nearby, if you can stroll in from outside it is not a room.
        const outside = new Uint8Array(w * h);
        const stack: number[] = [];
        const push = (gx: number, gy: number): void => {
            if (gx < minX || gx > maxX || gy < minY || gy > maxY) return;
            const i = idx(gx, gy);
            if (outside[i]) return;
            outside[i] = 1;
            stack.push(gx, gy);
        };
        for (let gx = minX; gx <= maxX; gx++) {
            push(gx, minY);
            push(gx, maxY);
        }
        for (let gy = minY; gy <= maxY; gy++) {
            push(minX, gy);
            push(maxX, gy);
        }
        while (stack.length > 0) {
            const cy = stack.pop()!;
            const cx = stack.pop()!;
            if (open(cx, cy, cx, cy - 1)) push(cx, cy - 1);
            if (open(cx, cy, cx, cy + 1)) push(cx, cy + 1);
            if (open(cx, cy, cx - 1, cy)) push(cx - 1, cy);
            if (open(cx, cy, cx + 1, cy)) push(cx + 1, cy);
        }

        // Pass two: what is left is sealed. Group it into rooms, so a room is one
        // region however many cells it spans, and a hole anywhere in it opens all
        // of it at once.
        let region = firstRegion;
        const grouped = new Uint8Array(w * h);
        for (let gy = minY; gy <= maxY; gy++) {
            for (let gx = minX; gx <= maxX; gx++) {
                const seed = idx(gx, gy);
                if (outside[seed] || grouped[seed]) continue;
                grouped[seed] = 1;
                const room: number[] = [gx, gy];
                const cells: string[] = [];
                let owner = -1;
                while (room.length > 0) {
                    const cy = room.pop()!;
                    const cx = room.pop()!;
                    cells.push(cellKey(cx, cy));
                    const f = this.foundationAt(cx, cy);
                    if (f && owner === -1) owner = f.owner;
                    for (const [nx, ny] of [
                        [cx, cy - 1],
                        [cx, cy + 1],
                        [cx - 1, cy],
                        [cx + 1, cy],
                    ] as [number, number][]) {
                        if (nx < minX || nx > maxX || ny < minY || ny > maxY) continue;
                        const ni = idx(nx, ny);
                        if (grouped[ni] || outside[ni]) continue;
                        if (!open(cx, cy, nx, ny)) continue;
                        grouped[ni] = 1;
                        room.push(nx, ny);
                    }
                }
                for (const c of cells) this.enclosure.set(c, region);
                this.enclosureOwner.set(region, owner);
                region++;
            }
        }
        return region;
    }

    /** Swinging a door changes what is sealed, so the layout must be re-flooded. */
    setDoorOpen(door: Structure, open: boolean): void {
        door.open = open;
        this.markLayoutChanged();
    }

    /** Which sealed room a world point sits in, or 0 for open ground. */
    regionAt(x: number, y: number): number {
        if (this.enclosureDirty) this.rebuildEnclosure();
        return this.enclosure.get(cellKey(Math.floor(x / CELL), Math.floor(y / CELL))) ?? 0;
    }

    regionOwner(region: number): number {
        if (this.enclosureDirty) this.rebuildEnclosure();
        return this.enclosureOwner.get(region) ?? -1;
    }

    /** Every sealed cell, for the renderer to black out. */
    enclosedCells(): Map<string, number> {
        if (this.enclosureDirty) this.rebuildEnclosure();
        return this.enclosure;
    }
}
