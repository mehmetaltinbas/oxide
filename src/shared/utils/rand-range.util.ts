export function randRange(rng: () => number, lo: number, hi: number): number {
    return lo + rng() * (hi - lo);
}
