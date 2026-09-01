import { Container } from 'src/features/items/types/container.interface';

export interface LootCrate {
    id: number;
    x: number;
    y: number;
    monumentId: string;
    container: Container;
    looted: boolean;
    respawn: number;
}
