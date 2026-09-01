/** The slice of a player other clients need in order to draw them. */
export interface NetPlayer {
    id: string;
    name: string;
    x: number;
    y: number;
    facing: number;
    health: number;
    /** Index of the held belt item, purely cosmetic for remote players. */
    held: string | null;
    alive: boolean;
    /** Last input sequence number this position accounts for. */
    ack: number;
}
