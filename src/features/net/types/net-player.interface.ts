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
    /**
     * Which team this player is on: 0 for nobody, otherwise a team id shared
     * with everyone on it. Teammates see each other in the world and on the
     * map; nothing else about them changes, and they can still shoot each
     * other.
     */
    team: number;
}
