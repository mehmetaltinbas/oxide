/**
 * A darker version of a hex colour, for shading one shape against another.
 *
 * Used where a second tone has to be derived from a value that comes from data
 * rather than from the design tokens: a garment's colour lives on its item
 * definition, so its sleeves cannot be a token.
 */
export function darken(hex: string, amount = 0.35): string {
    const clean = hex.replace('#', '');
    const full =
        clean.length === 3
            ? clean
                  .split('')
                  .map((c) => c + c)
                  .join('')
            : clean;
    const n = parseInt(full, 16);
    const r = Math.round(((n >> 16) & 255) * (1 - amount));
    const g = Math.round(((n >> 8) & 255) * (1 - amount));
    const b = Math.round((n & 255) * (1 - amount));
    return `rgb(${r}, ${g}, ${b})`;
}
