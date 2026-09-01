export function dist2(ax: number, ay: number, bx: number, by: number): number {
    const dx = bx - ax;
    const dy = by - ay;
    return dx * dx + dy * dy;
}
