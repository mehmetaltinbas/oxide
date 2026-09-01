export function pick<T>(rng: () => number, items: readonly T[]): T {
    return items[Math.floor(rng() * items.length)];
}
