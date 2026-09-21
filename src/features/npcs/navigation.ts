import { BuildSystem } from 'src/features/building/building';
import { CELL } from 'src/features/building/constants/cell.constant';
import { GRID_H } from 'src/features/building/constants/grid-h.constant';
import { GRID_W } from 'src/features/building/constants/grid-w.constant';
import { EdgeSide } from 'src/features/building/types/edge-side.type';
import { PathPoint } from 'src/features/npcs/types/path-point.interface';
import { World } from 'src/features/world/world';

/**
 * Grid pathfinding for survivors.
 *
 * The old steering was a whisker probe that fanned out around obstacles. That
 * works for a fence post and fails completely for a building: an agent trying
 * to reach a tree on the far side of its own compound wall would press into
 * the wall forever, because no local probe can see that the way round is
 * through the door. This is A* over the build grid, where a step between two
 * cells is only legal if the edge between them is not a wall or a shut door: * so a path through a base genuinely routes through its doorway.
 *
 * See docs/systems/ai-survivor.md.
 */

/** Cap on expansions per search, so a hopeless request cannot stall a frame. */
const MAX_EXPANSIONS = 420;
/**
 * Whole-frame budget for path searches across every agent. Without this, a
 * crowd all re-planning on the same frame is a visible hitch: measured at
 * 7.2ms per update before this cap, against a whole-frame budget of ~4ms.
 */
const SEARCHES_PER_FRAME = 3;

/**
 * Upper radius of each cached size class. A body is tested against the top of
 * its class, so the grid is never more optimistic than the collision that
 * follows it: planning a route a body cannot physically take is what leaves an
 * agent grinding along an obstacle instead of walking around it.
 */
const SIZE_CLASSES = [14, 22, 32];

function sizeClass(radius: number): number {
    for (let i = 0; i < SIZE_CLASSES.length; i++) {
        if (radius <= SIZE_CLASSES[i]) return i;
    }
    return SIZE_CLASSES.length - 1;
}

export class Navigator {
    /**
     * Lazily filled cache of which cells an agent can stand in: 0 unknown,
     * 1 free, 2 blocked. Testing this properly means a spatial query per cell,
     * which is far too expensive to redo on every A* expansion.
     */
    /**
     * One cache per size class, because whether a cell is standable depends on
     * how wide the body is. A single shared cache let the first agent to ask
     * decide for everybody, so a narrow one could mark a gap open and every
     * wider one after it read "open" from the cache and walked into the edge
     * of it. Three classes is enough at this cell size.
     */
    private caches = SIZE_CLASSES.map(() => new Uint8Array(GRID_W * GRID_H));
    private searchesThisFrame = 0;

    constructor(
        private world: World,
        private build: BuildSystem,
    ) {}

    /** Called once per simulation step to reopen the per-frame search budget. */
    beginFrame(): void {
        this.searchesThisFrame = 0;
    }

    /** Drop the cache when the world changes shape under it. */
    invalidate(): void {
        for (const cache of this.caches) cache.fill(0);
    }

    /** True when an agent cannot stand in the middle of a cell. */
    private cellBlocked(gx: number, gy: number, radius: number): boolean {
        if (gx < 0 || gy < 0 || gx >= GRID_W || gy >= GRID_H) return true;
        const key = gy * GRID_W + gx;
        const cache = this.caches[sizeClass(radius)];
        const cached = cache[key];
        if (cached !== 0) return cached === 2;

        // Test against the top of the size class rather than the exact body,
        // so every member of a class agrees with the cached answer.
        const r = SIZE_CLASSES[sizeClass(radius)];
        const cx = gx * CELL + CELL / 2;
        const cy = gy * CELL + CELL / 2;
        let blocked = false;
        // The sea. Everything here can swim and nothing chooses to: a route
        // through open water is not a route these people or animals would take,
        // and letting one exist is what had bears crossing bays in a line.
        if (this.world.biomeAt(cx, cy) === 'water') blocked = true;
        else if (this.build.deployableAt(gx, gy)) blocked = true;
        else {
            for (const n of this.world.nodesNear(cx, cy, CELL)) {
                if (n.hp <= 0 || n.kind === 'hemp') continue;
                const dx = cx - n.x;
                const dy = cy - n.y;
                if (dx * dx + dy * dy < (n.radius * 0.55 + r) ** 2) {
                    blocked = true;
                    break;
                }
            }
        }
        cache[key] = blocked ? 2 : 1;
        return blocked;
    }

