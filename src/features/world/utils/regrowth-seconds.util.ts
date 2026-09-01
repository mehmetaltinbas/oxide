import { REGROWTH } from 'src/features/world/constants/regrowth.constant';
import { randRange } from 'src/shared/utils/rand-range.util';

/** One in-game day, give or take a little, so a cleared field staggers. */
export function regrowthSeconds(): number {
    const slack = REGROWTH.seconds * REGROWTH.spread;
    return REGROWTH.seconds + randRange(Math.random, -slack, slack);
}
