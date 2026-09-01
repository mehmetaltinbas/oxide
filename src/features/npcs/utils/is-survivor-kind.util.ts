import { NpcKind } from 'src/features/npcs/types/npc-kind.type';

export function isSurvivorKind(kind: NpcKind): boolean {
    return kind === 'gatherer' || kind === 'raider';
}
