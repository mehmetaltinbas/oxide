import { BOW_HOLD } from 'src/features/items/constants/bow-hold.constant';
import { INK } from 'src/shared/design/constants/ink.constant';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';

/**
 * A bow in the hand, seen from above, with the hand at the origin and forward
 * up the screen. The grip is in the fist, the limbs sweep back toward the
 * archer, and the string runs between their tips. `draw` from 0 to 1 pulls
 * the string back into a V with an arrow on it: full is ready to loose.
 *
 * Drawn here rather than from the bow's icon because the icon's string is a
 * straight line, and a drawn bow is the one held item whose shape changes.
 */
export function drawBow(ctx: CanvasRenderingContext2D, draw: number): void {
    const tipX = 9.5;
    const tipY = 5.5 - draw * 1.5;
    const nockY = BOW_HOLD.nockRest + draw * BOW_HOLD.pull;
    ctx.save();
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    // The string, under the limbs, and the arrow along it.
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = 0.9;
    ctx.beginPath();
    ctx.moveTo(-tipX, tipY);
    ctx.lineTo(0, nockY);
    ctx.lineTo(tipX, tipY);
    ctx.stroke();
    if (draw > 0) {
        const headY = nockY - 24;
        stroke(ctx, 0, nockY, 0, headY + 3, 1.6, WORLD.wood);
        ctx.fillStyle = '#cfd8e0';
        ctx.beginPath();
        ctx.moveTo(0, headY - 1);
        ctx.lineTo(-2, headY + 3);
        ctx.lineTo(2, headY + 3);
        ctx.closePath();
        ctx.fill();
        ctx.fillStyle = '#8fae6a';
        ctx.fillRect(-1.6, nockY - 4, 3.2, 3);
    }

    // The limbs: one curve from tip to tip through the grip.
    ctx.beginPath();
    ctx.moveTo(-tipX, tipY);
    ctx.quadraticCurveTo(0, -tipY, tipX, tipY);
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = 2.6 + INK.width;
    ctx.stroke();
    ctx.strokeStyle = WORLD.wood;
    ctx.lineWidth = 2.6;
    ctx.stroke();
    ctx.restore();
}

/** A thick line with the pen under it. */
function stroke(
    ctx: CanvasRenderingContext2D,
    x0: number,
    y0: number,
    x1: number,
    y1: number,
    width: number,
    color: string,
): void {
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.strokeStyle = INK.line;
    ctx.lineWidth = width + INK.width;
    ctx.stroke();
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.stroke();
}
