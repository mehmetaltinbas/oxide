import { Biome } from 'src/features/world/types/biome.type';

/** What each biome is called where the player reads it, at the top of the screen. */
export const BIOME_LABEL: Record<Biome, string> = {
    grass: 'Grass',
    forest: 'Forest',
    beach: 'Beach',
    snow_beach: 'Snowy Beach',
    desert: 'Desert',
    snow: 'Snow',
    road: 'Road',
    water: 'Water',
};
