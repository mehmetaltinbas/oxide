import { CLAN_SKILLS } from 'src/features/clans/constants/clan-skills.constant';
import { ClanSkill } from 'src/features/clans/types/clan-skill.type';

export function rollClanSkill(rand: () => number): ClanSkill {
    const entries = Object.values(CLAN_SKILLS);
    const total = entries.reduce((a, e) => a + e.weight, 0);
    let roll = rand() * total;
    for (const e of entries) {
        roll -= e.weight;
        if (roll <= 0) return e.skill;
    }
    return 'seasoned';
}
