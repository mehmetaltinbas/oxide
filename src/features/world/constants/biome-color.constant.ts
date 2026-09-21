import { Biome } from 'src/features/world/types/biome.type';

/**
 * Flat colour per biome, printed rather than painted.
 *
 * Saturated on purpose. The old values were a shade off grey, which under the
 * ink pass and the dot screen came out washed: comic flats are laid down at
 * full strength because the black is what carries the shading, so a colour that
 * starts muted has nowhere left to go.
 */
export const BIOME_COLOR: Record<Biome, string> = {
    grass: '#55812e',
    forest: '#3a6a22',
    beach: '#d9be85',
    desert: '#c2a04e',
    snow: '#c6d6e4',
    road: '#5c584f',
    water: '#1f68ad',
};
