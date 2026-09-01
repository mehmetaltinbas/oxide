import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { Game } from 'src/app/game';
import { Renderer } from 'src/features/render/renderer';
import { saveGame } from 'src/features/session/save';
import { hasSave } from 'src/features/session/utils/has-save.util';
import { loadGame } from 'src/features/session/utils/load-game.util';
import { Hud } from 'src/features/ui/hud';
import { Input } from 'src/shared/core/input';

const canvas = document.getElementById('game') as HTMLCanvasElement;
const ctx = canvas.getContext('2d', { alpha: false });
if (!ctx) throw new Error('This browser has no 2D canvas.');

const input = new Input(canvas);
let viewW = window.innerWidth;
let viewH = window.innerHeight;

const game = new Game(input, viewW, viewH);
const renderer = new Renderer(ctx);
const hud = new Hud(ctx, renderer);

function resize(): void {
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    viewW = window.innerWidth;
    viewH = window.innerHeight;
    canvas.width = Math.floor(viewW * dpr);
    canvas.height = Math.floor(viewH * dpr);
    canvas.style.width = `${viewW}px`;
    canvas.style.height = `${viewH}px`;
    ctx!.setTransform(dpr, 0, 0, dpr, 0, 0);
    game.camera.resize(viewW, viewH);
}

// A fresh island every launch. A previous one can still be picked up from the
// title screen, but random generation is the default rather than the exception.
game.newWorld();
game.canContinue = hasSave();
game.phase = 'title';
game.onSave = () => saveGame(game);

// Title screen: Enter rolls a new island, C picks up where you left off.
window.addEventListener('keydown', (e) => {
    if (game.phase !== 'title') return;
    if (e.code === 'KeyC' && game.canContinue) {
        if (loadGame(game)) {
            game.canContinue = false;
            game.phase = game.player.alive ? 'play' : 'dead';
        }
    }
});

resize();
window.addEventListener('resize', resize);
window.addEventListener('beforeunload', () => saveGame(game));

const unlock = (): void => game.audio.unlock();
window.addEventListener('pointerdown', unlock, { once: true });
window.addEventListener('keydown', unlock, { once: true });

// Respawn is handled here so the death screen can own the Enter key.
window.addEventListener('keydown', (e) => {
    if (e.code !== 'Enter') return;
    if (game.phase === 'dead' && game.player.respawnTimer <= 0) game.respawn();
});

const STEP = 1 / 60;
/**
 * Frame ceiling. Above this there is nothing left to see: the simulation is a
 * fixed 60Hz: and uncapped rAF on a high-refresh display just burns battery
 * and spins fans for no visible gain.
 */
const MAX_FPS = 120;
const MIN_FRAME_MS = 1000 / MAX_FPS;

let accumulator = 0;
let last = performance.now();
let lastDrawn = 0;

function frame(now: number): void {
    // Skip the frame entirely if we are ahead of the cap; the next rAF will land
    // closer to the target.
    if (now - lastDrawn < MIN_FRAME_MS - 0.5) {
        requestAnimationFrame(frame);
        return;
    }
    const elapsed = Math.min((now - last) / 1000, 0.25);
    last = now;
    lastDrawn = now;
    accumulator += elapsed;

    // Smoothed so the counter is readable rather than jittering every frame.
    if (elapsed > 0) game.fps += (1 / elapsed - game.fps) * 0.1;

    // Input is polled once per frame, independent of how many simulation steps
    // run, so a keypress can never fall between two steps and be lost.
    game.pollInput();

    let steps = 0;
    while (accumulator >= STEP && steps < 5) {
        game.update(STEP, steps === 0);
        accumulator -= STEP;
        steps++;
    }
    if (steps === 5) accumulator = 0;

    ctx!.fillStyle = WORLD.voidBackdrop;
    ctx!.fillRect(0, 0, viewW, viewH);
    renderer.draw(game);
    hud.draw(game, viewW, viewH);

    input.endFrame();
    requestAnimationFrame(frame);
}

requestAnimationFrame(frame);

// Exposed for debugging in the console: window.oxide.game
(window as unknown as Record<string, unknown>).oxide = { game, input, renderer, hud, saveGame };
