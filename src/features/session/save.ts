import { Game } from 'src/app/game';
import { SAVE_KEY } from 'src/features/session/constants/save-key.constant';
import { SaveShape } from 'src/features/session/types/save-shape.interface';

export function saveGame(game: Game): void {
    try {
        const nodeDeltas: [number, number, number][] = [];
        for (const n of game.world.nodes) {
            if (n.hp !== n.maxHp || n.respawn > 0)
                nodeDeltas.push([n.id, Math.round(n.hp), Math.round(n.respawn)]);
        }
        const data: SaveShape = {
            version: 1,
            seed: game.world.seed,
            clock: game.clock,
            savedAt: Date.now(),
            player: game.player,
            structures: game.build.structures,
            deployables: game.build.deployables,
            nodeDeltas,
            lootedCrates: game.world.crates.filter((c) => c.looted).map((c) => c.id),
            nextBuildId: game.build.nextIdValue,
        };
        localStorage.setItem(SAVE_KEY, JSON.stringify(data));
    } catch {
        // A full or blocked localStorage should never take the game down.
    }
}
