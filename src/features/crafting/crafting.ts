import { BuildSystem } from 'src/features/building/building';
import { CRAFT_QUEUE_MAX } from 'src/features/crafting/constants/craft-queue-max.constant';
import { RECIPES } from 'src/features/crafting/constants/recipes.constant';
import { CraftHooks } from 'src/features/crafting/types/craft-hooks.interface';
import { CraftJob } from 'src/features/crafting/types/craft-job.interface';
import { CraftQueue } from 'src/features/crafting/types/craft-queue.type';
import { WorkbenchLevel } from 'src/features/crafting/types/workbench-level.type';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { ItemId } from 'src/features/items/types/item-id.type';
import { acquire } from 'src/features/items/utils/acquire.util';
import { countAcross } from 'src/features/items/utils/count-across.util';
import { payCraft } from 'src/features/items/utils/pay-craft.util';
import { refundCraft } from 'src/features/items/utils/refund-craft.util';
import { Audio } from 'src/shared/core/audio';
import { dist } from 'src/shared/utils/dist.util';

export class CraftSystem {
    /**
     * Entry 0 is being made right now, the rest are waiting. Everything in the
     * queue is already paid for, so cancelling refunds and reordering is free.
     */
    craftQueue: CraftQueue = [];

    constructor(
        private build: BuildSystem,
        private audio: Audio,
        private hooks: CraftHooks,
    ) {}

    get craftJob(): CraftJob | null {
        return this.craftQueue[0] ?? null;
    }

    /** Highest workbench the player is standing near. */
    benchLevel(): WorkbenchLevel {
        if (this.hooks.sandbox()) return 3;
        let best: WorkbenchLevel = 0;
        for (const d of this.build.deployables) {
            if (d.owner !== 0) continue;
            if (dist(this.hooks.player().x, this.hooks.player().y, d.x, d.y) > 160) continue;
            const lvl =
                d.kind === 'workbench3'
                    ? 3
                    : d.kind === 'workbench2'
                      ? 2
                      : d.kind === 'workbench1'
                        ? 1
                        : 0;
            if (lvl > best) best = lvl as WorkbenchLevel;
        }
        return best;
    }

    updateCrafting(dt: number): void {
        const job = this.craftQueue[0];
        if (!job) return;
        job.remaining -= dt;
        if (job.remaining > 0) return;

        const recipe = RECIPES[job.recipeIndex];
        const left = acquire(
            this.hooks.player().hotbar,
            this.hooks.player().inventory,
            recipe.out,
            recipe.amount,
        );
        if (left > 0)
            this.hooks.dropStack(
                { id: recipe.out, count: left },
                this.hooks.player().x,
                this.hooks.player().y,
            );

        job.left -= 1;
        if (job.left > 0) {
            job.remaining = recipe.seconds;
            return;
        }
        this.craftQueue.shift();
        const next = this.craftQueue[0];
        if (next) next.remaining = RECIPES[next.recipeIndex].seconds;
    }

    /** Drop a queued run and hand the materials back. */
    cancelCraft(index: number): void {
        const job = this.craftQueue[index];
        if (!job) return;
        const recipe = RECIPES[job.recipeIndex];
        if (!this.hooks.sandbox()) {
            const spilled = refundCraft(
                recipe,
                job.left,
                this.hooks.player().hotbar,
                this.hooks.player().inventory,
            );
            for (const s of spilled)
                this.hooks.dropStack(s, this.hooks.player().x, this.hooks.player().y);
        }
        this.craftQueue.splice(index, 1);
        const head = this.craftQueue[0];
        if (head && index === 0) head.remaining = RECIPES[head.recipeIndex].seconds;
        this.hooks.notify(`Cancelled ${ITEMS[recipe.out].name}.`);
    }

    /**
     * Move a waiting run up or down the queue. The one being made stays put:
     * shuffling it would mean throwing away the seconds already spent on it.
     */
    reorderCraft(index: number, dir: -1 | 1): void {
        const to = index + dir;
        if (index < 1 || to < 1 || index >= this.craftQueue.length || to >= this.craftQueue.length)
            return;
        const [job] = this.craftQueue.splice(index, 1);
        this.craftQueue.splice(to, 0, job);
    }

    /** How many of a recipe the player could actually pay for right now. */
    craftableCount(index: number): number {
        const recipe = RECIPES[index];
        if (this.hooks.sandbox()) return 99;
        if (recipe.bench > this.benchLevel()) return 0;
        let most = 99;
        for (const [id, need] of Object.entries(recipe.cost) as [ItemId, number][]) {
            most = Math.min(most, Math.floor(countAcross(this.hooks.containers(), id) / need));
        }
        return Math.max(0, most);
    }

    startCraft(index: number, amount = 1): void {
        const recipe = RECIPES[index];
        if (this.craftQueue.length >= CRAFT_QUEUE_MAX) {
            this.audio.deny();
            this.hooks.notify('Crafting queue is full.');
            return;
        }
        const possible = Math.min(amount, this.craftableCount(index));
        if (possible <= 0) {
            this.audio.deny();
            this.hooks.notify(
                recipe.bench > this.benchLevel()
                    ? `Needs a level ${recipe.bench} workbench.`
                    : 'Not enough materials.',
            );
            return;
        }
        // Pay for the whole run up front, then produce one at a time. Paying now
        // is what lets several runs queue up without racing each other for the
        // same wood, and is why cancelling has to refund.
        if (!this.hooks.sandbox())
            for (let i = 0; i < possible; i++) payCraft(recipe, this.hooks.containers());
        const seconds = this.hooks.sandbox() ? 0 : recipe.seconds;
        this.craftQueue.push({ recipeIndex: index, remaining: seconds, left: possible });
        this.audio.build();
    }
}
