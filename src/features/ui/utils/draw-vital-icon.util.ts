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
    const pen = INK.uiWidth / size;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';

    if (vital === 'food') {
        drawDrumstick(ctx, color, pen);
        ctx.restore();
        return;
    }

    ctx.beginPath();
    if (vital === 'health') {
        // Two lobes and a point.
        ctx.moveTo(0, 0.38);
        ctx.bezierCurveTo(-0.5, 0.02, -0.46, -0.42, -0.2, -0.4);
        ctx.bezierCurveTo(-0.08, -0.4, 0, -0.3, 0, -0.2);
        ctx.bezierCurveTo(0, -0.3, 0.08, -0.4, 0.2, -0.4);
        ctx.bezierCurveTo(0.46, -0.42, 0.5, 0.02, 0, 0.38);
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
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = pen;
    ctx.stroke();
    ctx.restore();
}

/**
 * A cooked leg: a round meaty end tapering into the bone, and the bone ending
 * in the double knob that makes it read as a drumstick rather than a blob.
 * The bone goes down first so the meat sits over the joint.
 */
function drawDrumstick(ctx: CanvasRenderingContext2D, color: string, pen: number): void {
    const bone = '#f2ead8';

    // The shaft, as a thick line with the pen under it.
    ctx.beginPath();
    ctx.moveTo(0.02, 0.04);
    ctx.lineTo(0.27, 0.29);
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = 0.13 + pen * 2;
    ctx.stroke();
    ctx.strokeStyle = bone;
    ctx.lineWidth = 0.13;
    ctx.stroke();

    // The knuckle: two knobs side by side across the end of the shaft.
    for (const [kx, ky] of [
        [0.24, 0.38],
        [0.37, 0.25],
    ] as const) {
        ctx.beginPath();
        ctx.arc(kx, ky, 0.085, 0, Math.PI * 2);
        ctx.fillStyle = bone;
        ctx.fill();
        ctx.strokeStyle = INK.line;
        ctx.lineWidth = pen;
        ctx.stroke();
    }
    // Paint the shaft back over the inner edges of the knobs so they join it.
    ctx.beginPath();
    ctx.moveTo(0.2, 0.22);
    ctx.lineTo(0.29, 0.31);
    ctx.strokeStyle = bone;
    ctx.lineWidth = 0.13;
    ctx.stroke();

    // The meat: a round end at the top left, tapering down onto the bone.
    ctx.beginPath();
    ctx.moveTo(0.12, 0.1);
    ctx.bezierCurveTo(0.08, -0.34, -0.4, -0.5, -0.42, -0.14);
    ctx.bezierCurveTo(-0.44, 0.16, -0.12, 0.28, 0.08, 0.16);
    ctx.closePath();
    ctx.fillStyle = color;
    ctx.fill();
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = pen;
    ctx.stroke();

    // Two short marks across the meat: the grain of it, in the comic way.
    ctx.beginPath();
    ctx.moveTo(-0.26, -0.22);
    ctx.quadraticCurveTo(-0.18, -0.28, -0.1, -0.24);
    ctx.moveTo(-0.3, -0.04);
    ctx.quadraticCurveTo(-0.2, -0.1, -0.1, -0.06);
    ctx.lineWidth = pen * 0.8;
    ctx.stroke();
}
