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
    // Lifted a step on 2026-09-22: the whole frame averaged 90 to 114 out of
    // 255 in brightness, and with the ink on top it read as dusk at noon.
    grass: '#6c9c3b',
    forest: '#508a31',
    beach: '#e8cf96',
    // Sand under snow: the snow's white, warmed a touch by what is beneath.
    snow_beach: '#ece9dd',
    desert: '#d8b762',
    snow: '#dde9f2',
    road: '#726d62',
    water: '#2f80c8',
};
