/**
 * Anything worth drawing between two simulation steps.
 *
 * The fields are optional because they are only ever written by the render
 * interpolation and only ever read by it.
 */
export interface RenderLerpable {
    x: number;
    y: number;
    facing: number;
    prevX?: number;
    prevY?: number;
    prevFacing?: number;
}

/**
 * Draw between simulation steps instead of on them.
 *
 * The simulation runs in fixed steps and frames are drawn whenever the browser
 * gets round to it. The two are not locked together, so some frames run no
 * step at all and others run two, and everything on screen lands on step
 * boundaries. That is the judder you see even standing still in single player,
 * and it has nothing to do with the network.
 *
 * The cure is the usual one: remember where everything was at the last step,
 * and draw it part of the way to where it is now, by however far through the
 * current step we are. The renderer is not told any of this. Positions are
 * blended just before it runs and put back immediately after, which keeps
 * `draw` reading exactly what it always read and keeps the simulation the only
 * thing that decides where anything really is.
 */
export class RenderLerp {
    private held: { it: RenderLerpable; x: number; y: number; facing: number }[] = [];

    /** Where everything is now, which is where the next frame blends from. */
    capture(items: RenderLerpable[]): void {
        for (const it of items) {
            it.prevX = it.x;
            it.prevY = it.y;
            it.prevFacing = it.facing;
        }
    }

    /** Blend toward the current step by `alpha`, keeping the truth to put back. */
    apply(items: RenderLerpable[], alpha: number): void {
        const a = alpha < 0 ? 0 : alpha > 1 ? 1 : alpha;
        for (const it of items) {
            if (it.prevX === undefined || it.prevY === undefined) continue;
            this.held.push({ it, x: it.x, y: it.y, facing: it.facing });
            it.x = it.prevX + (it.x - it.prevX) * a;
            it.y = it.prevY + (it.y - it.prevY) * a;
            if (it.prevFacing !== undefined) {
                let d = it.facing - it.prevFacing;
                while (d > Math.PI) d -= Math.PI * 2;
                while (d < -Math.PI) d += Math.PI * 2;
                it.facing = it.prevFacing + d * a;
            }
        }
    }

    /**
     * Put the real positions back.
     *
     * Always called straight after drawing, so nothing else in the game ever
     * sees a blended position: an interpolated body is a picture, not a fact,
     * and letting one reach the simulation would make it drift.
     */
    restore(): void {
        for (const h of this.held) {
            h.it.x = h.x;
            h.it.y = h.y;
            h.it.facing = h.facing;
        }
        this.held.length = 0;
    }
}
