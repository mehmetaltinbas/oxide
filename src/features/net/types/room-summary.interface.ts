export interface RoomSummary {
    id: string;
    name: string;
    players: number;
    maxPlayers: number;
    seed: number;
    /** Seconds since the room's world was created. */
    age: number;
    pvp: boolean;
}
