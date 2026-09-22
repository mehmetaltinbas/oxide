/**
 * Every binding, in one place.
 *
 * Settings is where a player looks for controls, so this is the only copy.
 * The help panel stays what it is: how the island works, not which key does it.
 */
export const CONTROLS_REFERENCE: { group: string; rows: [string, string][] }[] = [
    {
        group: 'Moving',
        rows: [
            ['WASD', 'Walk'],
            ['Shift', 'Sprint'],
            ['Mouse', 'Aim'],
            ['E', 'Use what you face'],
        ],
    },
    {
        group: 'Your hands',
        rows: [
            ['1-6', 'Belt slot'],
            ['Left click', 'Swing or fire'],
            ['R', 'Reload'],
            ['G', 'Drop the held stack'],
        ],
    },
    {
        group: 'Making and building',
        rows: [
            ['Tab', 'Pack and containers'],
            ['C', 'Crafting'],
            ['Q', 'Cycle piece, holding a plan'],
            ['Drag', 'Move a stack; out to drop'],
        ],
    },
    {
        group: 'The island',
        rows: [
            ['M', 'Open the map'],
            ['H', 'How the island works'],
            ['Esc', 'Settings, or back out'],
        ],
    },
];
