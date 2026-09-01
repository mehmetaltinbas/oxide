import { clamp } from 'src/shared/utils/clamp.util';
import { lerp } from 'src/shared/utils/lerp.util';

export class Camera {
    x = 0;
    y = 0;
    zoom = 1;
    private shakeTime = 0;
    private shakeMag = 0;
    private shakeX = 0;
    private shakeY = 0;

    constructor(
        public viewW: number,
        public viewH: number,
    ) {}

    resize(w: number, h: number): void {
        this.viewW = w;
        this.viewH = h;
    }

    follow(tx: number, ty: number, worldW: number, worldH: number, dt: number): void {
        const halfW = this.viewW / (2 * this.zoom);
        const halfH = this.viewH / (2 * this.zoom);
        const t = 1 - Math.pow(0.001, dt);
        this.x = lerp(this.x, tx, t);
        this.y = lerp(this.y, ty, t);
        // Never show the void past the world edge, unless the view is wider than the world.
        this.x = worldW > halfW * 2 ? clamp(this.x, halfW, worldW - halfW) : worldW / 2;
        this.y = worldH > halfH * 2 ? clamp(this.y, halfH, worldH - halfH) : worldH / 2;
    }

    shake(magnitude: number, duration = 0.25): void {
        if (magnitude > this.shakeMag || this.shakeTime <= 0) {
            this.shakeMag = magnitude;
            this.shakeTime = duration;
        }
    }

    update(dt: number): void {
        if (this.shakeTime > 0) {
            this.shakeTime -= dt;
            const falloff = Math.max(0, this.shakeTime) / 0.25;
            const m = this.shakeMag * falloff;
            this.shakeX = (Math.random() * 2 - 1) * m;
            this.shakeY = (Math.random() * 2 - 1) * m;
        } else {
            this.shakeX = 0;
            this.shakeY = 0;
            this.shakeMag = 0;
        }
    }

    /** Apply the world transform. Caller is responsible for save()/restore(). */
    applyTransform(ctx: CanvasRenderingContext2D): void {
        ctx.translate(this.viewW / 2, this.viewH / 2);
        ctx.scale(this.zoom, this.zoom);
        ctx.translate(-this.x + this.shakeX, -this.y + this.shakeY);
    }

    screenToWorld(sx: number, sy: number): { x: number; y: number } {
        return {
            x: (sx - this.viewW / 2) / this.zoom + this.x,
            y: (sy - this.viewH / 2) / this.zoom + this.y,
        };
    }

    worldToScreen(wx: number, wy: number): { x: number; y: number } {
        return {
            x: (wx - this.x) * this.zoom + this.viewW / 2,
            y: (wy - this.y) * this.zoom + this.viewH / 2,
        };
    }

    /** World-space rectangle currently visible, padded for off-screen culling slack. */
    viewBounds(pad = 64): { x0: number; y0: number; x1: number; y1: number } {
        const halfW = this.viewW / (2 * this.zoom) + pad;
        const halfH = this.viewH / (2 * this.zoom) + pad;
        return { x0: this.x - halfW, y0: this.y - halfH, x1: this.x + halfW, y1: this.y + halfH };
    }
}
