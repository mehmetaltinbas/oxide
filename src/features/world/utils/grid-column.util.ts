/** A column's letters: A to Z, then AA, AB and on, as Rust does past Z. */
export function gridColumn(col: number): string {
    let n = col;
    let out = '';
    do {
        out = String.fromCharCode(65 + (n % 26)) + out;
        n = Math.floor(n / 26) - 1;
    } while (n >= 0);
    return out;
}