    /** Can an agent walk from one cell to the neighbour across their shared edge? */
    private edgeOpen(fromX: number, fromY: number, toX: number, toY: number): boolean {
        let gx = fromX;
        let gy = fromY;
        let side: EdgeSide;
        if (toY < fromY) {
            side = 'n';
        } else if (toY > fromY) {
            side = 'n';
            gy = toY;
        } else if (toX < fromX) {
            side = 'w';
        } else {
            side = 'w';
            gx = toX;
        }
        const piece = this.build.edgeAt(gx, gy, side);
        if (!piece) return true;
        if (piece.kind === 'doorway') return true;
        // A shut door is a wall until somebody opens it.
        if (piece.kind === 'door' && piece.open) return true;
        return false;
    }

    /**
     * A* from one world point to another. Returns cell-centre waypoints, or null
     * when no route was found inside the expansion budget.
     */
    findPath(
        fromX: number,
        fromY: number,
        toX: number,
        toY: number,
        radius: number,
    ): PathPoint[] | null {
        const sx = Math.floor(fromX / CELL);
        const sy = Math.floor(fromY / CELL);
        const gx = Math.floor(toX / CELL);
        const gy = Math.floor(toY / CELL);
        if (sx === gx && sy === gy) return [{ x: toX, y: toY }];

        const key = (x: number, y: number): number => y * GRID_W + x;
        const start = key(sx, sy);
        const goal = key(gx, gy);

        if (this.searchesThisFrame >= SEARCHES_PER_FRAME) return null;
        this.searchesThisFrame++;

        const open: number[] = [start];
        // Membership set alongside the frontier: `open.includes` inside the loop
        // made the whole search quadratic.
        const inOpen = new Set<number>([start]);
        const cameFrom = new Map<number, number>();
        const gScore = new Map<number, number>([[start, 0]]);
        const fScore = new Map<number, number>([[start, Math.abs(gx - sx) + Math.abs(gy - sy)]]);
        let expansions = 0;

        while (open.length > 0 && expansions < MAX_EXPANSIONS) {
            // Small frontier, so a linear scan beats the cost of a heap here.
            let bestIndex = 0;
            let best = fScore.get(open[0]) ?? Infinity;
            for (let i = 1; i < open.length; i++) {
                const f = fScore.get(open[i]) ?? Infinity;
                if (f < best) {
                    best = f;
                    bestIndex = i;
                }
            }
            const current = open.splice(bestIndex, 1)[0];
            inOpen.delete(current);
            if (current === goal) return this.rebuild(cameFrom, current, toX, toY);
            expansions++;

            const cx = current % GRID_W;
            const cy = Math.floor(current / GRID_W);
            const neighbours: [number, number][] = [
                [cx, cy - 1],
                [cx, cy + 1],
                [cx - 1, cy],
                [cx + 1, cy],
            ];
            for (const [nx, ny] of neighbours) {
                const nk = key(nx, ny);
                if (nk !== goal && this.cellBlocked(nx, ny, radius)) continue;
                if (!this.edgeOpen(cx, cy, nx, ny)) continue;
                const tentative = (gScore.get(current) ?? Infinity) + 1;
                if (tentative >= (gScore.get(nk) ?? Infinity)) continue;
                cameFrom.set(nk, current);
                gScore.set(nk, tentative);
                fScore.set(nk, tentative + Math.abs(gx - nx) + Math.abs(gy - ny));
                if (!inOpen.has(nk)) {
                    inOpen.add(nk);
                    open.push(nk);
                }
            }
        }
        return null;
    }

    private rebuild(
        cameFrom: Map<number, number>,
        current: number,
        toX: number,
        toY: number,
    ): PathPoint[] {
        const cells: number[] = [current];
        let node = current;
        while (cameFrom.has(node)) {
            node = cameFrom.get(node)!;
            cells.push(node);
        }
        cells.reverse();
        // Drop the cell the agent is already standing in.
        cells.shift();
        const out: PathPoint[] = cells.map((c) => ({
            x: (c % GRID_W) * CELL + CELL / 2,
            y: Math.floor(c / GRID_W) * CELL + CELL / 2,
        }));
        // Finish on the real target rather than the middle of its cell.
        if (out.length > 0) out[out.length - 1] = { x: toX, y: toY };
        else out.push({ x: toX, y: toY });
        return out;
    }
}
