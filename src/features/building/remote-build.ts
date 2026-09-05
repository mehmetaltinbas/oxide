import { BuildSystem } from 'src/features/building/building';
import { CELL } from 'src/features/building/constants/cell.constant';
import { BuildKind } from 'src/features/building/types/build-kind.type';
import { BuildTier } from 'src/features/building/types/build-tier.type';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { DeployableKind } from 'src/features/building/types/deployable-kind.type';
import { EdgeSide } from 'src/features/building/types/edge-side.type';
import { Structure } from 'src/features/building/types/structure.interface';
import { NetDeployable } from 'src/features/net/types/net-deployable.interface';
import { NetStructure } from 'src/features/net/types/net-structure.interface';

/**
 * The room's building, put on the local map.
 *
 * Online, the server owns what is built and everybody's client is told about
 * it. This is the one place that turns those wire pieces into the same
 * `Structure` and `Deployable` records the single-player game uses, so the rest
 * of the game cannot tell the difference between a wall you put up and a wall
 * somebody across the island put up.
 *
 * Owners arrive as client ids, which are strings, and everything local keys
 * ownership by a number: 0 is you, anything else is somebody else. The mapping
 * is kept here and nowhere else.
 */
export class RemoteBuild {
    private owners = new Map<string, number>();
    private nextOwner = 1;

    constructor(
        private build: BuildSystem,
        private you: () => string,
    ) {}

    /** Forget the room. Called when leaving, so a new room starts clean. */
    reset(): void {
        this.owners.clear();
        this.nextOwner = 1;
    }

    /** The whole room at once, on join. */
    applySnapshot(structures: NetStructure[], deployables: NetDeployable[]): void {
        this.build.structures = structures.map((s) => this.toStructure(s));
        this.build.deployables = deployables.map((d) => this.toDeployable(d));
        this.build.reindex();
    }

    /** One piece, as it goes up. */
    applyBuilt(s: NetStructure): void {
        const existing = this.build.structures.find((x) => x.id === s.id);
        if (existing) {
            // A doorway that grew a door keeps its id, so this is an edit
            // rather than a new piece.
            Object.assign(existing, this.toStructure(s));
            this.build.reindex();
            return;
        }
        // Your own piece went down the instant you clicked, under an id this
        // client made up. This is the same piece coming back with the server's
        // id on it, so the local one has to go or the wall exists twice.
        this.dropLocalAt(s.kind, s.gx, s.gy, s.side as EdgeSide | undefined);
        this.build.structures.push(this.toStructure(s));
        this.build.reindex();
    }

    applyDeployed(d: NetDeployable): void {
        if (this.build.deployables.some((x) => x.id === d.id)) return;
        const at = RemoteBuild.cellOf(d);
        this.build.deployables = this.build.deployables.filter(
            (x) => Math.floor(x.x / CELL) !== at.gx || Math.floor(x.y / CELL) !== at.gy,
        );
        this.build.deployables.push(this.toDeployable(d));
        this.build.reindex();
    }

    /**
     * The server would not have it. Take the optimistic piece back off the map.
     */
    applyRefused(kind: string, gx: number, gy: number, side?: EdgeSide): void {
        this.dropLocalAt(kind, gx, gy, side);
        this.build.reindex();
    }

    /** Remove whatever this client put in a slot on its own say-so. */
    private dropLocalAt(kind: string, gx: number, gy: number, side?: EdgeSide): void {
        if (kind === 'foundation') {
            this.build.structures = this.build.structures.filter(
                (x) => !(x.kind === 'foundation' && x.gx === gx && x.gy === gy),
            );
            return;
        }
        if (side) {
            this.build.structures = this.build.structures.filter(
                (x) => !(x.side === side && x.gx === gx && x.gy === gy),
            );
            return;
        }
        this.build.deployables = this.build.deployables.filter(
            (x) => Math.floor(x.x / CELL) !== gx || Math.floor(x.y / CELL) !== gy,
        );
    }

    applyDestroyed(id: number): void {
        this.build.structures = this.build.structures.filter((s) => s.id !== id);
        this.build.deployables = this.build.deployables.filter((d) => d.id !== id);
        this.build.reindex();
    }

    applyDoor(id: number, open: boolean): void {
        const s = this.build.structures.find((x) => x.id === id);
        if (s) s.open = open;
    }

    private ownerOf(id: string): number {
        if (id === this.you()) return 0;
        let owner = this.owners.get(id);
        if (owner === undefined) {
            owner = this.nextOwner++;
            this.owners.set(id, owner);
        }
        return owner;
    }

    private toStructure(s: NetStructure): Structure {
        return {
            id: s.id,
            kind: s.kind as BuildKind,
            tier: s.tier as BuildTier,
            gx: s.gx,
            gy: s.gy,
            side: s.side as EdgeSide | undefined,
            hp: s.hp,
            maxHp: s.maxHp,
            owner: this.ownerOf(s.owner),
            open: s.open,
            // The soft side is a melee rule, and the server has no combat yet.
            // Zero reads as "no soft side", which is the safe answer until it
            // does: nothing gets a bonus for hitting the wrong face.
            softX: 0,
            softY: 0,
            flash: 0,
        };
    }

    private toDeployable(d: NetDeployable): Deployable {
        return {
            id: d.id,
            kind: d.kind as DeployableKind,
            x: d.x,
            y: d.y,
            hp: d.hp,
            maxHp: d.hp,
            owner: this.ownerOf(d.owner),
            flash: 0,
        };
    }

    /** The cell a deployable stands in, for callers that think in cells. */
    static cellOf(d: NetDeployable): { gx: number; gy: number } {
        return { gx: Math.floor(d.x / CELL), gy: Math.floor(d.y / CELL) };
    }
}
