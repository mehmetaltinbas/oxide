import { MeleeStrike } from 'src/features/items/types/melee-strike.type';
import { MeleeMotion } from 'src/features/render/types/melee-motion.interface';

/** The right shoulder, where every swing pivots. */
const SHOULDER: [number, number] = [10, -1];
/** From the shoulder to the grip, arm extended. */
const ARM = 12.5;
/** From the grip out to a tool's head, near enough for the motion lines. */
const HEAD = 20;

/** One key pose: the arm's angle from straight ahead, the tool's, and so on. */
interface Key {
    arm: number;
    tool: number;
    stretch: number;
    twist: number;
    /** A hand position to use instead of one worked out from `arm`. */
    hand?: [number, number];
}

/**
 * How a melee weapon is carried and swung, frame by frame.
 *
 * Every blow has three parts: a wind-up it starts in (the blow lands the
 * moment you click, so the swing opens already drawn back), a fast strike
 * through the target in front of you, and a slower recovery to the carry.
 * The strike takes the first third of the swing and eases out, the way a
 * swing is fastest at the start and brakes through the follow-through.
 *
 * - A chop (hatchet) swings from out at the right side across the front.
 * - An overhead blow (pick, hammer) comes from up over the shoulder, head
 *   behind you, down onto the ground ahead: from above, the tool looks short
 *   while it points at the sky and full length when it lands.
 * - A smash (a rock in the fist) is drawn back by the ear and driven forward.
 *
 * `t` runs 0 to 1 over the swing; null is the carry between blows.
 */
export function meleeMotion(style: MeleeStrike, t: number | null, walkPhase: number): MeleeMotion {
    const bob = Math.sin(walkPhase) * 0.8;
    const carry = CARRY[style] ?? CARRY.chop;
    if (t === null)
        return pose({
            ...carry,
            hand: carry.hand ? [carry.hand[0], carry.hand[1] + bob] : undefined,
        });

    const keys = SWING[style] ?? SWING.chop;
    const strikeEnd = 0.34;
    let k: Key;
    let sweep = 0;
    if (t < strikeEnd) {
        const u = easeOut(t / strikeEnd);
        k = mix(keys.windup, keys.strike, u);
        sweep = 1 - t / strikeEnd;
    } else {
        const u = easeInOut((t - strikeEnd) / (1 - strikeEnd));
        k = mix(keys.strike, carry, u);
    }
    const m = pose(k);
    // Motion lines behind the head while it is moving fast.
    if (sweep > 0.15) {
        if (style === 'smash') {
            const w = pose(keys.windup).hand;
            m.smear = { kind: 'line', x0: w[0], y0: w[1], x1: m.hand[0], y1: m.hand[1] };
        } else {
            const back = mix(keys.windup, k, 0.55);
            m.smear = {
                kind: 'arc',
                radius: ARM + HEAD * k.stretch,
                from: back.arm - Math.PI / 2,
                to: k.arm - Math.PI / 2,
            };
        }
    }
    return m;
}

/** A key pose made concrete: where the hand is, from the arm's angle. */
function pose(k: Key): MeleeMotion {
    const hand: [number, number] = k.hand ?? [
        SHOULDER[0] + Math.sin(k.arm) * ARM,
        SHOULDER[1] - Math.cos(k.arm) * ARM,
    ];
    return { hand, angle: k.tool, stretch: k.stretch, twist: k.twist };
}

function mix(a: Key, b: Key, u: number): Key {
    const l = (x: number, y: number): number => x + (y - x) * u;
    const ha = a.hand ?? pose(a).hand;
    const hb = b.hand ?? pose(b).hand;
    return {
        arm: l(a.arm, b.arm),
        tool: l(a.tool, b.tool),
        stretch: l(a.stretch, b.stretch),
        twist: l(a.twist, b.twist),
        hand: [l(ha[0], hb[0]), l(ha[1], hb[1])],
    };
}

function easeOut(u: number): number {
    return 1 - (1 - u) * (1 - u) * (1 - u);
}

function easeInOut(u: number): number {
    return u * u * (3 - 2 * u);
}

/**
 * How each is carried between blows: the hand low at the right hip, the head
 * of the tool out ahead and a little to the side, not pointed at the sky.
 */
const CARRY: Record<MeleeStrike, Key> = {
    chop: { arm: 0.75, tool: 0.35, stretch: 1, twist: 0, hand: [12, -5] },
    overhead: { arm: 0.75, tool: 0.3, stretch: 1, twist: 0, hand: [12, -5] },
    smash: { arm: 0.4, tool: 0, stretch: 1, twist: 0, hand: [9.5, -9] },
    thrust: { arm: 0.3, tool: 0, stretch: 1, twist: 0, hand: [7.5, -12] },
};

/** Where each blow starts and where it lands. */
const SWING: Record<MeleeStrike, { windup: Key; strike: Key }> = {
    chop: {
        // Out to the right, head swung back behind the shoulder.
        windup: { arm: 1.75, tool: 2.1, stretch: 1, twist: 0.25 },
        // Across the front, head through the target and past it.
        strike: { arm: -0.3, tool: -0.55, stretch: 1, twist: -0.3 },
    },
    overhead: {
        // Raised over the shoulder: head behind you, pointing up, so short.
        windup: { arm: 2.4, tool: 2.9, stretch: 0.55, twist: 0.12, hand: [8, 2] },
        // Driven down onto the ground ahead, at full length.
        strike: { arm: 0.1, tool: 0.02, stretch: 1.08, twist: -0.18, hand: [5.5, -16] },
    },
    smash: {
        // Cocked back beside the ear.
        windup: { arm: 2.2, tool: 0, stretch: 1, twist: 0.2, hand: [11, 2] },
        // Driven forward into what is in front, across to the centre line.
        strike: { arm: -0.1, tool: 0, stretch: 1, twist: -0.35, hand: [3.5, -17] },
    },
    thrust: {
        windup: { arm: 0.3, tool: 0, stretch: 1, twist: 0, hand: [7.5, -12] },
        strike: { arm: 0.3, tool: 0, stretch: 1, twist: 0, hand: [4.5, -34] },
    },
};
