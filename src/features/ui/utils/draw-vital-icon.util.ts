import { Vital } from 'src/features/ui/types/vital.type';
import { INK } from 'src/shared/design/constants/ink.constant';

/**
 * The picture at the head of a vital gauge: a heart, a drumstick, a drop.
 *
 * A word under each bar was one more thing to read in a corner you only
 * glance at. Drawn in a unit square scaled to `size`, and inked like the item
 * icons, so the gauges sit in the same printed style as everything else.
 */
export function drawVitalIcon(
    ctx: CanvasRenderingContext2D,
    vital: Vital,
    cx: number,
    cy: number,
    size: number,
    color: string,
): void {
    ctx.save();
    ctx.translate(cx, cy);
    ctx.scale(size, size);
    ctx.beginPath();
    if (vital === 'health') {
        // Two lobes and a point.
        ctx.moveTo(0, 0.38);
        ctx.bezierCurveTo(-0.5, 0.02, -0.46, -0.42, -0.2, -0.4);
        ctx.bezierCurveTo(-0.08, -0.4, 0, -0.3, 0, -0.2);
        ctx.bezierCurveTo(0, -0.3, 0.08, -0.4, 0.2, -0.4);
        ctx.bezierCurveTo(0.46, -0.42, 0.5, 0.02, 0, 0.38);
    } else if (vital === 'food') {
        // A drumstick: the meat, then the bone sticking out of it.
        ctx.ellipse(-0.1, -0.08, 0.3, 0.24, -0.7, 0, Math.PI * 2);
    } else {
        // A falling drop.
        ctx.moveTo(0, -0.44);
        ctx.bezierCurveTo(0.12, -0.2, 0.34, 0.02, 0.3, 0.18);
        ctx.bezierCurveTo(0.26, 0.4, -0.26, 0.4, -0.3, 0.18);
        ctx.bezierCurveTo(-0.34, 0.02, -0.12, -0.2, 0, -0.44);
    }
    ctx.closePath();
    ctx.fillStyle = color;
    ctx.fill();
    const pen = INK.uiWidth / size;
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = pen;
    ctx.lineJoin = 'round';
    ctx.stroke();

    if (vital === 'food') {
        // The bone, drawn after the meat so it pokes out of it.
        ctx.beginPath();
        ctx.moveTo(0.1, 0.12);
        ctx.lineTo(0.3, 0.32);
        ctx.lineCap = 'round';
        ctx.lineWidth = 0.12 + pen * 2;
        ctx.strokeStyle = INK.line;
        ctx.stroke();
        ctx.lineWidth = 0.12;
        ctx.strokeStyle = '#f2ead8';
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(0.34, 0.36, 0.08, 0, Math.PI * 2);
        ctx.fillStyle = '#f2ead8';
        ctx.fill();
        ctx.lineWidth = pen;
        ctx.strokeStyle = INK.line;
        ctx.stroke();
    }
    ctx.restore();
}
