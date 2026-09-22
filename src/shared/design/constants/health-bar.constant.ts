/** A health bar over anything in the world: white, in a black box. */
export const HEALTH_BAR = {
    fill: '#ffffff',
    /**
     * Height of the white bar, and the black box round it each side, in world
     * units. Both a quarter down from 4 and 1.2, which read as too heavy:
     * 4.8 across in all, from 6.4.
     */
    height: 3,
    border: 0.9,
} as const;
