import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { Game } from 'src/app/game';
import { Renderer } from 'src/features/render/renderer';
import { saveGame } from 'src/features/session/save';
import { hasSave } from 'src/features/session/utils/has-save.util';
import { loadGame } from 'src/features/session/utils/load-game.util';
import { Hud } from 'src/features/ui/hud';
import { listScenarios, runScenario } from 'src/features/dev/run-scenario.util';
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

// Respawn is handled here so the death screen owns its keys: a number wakes
// you in that bag, B on a beach, and Enter in your first bag if you have one.
window.addEventListener('keydown', (e) => {
    if (game.phase !== 'dead' || game.player.respawnTimer > 0) return;
    const bags = game.myBags();
    if (e.code === 'Enter') game.respawn(bags[0]?.id ?? null);
    else if (e.code === 'KeyB') game.respawn(null);
    else if (/^Digit[1-9]$/.test(e.code)) {
        const bag = bags[Number(e.code.slice(5)) - 1];
        if (bag) game.respawn(bag.id);
    }
});

const STEP = 1 / 60;

/**
 * The next frame, as soon as the machine will give it.
 *
 * `requestAnimationFrame` hands a frame back at the display's refresh and not
 * one sooner, so on a 120Hz panel it answers the panel's question rather than
 * this game's: how long does a frame actually take to build. A message posted
 * to ourselves comes back on the next turn of the event loop with no such
 * ceiling and no four millisecond clamp, so the counter reads what the drawing
 * costs.
 *
 * The cost of that is a hot machine: nothing throttles this loop, and a
 * backgrounded tab keeps running where rAF would have stopped it.
 */
const channel = new MessageChannel();
let pending: ((now: number) => void) | null = null;
channel.port1.onmessage = (): void => {
    const run = pending;
    pending = null;
    if (run) run(performance.now());
};
function schedule(next: (now: number) => void): void {
    pending = next;
    channel.port2.postMessage(0);
}

let accumulator = 0;
let last = performance.now();

function frame(now: number): void {
    const elapsed = Math.min((now - last) / 1000, 0.25);
    last = now;
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

    // Smooth the room at the display's rate, not the simulation's, then draw
    // between simulation steps rather than on them. The fixed step and the
    // frames are not locked together, so some frames run no step and others run
    // two, and everything landed on step boundaries.
    game.smoothRoom();
    game.beginFrame(accumulator / STEP);

    ctx!.fillStyle = WORLD.voidBackdrop;
    ctx!.fillRect(0, 0, viewW, viewH);
    renderer.draw(game);
    hud.draw(game, viewW, viewH);
    game.endFrame();

    input.endFrame();
    schedule(frame);
}

// The lettering is loaded before the first frame is drawn. `font-display:
// block` stops the browser painting a fallback, but canvas measures text
// itself: without this the first frames are laid out in monospace and then jump
// when the real face arrives.
if (document.fonts) {
    void document.fonts.ready.then(() => schedule(frame));
} else {
    schedule(frame);
}

// Exposed for debugging in the console: window.oxide.game
const harness: Record<string, unknown> = { game, input, renderer, hud, saveGame };
(window as unknown as Record<string, unknown>).oxide = harness;

// Jump straight to a state worth testing, so checking a change does not mean
// playing from the beach to reach the situation it is about.
//
// Everything sits inside the dev check, the console helpers included:
// `import.meta.env.DEV` folds to false in a build, so this block goes, and with
// nothing left referencing the module it goes too. Hanging the helpers off the
// harness unconditionally would ship the whole scenario table.
if (import.meta.env.DEV) {
    harness.scenario = (name: string) => runScenario(game, name);
    harness.scenarios = listScenarios;
    const wanted = new URLSearchParams(window.location.search).get('scenario');
    if (wanted) runScenario(game, wanted);
    else listScenarios();
}
