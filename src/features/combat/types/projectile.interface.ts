import { Faction } from 'src/features/combat/types/faction.type';

export interface Projectile {
    x: number;
    y: number;
    vx: number;
    vy: number;
    life: number;
    damage: number;
    faction: Faction;
    color: string;
    length: number;
}
