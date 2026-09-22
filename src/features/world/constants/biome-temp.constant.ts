import { Biome } from 'src/features/world/types/biome.type';

/** Ambient temperature by biome, in degrees. Night subtracts more on top. */
export const BIOME_TEMP: Record<Biome, number> = {
    grass: 20,
    forest: 16,
    beach: 22,
    /** Sand under snow: as cold as the snowfield it edges. */
    snow_beach: -6,
    desert: 34,
    snow: -8,
    road: 20,
    water: 12,
};
