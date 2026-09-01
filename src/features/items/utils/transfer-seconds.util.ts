import { TRANSFER } from 'src/features/items/constants/transfer.constant';

/** How long shifting this many of a thing should take. */
export function transferSeconds(count: number): number {
    return Math.min(TRANSFER.maxSeconds, TRANSFER.baseSeconds + count * TRANSFER.perItemSeconds);
}
