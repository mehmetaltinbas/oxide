/**
 * A number in 0..1 from a seed and a slot, the same every time it is asked.
 *
 * For drawing variety that must not flicker: a tree's lean is looked up, not
 * rolled, so it leans the same way on every frame.
 */
export function seeded(seed: number, slot: number): number {
    const n = Math.sin(seed * 12.9898 + slot * 78.233) * 43758.5453;
    return n - Math.floor(n);
}
