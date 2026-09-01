import { BuildTier } from 'src/features/building/types/build-tier.type';
import { ClanSkill } from 'src/features/clans/types/clan-skill.type';

export interface ClanSkillDef {
    skill: ClanSkill;
    label: string;
    /** Relative chance of a new clan rolling this skill. */
    weight: number;
    /** Compound footprint in cells, and what it is built from. */
    baseSize: number;
    tier: BuildTier;
    /** Bodies living at the base. */
    members: number;
    /** Tech gained per day, which drives raid size and charges. */
    techPerDay: number;
    /** Multiplies how often they come for you. */
    raidPace: number;
    /** Multiplies what is in their loot box. */
    wealth: number;
    /** Seconds between pieces going up while they are founding a base. */
    buildStep: number;
}
