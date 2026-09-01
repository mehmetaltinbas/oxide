export function segmentHitsCircle(
    ax: number,
    ay: number,
    bx: number,
    by: number,
    cx: number,
    cy: number,
    radius: number,
): boolean {
    const dx = bx - ax;
    const dy = by - ay;
    const len2 = dx * dx + dy * dy;
    let t = len2 > 0 ? ((cx - ax) * dx + (cy - ay) * dy) / len2 : 0;
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    const nx = ax + dx * t;
    const ny = ay + dy * t;
    const ddx = cx - nx;
    const ddy = cy - ny;
    return ddx * ddx + ddy * ddy <= radius * radius;
}
