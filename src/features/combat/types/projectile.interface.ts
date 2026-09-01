import { Faction } from 'src/features/combat/types/faction.type';

export interface Projectile {
    x: number;
    y: number;
    vx: number;
    vy: number;
    life: number;
    damage: number;
    faction: Faction;
    /**
     * Who fired it: 0 for the player or wildlife, otherwise a clan owner id.
     * A shot hits anything that is not on the shooter's side, which is what lets
     * an armed clansman actually shoot a boar.
     */
    shooterClan: number;
    color: string;
    length: number;
}
