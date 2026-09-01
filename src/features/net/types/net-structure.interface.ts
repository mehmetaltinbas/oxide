/** A built piece, mirrored from the server's authoritative build state. */
export interface NetStructure {
    id: number;
    kind: string;
    tier: string;
    gx: number;
    gy: number;
    side?: 'n' | 'w';
    hp: number;
    maxHp: number;
    owner: string;
    open?: boolean;
}
