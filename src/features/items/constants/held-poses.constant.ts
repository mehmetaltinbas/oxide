import { HeldPose } from 'src/features/items/types/held-pose.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * Anything without a row of its own: held in the fist, at a size that reads
 * as a thing in a hand rather than a thing on the ground.
 */
export const DEFAULT_HELD_POSE: HeldPose = { size: 14, gripX: 0, gripY: 0, angle: 0 };

/**
 * One row per item that is held some particular way. The grips and angles are
 * read off each icon in `icons.ts`: where its handle ends, which way its blade
 * or barrel points.
 */
export const HELD_POSES: Partial<Record<ItemId, HeldPose>> = {
    // A rock is a stone in the fist, gripped from behind so it shows past the
    // knuckles; held by its middle, the hand covered it completely.
    rock: { size: 17, gripX: 0, gripY: 0.2, angle: 0 },
    // Tools by the end of the handle, head forward.
    hatchet: { size: 28, gripX: 0.11, gripY: 0.28, angle: 0 },
    pickaxe: { size: 30, gripX: 0, gripY: 0.28, angle: 0 },
    hammer: { size: 24, gripX: 0, gripY: 0.28, angle: 0 },
    building_plan: { size: 20, gripX: 0, gripY: 0.22, angle: 0 },
    // The spear's shaft runs corner to corner in its icon; turned to point ahead.
    spear: { size: 44, gripX: -0.12, gripY: 0.16, angle: -0.69 },
    // The bow is held across the body, belly forward.
    bow: { size: 30, gripX: -0.24, gripY: 0, angle: Math.PI / 2 },
    // Guns point along +x in their icons.
    revolver: { size: 26, gripX: -0.17, gripY: 0.16, angle: -Math.PI / 2 },
    rifle: { size: 40, gripX: -0.12, gripY: 0.12, angle: -Math.PI / 2 },
    rocket_launcher: { size: 46, gripX: -0.05, gripY: 0.1, angle: -Math.PI / 2 },
    ak47: { size: 42, gripX: -0.1, gripY: 0.08, angle: -Math.PI / 2 },
};
