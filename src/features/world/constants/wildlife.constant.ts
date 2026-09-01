import { NpcKind } from 'src/features/npcs/types/npc-kind.type';

/**
 * Wildlife population for the whole island.
 *
 * Halved again from the already-sparse pass. On an island this size the old
 * numbers still meant something was usually in earshot, which made a bear an
 * inconvenience rather than a thing you back away from. Meeting anything should
 * be an event, and with one-day respawns a hunted-out stretch stays quiet long
 * enough to notice.
 */
export const WILDLIFE: [NpcKind, number][] = [
    ['boar', 90],
    ['wolf', 55],
    ['bear', 26],
];
