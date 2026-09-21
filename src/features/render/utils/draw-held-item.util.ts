import { DEFAULT_HELD_POSE, HELD_POSES } from 'src/features/items/constants/held-poses.constant';
import { drawItemIcon } from 'src/features/items/icons';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * An item in a hand, with the hand at the origin.
 *
 * Drawn from the item's own icon rather than a generic handle and blade, which
 * is what made a rock look like a pickaxe. `swing` turns it about the grip.
 */
export function drawHeldItem(ctx: CanvasRenderingContext2D, id: ItemId, swing = 0): void {
    const pose = HELD_POSES[id] ?? DEFAULT_HELD_POSE;
    ctx.save();
    ctx.rotate(pose.angle + swing);
    drawItemIcon(ctx, id, -pose.gripX * pose.size, -pose.gripY * pose.size, pose.size);
    ctx.restore();
}
