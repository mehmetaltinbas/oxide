import { BuildTier } from 'src/features/building/types/build-tier.type';
import { TierDef } from 'src/features/building/types/tier-def.interface';

export const TIER_DEFS: Record<BuildTier, TierDef> = {
    twig: {
        tier: 'twig',
        name: 'Twig',
        hp: 10,
        cost: { wood: 10 },
        color: '#6b6a4a',
        edge: '#4a4936',
        satchels: 1,
    },
    wood: {
        tier: 'wood',
        name: 'Wood',
        hp: 250,
        cost: { wood: 50 },
        color: '#8a6034',
        edge: '#5d4022',
        satchels: 1,
    },
    stone: {
        tier: 'stone',
        name: 'Stone',
        hp: 500,
        cost: { stone: 150 },
        color: '#7e858c',
        edge: '#4f555b',
        satchels: 2,
    },
    metal: {
        tier: 'metal',
        name: 'Sheet Metal',
        hp: 1000,
        cost: { metal: 200 },
        color: '#9aa8b4',
        edge: '#5f6a74',
        satchels: 4,
    },
};
