import { HumanLook } from 'src/features/render/types/human-look.interface';
import { darken } from 'src/shared/utils/darken.util';
import { INK } from 'src/shared/design/constants/ink.constant';
import { TAU } from 'src/shared/constants/tau.constant';

/**
 * One person, seen from above, in the context's current frame: origin at the
 * body's centre, facing up the screen (negative y is forward).
 *
 * Built the way a person is actually shaped from overhead rather than as a
 * stack of circles: feet stepping under the body, a pair of shoulders wider
 * than they are deep, arms that swing against the stride and end in hands, and
 * a head that is mostly hair from this angle with the face showing only as a
 * crescent at the front, a nose and two ears. The fills are inked by the
 * canvas's pen like everything else in the world; the marks inside (the
 * parting, the collar, the seams) are drawn here, solid black.
 *
 * Everybody human goes through this one routine, so a survivor, a scientist
 * and the player cannot drift into three different drawings of a person.
 */
export function drawHuman(ctx: CanvasRenderingContext2D, look: HumanLook): void {
    const s = look.radius / 13;
    const tint = (c: string): string => look.hurt ?? c;
    const skin = tint(look.skin);
    const hair = tint(look.hair);
    const shirt = look.shirt ? tint(look.shirt) : skin;
    const sleeve = look.shirt ? tint(darken(look.shirt)) : skin;
    const legs = tint(look.legs);
    const step = Math.sin(look.phase) * 4 * look.stride;

    ctx.save();
    ctx.scale(s, s);
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';

    // Feet, under everything, one ahead of the other.
    if (!look.swimming) {
        ctx.fillStyle = darken(legs);
        for (const [x, y] of [
            [-4.6, -step],
            [4.6, step],
        ] as const) {
            ctx.beginPath();
            ctx.ellipse(x, y + 1, 3, 4.6, 0, 0, TAU);
            ctx.fill();
        }
    }

    // Arms, from the shoulder to the hand, drawn before the torso so the
    // shoulder sits over the top of the arm. A swimmer reaches forward in turn.
    const reach = look.swimming ? Math.sin(look.phase) * 5 : 0;
    const hands: [number, number][] = look.swimming
        ? [
              [-7, -14 - reach],
              [7, -14 + reach],
          ]
        : [[-12.5, -2 + step * 0.9], look.holding ? [7.5, -12] : [12.5, -2 - step * 0.9]];
    // Shoulders: where each arm leaves the body.
    const shoulders: [number, number][] = [
        [-10, -1],
        [10, -1],
    ];
    let jab = 0;
    if (look.punch && !look.swimming) {
        // Out fast and back slower, the way a jab is thrown: the fist is at
        // full reach a third of the way through, then recovers.
        const t = look.punch.t;
        jab = t < 0.35 ? t / 0.35 : 1 - (t - 0.35) / 0.65;
        jab = jab * jab * (3 - 2 * jab);
        const throwing = look.punch.side === -1 ? 0 : 1;
        const guard = 1 - throwing;
        const sx = look.punch.side;
        // The throwing shoulder rolls forward and the fist drives in toward
        // the centre line, out ahead of the face.
        shoulders[throwing] = [sx * 9.5, -1 - 3.5 * jab];
        hands[throwing] = [sx * (12.5 - 9 * jab), -2 - 22 * jab];
        // The other fist comes up by the chin.
        hands[guard] = [-sx * (12.5 - 5.5 * jab), -2 - 8 * jab];
    }
    for (let i = 0; i < 2; i++) {
        const [hx, hy] = hands[i];
        const [sx, sy] = shoulders[i];
        limb(ctx, sx, sy, hx, hy, 5.2, sleeve);
        // The item goes under the fist, so the hand closes over its grip.
        if (i === 1 && look.held) {
            ctx.save();
            ctx.translate(hx, hy);
            look.held(ctx);
            ctx.restore();
        }
        ctx.fillStyle = skin;
        ctx.beginPath();
        // A fist that is punching is balled up, a touch bigger than an open hand.
        const throwing = look.punch && (look.punch.side === -1 ? 0 : 1) === i;
        ctx.arc(hx, hy, throwing ? 2.9 + 0.7 * jab : 2.9, 0, TAU);
        ctx.fill();
    }
    // At full reach, a few strokes behind the fist: the comic way of saying it
    // moved fast. Straight lines trailing back along the arm.
    if (look.punch && jab > 0.75) {
        const i = look.punch.side === -1 ? 0 : 1;
        const [hx, hy] = hands[i];
        marks(ctx, () => {
            for (const off of [-2.6, 0, 2.6]) {
                ctx.moveTo(hx + off, hy + 5);
                ctx.lineTo(hx + off * 1.3, hy + 11);
            }
        });
    }

    // The shoulders and chest: wider than deep, rounded at the ends.
    const halfW = look.swimming ? 9.5 : 11.5;
    const halfD = look.swimming ? 5 : 6.2;
    ctx.fillStyle = shirt;
    ctx.beginPath();
    ctx.roundRect(-halfW, -halfD + 1, halfW * 2, halfD * 2, halfD);
    ctx.fill();
    marks(ctx, () => {
        // The collar, round the back of the neck.
        ctx.moveTo(-4.2, 1.2);
        ctx.quadraticCurveTo(0, 4.4, 4.2, 1.2);
        if (look.shirt) {
            // Shoulder seams, where a sleeve is sewn on.
            ctx.moveTo(-halfW + 3.6, -halfD + 2.2);
            ctx.lineTo(-halfW + 2.8, halfD - 0.6);
            ctx.moveTo(halfW - 3.6, -halfD + 2.2);
            ctx.lineTo(halfW - 2.8, halfD - 0.6);
        }
    });

    // The head.
    const hy = -1.2;
    if (look.hood) {
        const hood = tint(look.hood);
        ctx.fillStyle = hood;
        ctx.beginPath();
        ctx.arc(0, hy, 8, 0, TAU);
        ctx.fill();
        ctx.fillStyle = '#2c3a3f';
        ctx.beginPath();
        ctx.ellipse(0, hy - 3, 5.4, 3.2, 0, 0, TAU);
        ctx.fill();
    } else {
        // Ears first, so the head covers their inner half.
        ctx.fillStyle = skin;
        for (const side of [-1, 1]) {
            ctx.beginPath();
            ctx.ellipse(side * 6.6, hy - 0.4, 1.6, 2.3, 0, 0, TAU);
            ctx.fill();
        }
        ctx.beginPath();
        ctx.arc(0, hy, 6.8, 0, TAU);
        ctx.fill();
        // The nose, just past the brow: the one thing that says which way a
        // face is turned when all you can see is the top of a head.
        ctx.beginPath();
        ctx.ellipse(0, hy - 7, 1.5, 1.9, 0, 0, TAU);
        ctx.fill();
        // Hair over the crown and back, leaving the face as a crescent.
        ctx.fillStyle = hair;
        ctx.beginPath();
        ctx.arc(0, hy + 1.7, 6.6, 0, TAU);
        ctx.fill();
        marks(ctx, () => {
            // A parting and the lie of the hair, back from the brow.
            ctx.moveTo(-1.2, hy - 3.2);
            ctx.quadraticCurveTo(-1.6, hy + 1, -0.8, hy + 5.4);
            ctx.moveTo(2.6, hy - 2.2);
            ctx.quadraticCurveTo(3.4, hy + 1.2, 2.8, hy + 4.6);
            ctx.moveTo(-4.4, hy - 1);
            ctx.quadraticCurveTo(-4.8, hy + 1.8, -3.6, hy + 4);
        });
    }
    ctx.restore();
}

/** An arm: a thick line with the pen under it, the way a drawn limb is inked. */
function limb(
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

/** Interior line work, solid black and lighter than the outline. */
function marks(ctx: CanvasRenderingContext2D, build: () => void): void {
    ctx.save();
    ctx.strokeStyle = INK.line;
    ctx.globalAlpha = 1;
    ctx.lineWidth = INK.fineWidth;
    ctx.beginPath();
    build();
    ctx.stroke();
    ctx.restore();
}
