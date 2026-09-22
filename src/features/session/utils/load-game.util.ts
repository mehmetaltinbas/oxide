import { Game } from 'src/app/game';
import { SAVE_KEY } from 'src/features/session/constants/save-key.constant';
import { SaveShape } from 'src/features/session/types/save-shape.interface';

/**
 * Rebuild the world from its seed, then lay the saved deltas back on top.
 * Returns false when there is nothing usable to load.
 */
export function loadGame(game: Game): boolean {
    let raw: string | null = null;
    try {
        raw = localStorage.getItem(SAVE_KEY);
    } catch {
        return false;
    }
    if (!raw) return false;

    let data: SaveShape;
    try {
        data = JSON.parse(raw) as SaveShape;
    } catch {
        return false;
    }
    if (!data || data.version !== 1) return false;

    // newWorld regenerates the same map from the seed and wires up the systems;
    // the saved deltas then go back on top of it.
    game.newWorld(data.seed);
    game.clock = data.clock ?? 0;

    for (const [id, hp, respawn] of data.nodeDeltas ?? []) {
        const node = game.world.nodes.find((n) => n.id === id);
        if (!node) continue;
        node.hp = hp;
        node.respawn = respawn;
    }
    const looted = new Set(data.lootedCrates ?? []);
    for (const c of game.world.crates) {
        if (looted.has(c.id)) {
            c.looted = true;
            c.respawn = 240;
        }
    }

    game.player = data.player;
    game.build.structures = data.structures ?? [];
    game.build.deployables = data.deployables ?? [];
    game.build.reindex();
    if (data.nextBuildId) game.build.setNextId(Math.max(data.nextBuildId, game.build.nextIdValue));

    // Time passes while the game is closed, and bases rot when nobody is home.
    const away = Math.max(0, (Date.now() - (data.savedAt ?? Date.now())) / 1000);
    game.structures.applyDecay(away / 3600);

    game.camera.x = game.player.x;
    game.camera.y = game.player.y;
    game.phase = game.player.alive ? 'play' : 'dead';
    return true;
}
