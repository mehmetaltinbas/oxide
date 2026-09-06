import { Game } from 'src/app/game';
import { CELL } from 'src/features/building/constants/cell.constant';
import { DeployableKind } from 'src/features/building/types/deployable-kind.type';
import { addItem } from 'src/features/items/utils/add-item.util';
import { DAY_SECONDS } from 'src/features/time/constants/day-seconds.constant';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * Jump straight to a game state worth testing.
 *
 * Checking a change used to mean playing from the beach until the game reached
 * the situation the change was about, which for anything past the first few
 * minutes is a lot of gathering before you learn whether it even works. These
 * set the game up in one step instead.
 *
 * Development only. `main.ts` reads `?scenario=` inside a dev check, and the
 * console helpers live in the same block, so none of this ships. Hanging them
 * off the harness unconditionally would keep the whole table in the bundle.
 *
 * Adding one is a new row here, not a new branch somewhere else.
 */
export interface Scenario {
    what: string;
    set(game: Game): void;
}

/** Run the simulation on, so a state settles rather than starting mid-air. */
function settle(game: Game, seconds: number): void {
    const steps = Math.round(seconds * 60);
    for (let i = 0; i < steps; i++) game.update(1 / 60, i === 0);
}

/** Put things in the player's hands, so a test is not a shopping trip. */
function give(game: Game, items: [ItemId, number][]): void {
    for (const [id, count] of items) addItem(game.containers[0], id, count);
}

/** Drop a deployable on the cell the player is standing in, offset by cells. */
function deployBeside(game: Game, kind: DeployableKind, dx: number, dy: number, hp = 300): void {
    const gx = Math.floor(game.player.x / CELL) + dx;
    const gy = Math.floor(game.player.y / CELL) + dy;
    game.build.deploy(kind, gx, gy, 0, hp);
}

export const SCENARIOS: Record<string, Scenario> = {
    fresh: {
        what: 'On the beach with a rock, as a new survivor starts.',
        set(game) {
            game.startSingle();
            settle(game, 1);
        },
    },

    kitted: {
        what: 'Tools, weapons and materials in the pack, for testing anything else.',
        set(game) {
            SCENARIOS.fresh.set(game);
            give(game, [
                ['wood', 2000],
                ['stone', 2000],
                ['metal', 1000],
                ['scrap', 500],
                ['bandage', 10],
                ['building_plan', 1],
                ['hatchet', 1],
                ['pickaxe', 1],
            ]);
        },
    },

    base: {
        what: 'Standing in a small base of your own, with a cupboard and a box.',
        set(game) {
            SCENARIOS.kitted.set(game);
            const gx = Math.floor(game.player.x / CELL);
            const gy = Math.floor(game.player.y / CELL);
            // A two by two floor with a wall along the north of each front cell.
            for (let x = 0; x < 2; x++) {
                for (let y = 0; y < 2; y++) game.build.placeFoundation(gx + x, gy + y, 0);
            }
            for (let x = 0; x < 2; x++) {
                // Soft side faces the player, which is where the inside is.
                game.build.placeEdge(gx + x, gy, 'n', 'wall', 0, {
                    x: game.player.x,
                    y: game.player.y,
                });
            }
            deployBeside(game, 'tool_cupboard', 0, 1, 400);
            deployBeside(game, 'wooden_box', 1, 1, 200);
            game.build.reindex();
            game.npcs.invalidateNavigation();
            settle(game, 0.5);
        },
    },

    night: {
        what: 'The same base at midnight, when the island turns unfriendly.',
        set(game) {
            SCENARIOS.base.set(game);
            // Straight to the next midnight. `skipTime` looks like the obvious
            // way and is creative-mode only: it returned without doing anything
            // and this scenario quietly handed back broad daylight.
            game.clock = Math.ceil(game.clock / DAY_SECONDS) * DAY_SECONDS;
            settle(game, 0.5);
        },
    },

    monument: {
        what: 'Standing at the nearest monument, where the loot and the trouble are.',
        set(game) {
            SCENARIOS.kitted.set(game);
            const here = game.world.monuments
                .map((m) => ({ m, d: Math.hypot(m.x - game.player.x, m.y - game.player.y) }))
                .sort((a, b) => a.d - b.d)[0];
            if (here) {
                game.player.x = here.m.x;
                game.player.y = here.m.y + here.m.radius * 0.6;
            }
            settle(game, 0.5);
        },
    },

    clanNeighbours: {
        what: 'Next door to the nearest AI clan, for raids and building privilege.',
        set(game) {
            SCENARIOS.kitted.set(game);
            const tc = game.build
                .toolCupboards()
                .filter((d) => d.owner !== 0)
                .sort(
                    (a, b) =>
                        Math.hypot(a.x - game.player.x, a.y - game.player.y) -
                        Math.hypot(b.x - game.player.x, b.y - game.player.y),
                )[0];
            if (tc) {
                game.player.x = tc.x + 260;
                game.player.y = tc.y;
            }
            settle(game, 0.5);
        },
    },
};
