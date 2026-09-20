import { INK } from 'src/shared/design/constants/ink.constant';

/**
 * The printed textures: a dot screen and a hatch.
 *
 * Shading on a comic page is not a gradient, it is ink applied in a pattern
 * dense enough to read as darker. Both are built once into small offscreen
 * tiles and used as repeating fills, which is also the only way this is
 * affordable: a dot screen drawn dot by dot over open ground is thousands of
 * paths a frame, and a tile is one.
 *
 * The tiles are in world units and the camera does not zoom, so a dot is the
 * size it looks.
 */
export class ComicTexture {
    private halftonePattern: CanvasPattern | null = null;
    private hatchPattern: CanvasPattern | null = null;

    constructor(private ctx: CanvasRenderingContext2D) {}

    /** An even dot screen, for the lighter half of shading. */
    halftone(): CanvasPattern | null {
        if (!this.halftonePattern) this.halftonePattern = this.buildHalftone();
        return this.halftonePattern;
    }

    /** Diagonal lines, for the darker half and for anything in shadow. */
    hatch(): CanvasPattern | null {
        if (!this.hatchPattern) this.hatchPattern = this.buildHatch();
        return this.hatchPattern;
    }

    private buildHalftone(): CanvasPattern | null {
        const size = INK.halftoneSpacing;
        const tile = document.createElement('canvas');
        tile.width = size;
        tile.height = size;
        const t = tile.getContext('2d');
        if (!t) return null;
        t.fillStyle = INK.halftoneColor;
        // Two dots offset by half a tile, so the grid reads as a screen rather
        // than as rows and columns.
        t.beginPath();
        t.arc(size * 0.25, size * 0.25, INK.halftoneDot, 0, Math.PI * 2);
        t.arc(size * 0.75, size * 0.75, INK.halftoneDot, 0, Math.PI * 2);
        t.fill();
        return this.ctx.createPattern(tile, 'repeat');
    }

    private buildHatch(): CanvasPattern | null {
        const size = INK.hatchSpacing;
        const tile = document.createElement('canvas');
        tile.width = size;
        tile.height = size;
        const t = tile.getContext('2d');
        if (!t) return null;
        t.strokeStyle = INK.hatchColor;
        t.lineWidth = INK.hatchWidth;
        // One corner-to-corner stroke, plus the two half strokes that make it
        // continue across the seam.
        t.beginPath();
        t.moveTo(-size, size * 2);
        t.lineTo(size * 2, -size);
        t.stroke();
        return this.ctx.createPattern(tile, 'repeat');
    }
}
