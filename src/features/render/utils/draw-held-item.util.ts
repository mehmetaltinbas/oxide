import { DEFAULT_HELD_POSE, HELD_POSES } from 'src/features/items/constants/held-poses.constant';
import { drawItemIcon } from 'src/features/items/icons';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * Where a tool's head sits in its icon, along the icon's own length: the
 * business end is about this far up from the middle in every one of them.
 */
const HEAD_Y = -0.2;

/**
 * An item in a hand, with the hand at the origin.
 *
 * Drawn from the item's own icon rather than a generic handle and blade, which
 * is what made a rock look like a pickaxe. `swing` turns it about the grip.
 */
export function drawHeldItem(
    ctx: CanvasRenderingContext2D,
    id: ItemId,
    swing = 0,
    /**
     * How long the item looks along its length, 1 as it is. Below 1 is
     * foreshortening: a pick raised over the shoulder points half at the sky,
     * and from above it looks shorter.
     */
    stretch = 1,
    /**
     * Put the head of the tool over the hand rather than the grip.
     *
     * A tool held upright is seen end-on from above: what sits over the fist
     * is its head, with the handle running away below it, out of the picture.
     * Drawn from the grip it hung off to one side instead.
     */
    overHand = false,
): void {
    const pose = HELD_POSES[id] ?? DEFAULT_HELD_POSE;
    ctx.save();
    ctx.rotate(swing);
    if (overHand) ctx.translate(0, (pose.gripY - HEAD_Y) * pose.size * stretch);
    ctx.scale(1, stretch);
    ctx.rotate(pose.angle);
    drawItemIcon(ctx, id, -pose.gripX * pose.size, -pose.gripY * pose.size, pose.size);
    ctx.restore();
}
