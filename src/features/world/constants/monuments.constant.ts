import { MonumentDef } from 'src/features/world/types/monument-def.interface';

export const MONUMENTS: MonumentDef[] = [
    {
        // Deliberately unguarded and un-irradiated: somewhere a fresh spawn can
        // loot on day one without a gun or a hazmat suit.
        id: 'cabins',
        name: 'Abandoned Cabins',
        radius: 160,
        rads: 0,
        scientists: 0,
        crates: 3,
        loot: {
            wood: [40, 120],
            cloth: [10, 30],
            stone: [20, 60],
            bandage: [1, 2],
            lowgrade: [5, 15],
        },
        color: '#6b6250',
    },
    {
        id: 'lighthouse',
        name: 'Lighthouse',
        radius: 210,
        rads: 0,
        scientists: 0,
        crates: 3,
        loot: {
            scrap: [10, 25],
            cloth: [10, 30],
            lowgrade: [10, 30],
            metal: [20, 60],
            bandage: [1, 3],
        },
        color: '#8a8f96',
    },
    {
        id: 'airfield',
        name: 'Airfield',
        radius: 340,
        rads: 4,
        scientists: 4,
        crates: 6,
        loot: {
            scrap: [25, 60],
            metal: [60, 160],
            sulfur: [20, 60],
            pistol_ammo: [8, 24],
            shotgun_shell: [4, 12],
            medkit: [1, 2],
            gunpowder: [10, 40],
        },
        color: '#6f6f68',
    },
    {
        id: 'powerplant',
        name: 'Power Plant',
        radius: 380,
        rads: 9,
        scientists: 7,
        crates: 8,
        loot: {
            scrap: [50, 120],
            metal: [120, 300],
            sulfur: [60, 140],
            rifle_ammo: [10, 30],
            c4: [0, 1],
            hazmat: [0, 1],
            // Now and then an AK, as in Rust's elite crates. Roughly one crate
            // in four, since a [0, 1] roll is half the time a zero.
            ak47: [0, 1],
            rocket_launcher: [0, 1],
            gunpowder: [30, 90],
        },
        color: '#5f6663',
    },
];
