import { CraftJob } from 'src/features/crafting/types/craft-job.interface';

/**
 * The crafting queue. Entry 0 is the one actually being made; the rest are
 * waiting. Everything in the queue is already paid for, so cancelling has to
 * refund, and reordering costs nothing.
 */
export type CraftQueue = CraftJob[];
