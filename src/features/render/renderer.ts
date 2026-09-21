import { TYPE } from 'src/shared/design/constants/type.constant';
import { PROGRESS_ARC } from 'src/shared/design/constants/progress-arc.constant';
import { drawHuman } from 'src/features/render/utils/draw-human.util';
import { drawHeldItem } from 'src/features/render/utils/draw-held-item.util';
import { ItemId } from 'src/features/items/types/item-id.type';
import { TREE_COVER } from 'src/features/render/constants/tree-cover.constant';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { HUMAN_PALETTE } from 'src/shared/design/constants/human-palette.constant';
import { ComicTexture } from 'src/features/render/comic-texture';
import { Ink } from 'src/features/render/ink';
import { GRASS, INK } from 'src/shared/design/constants/ink.constant';
import { wornId } from 'src/features/items/utils/worn-id.util';
import { darken } from 'src/shared/utils/darken.util';
import { GameView } from 'src/app/types/game-view.type';
import { CELL } from 'src/features/building/constants/cell.constant';
import { TC_RADIUS } from 'src/features/building/constants/tc-radius.constant';
import { TIER_DEFS } from 'src/features/building/constants/tier-defs.constant';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { centerOf } from 'src/features/building/utils/center-of.util';
import { edgeSegment } from 'src/features/building/utils/edge-segment.util';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { drawItemIcon } from 'src/features/items/icons';
import { NPCS } from 'src/features/npcs/constants/npcs.constant';
import { Npc } from 'src/features/npcs/types/npc.interface';
import { BIOME_COLOR } from 'src/features/world/constants/biome-color.constant';
import { BIOME_H } from 'src/features/world/constants/biome-h.constant';
import { BIOME_TILE } from 'src/features/world/constants/biome-tile.constant';
import { BIOME_W } from 'src/features/world/constants/biome-w.constant';
import { MONUMENTS } from 'src/features/world/constants/monuments.constant';
import { NODES } from 'src/features/world/constants/nodes.constant';
import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { TAU } from 'src/shared/constants/tau.constant';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';

/** Shared comparator, so sorting does not allocate a closure per frame. */
function byY(a: { y: number }, b: { y: number }): number {
    return a.y - b.y;
}

const nodeBuf: ResourceNode[] = [];

export class Renderer {
    /** The pen, and the printed textures. Both belong to this canvas. */
    private ink: Ink;
    private texture: ComicTexture;

    constructor(private ctx: CanvasRenderingContext2D) {
        this.ink = new Ink(ctx);
        this.texture = new ComicTexture(ctx);
    }

    draw(game: GameView): void {
        const ctx = this.ctx;
        const cam = game.camera;
        ctx.save();
        cam.applyTransform(ctx);

        // A new island invalidates every baked chunk.
        if (this.bakedFor !== game.world) {
            this.bakedFor = game.world;
            this.mapCanvas = null;
        }
        this.biomes = game.world.biomes;
        const b = cam.viewBounds(140);
        // Everything in the world is a drawn shape, so it all gets the pen.
        // The interface is not a panel and is left alone; see `ink.ts`.
        this.ink.start();
        this.drawTerrain(b);
        this.drawMonuments(game);
        this.drawFoundations(game, b);
        this.drawGround(game);
        this.drawDeployables(game, b);
        this.drawNodes(game, b);
        this.drawWalls(game, b);
        this.drawRoofs(game, b);
        this.drawExplosives(game);
        this.drawNpcs(game, b);
        this.drawRemotePlayers(game);
        this.drawPlayer(game);
        this.drawCovering();
        // Light, sparks and a ghost placement are not objects: a line round
        // any of them turns it into a sticker.
        this.ink.suspend(() => {
            this.drawShots(game);
            this.drawParticles(game);
            this.drawBuildPreview(game);
        });
        this.ink.stop();

        ctx.restore();

        this.drawLighting(game);

        ctx.save();
        cam.applyTransform(ctx);
        this.drawFloatingText(game);
        ctx.restore();
    }

    // ---------------------------------------------------------------- terrain

    /**
     * Terrain is a flat colour per biome tile, so the whole island bakes into ONE
     * small canvas at one pixel per tile and is blitted scaled with smoothing
     * off. The whole 20736² map costs ~190KB.
     *
     * This replaced per-chunk 1536² canvases that were never evicted: walking the
     * map accumulated close to 2GB of canvas memory, at which point the browser
     * started dropping them and the world rendered as black bands.
     * See docs/systems/performance.md.
     */
    private drawTerrain(b: { x0: number; y0: number; x1: number; y1: number }): void {
        const ctx = this.ctx;
        const map = this.terrainMap();
        if (map) {
            const sx = Math.max(0, Math.floor(b.x0 / BIOME_TILE));
            const sy = Math.max(0, Math.floor(b.y0 / BIOME_TILE));
            const ex = Math.min(BIOME_W, Math.ceil(b.x1 / BIOME_TILE));
            const ey = Math.min(BIOME_H, Math.ceil(b.y1 / BIOME_TILE));
            const sw = Math.max(1, ex - sx);
            const sh = Math.max(1, ey - sy);
            const smoothing = ctx.imageSmoothingEnabled;
            ctx.imageSmoothingEnabled = false;
            ctx.drawImage(
                map,
                sx,
                sy,
                sw,
                sh,
                sx * BIOME_TILE,
                sy * BIOME_TILE,
                sw * BIOME_TILE,
                sh * BIOME_TILE,
            );
            ctx.imageSmoothingEnabled = smoothing;
        }

        // Mottling and water shimmer stay live, over visible tiles only. Bounded by
        // the viewport, so the cost does not grow with the map.
        const x0 = Math.max(0, Math.floor(b.x0 / BIOME_TILE));
        const x1 = Math.min(BIOME_W - 1, Math.floor(b.x1 / BIOME_TILE));
        const y0 = Math.max(0, Math.floor(b.y0 / BIOME_TILE));
        const y1 = Math.min(BIOME_H - 1, Math.floor(b.y1 / BIOME_TILE));
        const t = performance.now() / 900;
        // Mottling and shimmer are the tone of the ground, not things on it:
        // inked, every tile came out as a cell in a black grid.
        this.ink.suspend(() => {
            for (let y = y0; y <= y1; y++) {
                for (let x = x0; x <= x1; x++) {
                    const biome = this.biomes![y * BIOME_W + x];
                    if (biome === 'water') {
                        if (Math.sin((x + y) * 0.7 + t) <= 0.55) continue;
                        ctx.fillStyle = 'rgba(120, 180, 220, 0.13)';
                    } else {
                        const n = ((x * 73856093) ^ (y * 19349663)) >>> 0;
                        if (n % 5 !== 0) continue;
                        ctx.fillStyle = 'rgba(0,0,0,0.07)';
                    }
                    ctx.fillRect(x * BIOME_TILE, y * BIOME_TILE, BIOME_TILE + 1, BIOME_TILE + 1);
                }
            }
        });

        this.drawGrass(b);
        this.drawGroundScreen(b);
    }

    /**
     * Tufts of grass across open ground.
     *
     * On a fixed grid, jittered from the cell's own coordinates, so a tuft
     * stays put as the camera moves. Strokes rather than fills, which keeps the
     * pen off them: a blade of grass is already a mark.
     */
    private drawGrass(b: { x0: number; y0: number; x1: number; y1: number }): void {
        const ctx = this.ctx;
        const step = GRASS.spacing;
        ctx.save();
        ctx.strokeStyle = GRASS.color;
        ctx.lineWidth = GRASS.width;
        ctx.lineCap = 'round';
        ctx.beginPath();
        const gx0 = Math.floor(b.x0 / step);
        const gx1 = Math.ceil(b.x1 / step);
        const gy0 = Math.floor(b.y0 / step);
        const gy1 = Math.ceil(b.y1 / step);
        for (let gy = gy0; gy <= gy1; gy++) {
            for (let gx = gx0; gx <= gx1; gx++) {
                const n = Math.sin(gx * 127.1 + gy * 311.7) * 43758.5453;
                const r = n - Math.floor(n);
                const n2 = Math.sin(gx * 269.5 + gy * 183.3) * 43758.5453;
                const r2 = n2 - Math.floor(n2);
                if (r > 0.62) continue;
                const x = gx * step + r * step;
                const y = gy * step + r2 * step;
                // Nothing grows in the sea.
                const bx = Math.floor(x / BIOME_TILE);
                const by = Math.floor(y / BIOME_TILE);
                if (this.biomes && this.biomes[by * BIOME_W + bx] === 'water') continue;
                for (let i = 0; i < GRASS.blades; i++) {
                    const lean = (i - 1) * 3.5 + (r - 0.5) * 3;
                    const h = GRASS.height * (0.7 + r2 * 0.6);
                    ctx.moveTo(x + (i - 1) * 3, y);
                    ctx.lineTo(x + (i - 1) * 3 + lean, y - h);
                }
            }
        }
        ctx.stroke();
        ctx.restore();
    }

    /**
     * The dot screen over the ground.
     *
     * Light enough to read as paper rather than as dirt, and the one thing that
     * stops a flat biome colour looking like a vector drawing.
     */
    private drawGroundScreen(b: { x0: number; y0: number; x1: number; y1: number }): void {
        const pattern = this.texture.halftone();
        if (!pattern) return;
        const ctx = this.ctx;
        ctx.save();
        ctx.fillStyle = pattern;
        this.ink.suspend(() => ctx.fillRect(b.x0, b.y0, b.x1 - b.x0, b.y1 - b.y0));
        ctx.restore();
    }

    /**
     * Thin ink inside a shape.
     *
     * The outline says where a thing ends; these say what it is made of. Always
     * solid black: a comic has one ink and no grey, and a mark at half opacity
     * reads as pencil left under the ink.
     */
    private lines(ctx: CanvasRenderingContext2D, build: (path: Path2D) => void): void {
        const path = new Path2D();
        build(path);
        ctx.save();
        ctx.strokeStyle = INK.line;
        ctx.globalAlpha = 1;
        ctx.lineWidth = INK.markWidth;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';
        ctx.stroke(path);
        ctx.restore();
    }

    /**
     * Ink texture inside one tier of a conifer.
     *
     * Not an outline: the pen already drew that. These are the marks that say
     * what the shape is made of, which for a pine is the fishbone of its
     * branches and hatching down the side the light is not on.
     */
    private markTier(ctx: CanvasRenderingContext2D, y: number, r: number, variant: number): void {
        const apexY = y - r;
        const baseY = y + r * 0.5;
        const height = baseY - apexY;

        ctx.save();
        ctx.beginPath();
        ctx.moveTo(0, apexY);
        ctx.lineTo(-r, baseY);
        ctx.lineTo(r, baseY);
        ctx.closePath();
        ctx.clip();

        ctx.strokeStyle = INK.line;
        ctx.globalAlpha = 1;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        // Branches, two rows, the lower one wider: a tier spreads as it drops.
        ctx.lineWidth = INK.markWidth;
        ctx.beginPath();
        for (const t of [0.42, 0.7]) {
            const py = apexY + height * t;
            const reach = r * t * 0.62;
            const drop = reach * 0.42;
            const lean = ((variant % 3) - 1) * 0.6;
            ctx.moveTo(lean, py);
            ctx.lineTo(-reach + lean, py + drop);
            ctx.moveTo(lean, py);
            ctx.lineTo(reach + lean, py + drop);
        }
        ctx.stroke();

        // Shade down the right flank: hatching is how a press says the light
        // comes from the other side.
        ctx.lineWidth = INK.hatchMarkWidth;
        ctx.beginPath();
        const step = INK.hatchSpacing * 0.75;
        for (let hx = r * 0.18; hx < r; hx += step) {
            ctx.moveTo(hx, baseY);
            ctx.lineTo(hx + height * 0.45, apexY + height * 0.25);
        }
        ctx.stroke();
        ctx.restore();
    }

    /**
     * Facets and stipple on stone.
     *
     * A boulder is flat faces meeting at edges, and a press shades the far side
     * with dots rather than with a darker grey.
     */
    private markStone(radius: number, seed: number): void {
        const ctx = this.ctx;
        this.ink.suspend(() => {
            ctx.save();
            ctx.beginPath();
            ctx.ellipse(0, 0, radius * 0.95, radius * 0.74, 0, 0, TAU);
            ctx.clip();

            ctx.fillStyle = INK.line;
            const step = INK.halftoneSpacing * 0.55;
            for (let y = -radius; y < radius; y += step) {
                for (let x = -radius; x < radius; x += step) {
                    const shade = (x + y) / (radius * 2);
                    if (shade < 0.12) continue;
                    const off = ((Math.floor((y + radius) / step) % 2) * step) / 2;
                    ctx.beginPath();
                    ctx.arc(x + off, y, INK.stippleDot * (0.6 + shade * 0.9), 0, TAU);
                    ctx.fill();
                }
            }

            ctx.strokeStyle = INK.line;
            ctx.lineWidth = INK.markWidth;
            ctx.lineCap = 'round';
            ctx.lineJoin = 'round';
            ctx.beginPath();
            for (let i = 0; i < 3; i++) {
                const a = (((seed + i * 29) % 100) / 100) * TAU;
                ctx.moveTo(-radius * 0.1, -radius * 0.12);
                ctx.lineTo(Math.cos(a) * radius * 0.9, Math.sin(a) * radius * 0.7);
            }
            ctx.stroke();
            ctx.restore();
        });
    }

    /** Bake the biome grid once, one pixel per tile. */
    private terrainMap(): HTMLCanvasElement | null {
        if (this.mapCanvas) return this.mapCanvas;
        if (!this.biomes) return null;
        const canvas = document.createElement('canvas');
        canvas.width = BIOME_W;
        canvas.height = BIOME_H;
        const c = canvas.getContext('2d');
        if (!c) return null;
        for (let y = 0; y < BIOME_H; y++) {
            for (let x = 0; x < BIOME_W; x++) {
                c.fillStyle = BIOME_COLOR[this.biomes[y * BIOME_W + x]];
                c.fillRect(x, y, 1, 1);
            }
        }
        this.mapCanvas = canvas;
        return canvas;
    }

    /** Biome grid for the current world, refreshed each frame by draw(). */
    private biomes: GameView['world']['biomes'] | null = null;
    /** The whole island baked at one pixel per biome tile. */
    private mapCanvas: HTMLCanvasElement | null = null;
    private bakedFor: unknown = null;

    private drawMonuments(game: GameView): void {
        const ctx = this.ctx;
        for (const m of game.world.monuments) {
            const def = MONUMENTS.find((d) => d.id === m.defId);
            if (!def) continue;
            ctx.save();
            ctx.globalAlpha = 0.55;
            ctx.fillStyle = def.color;
            ctx.beginPath();
            ctx.arc(m.x, m.y, m.radius, 0, TAU);
            ctx.fill();
            ctx.globalAlpha = 1;

            // Concrete slabs and a few blocks so it reads as a built-up place.
            ctx.fillStyle = '#55564f';
            const seed = m.x + m.y;
            for (let i = 0; i < 9; i++) {
                const a = (((seed + i * 97) % 360) / 360) * TAU;
                const r = (((seed + i * 53) % 100) / 100) * m.radius * 0.7;
                const w = 40 + ((seed + i * 31) % 70);
                ctx.fillRect(
                    m.x + Math.cos(a) * r - w / 2,
                    m.y + Math.sin(a) * r - w / 3,
                    w,
                    w * 0.66,
                );
            }

            if (def.rads > 0) {
                ctx.strokeStyle = 'rgba(180, 230, 90, 0.5)';
                ctx.lineWidth = 3;
                ctx.setLineDash([14, 12]);
                ctx.beginPath();
                ctx.arc(m.x, m.y, m.radius, 0, TAU);
                ctx.stroke();
                ctx.setLineDash([]);
                ctx.fillStyle = 'rgba(180, 230, 90, 0.75)';
                ctx.font = `bold 15px ${TYPE.display}`;
                ctx.textAlign = 'center';
                ctx.fillText(`☢ ${def.name}`, m.x, m.y - m.radius + 18);
                ctx.textAlign = 'left';
            } else {
                ctx.fillStyle = 'rgba(220,220,200,0.65)';
                ctx.font = `bold 14px ${TYPE.display}`;
                ctx.textAlign = 'center';
                ctx.fillText(def.name, m.x, m.y - m.radius + 18);
                ctx.textAlign = 'left';
            }
            ctx.restore();
        }

        for (const c of game.world.crates) {
            if (c.looted) continue;
            ctx.save();
            ctx.translate(c.x, c.y);
            ctx.fillStyle = WORLD.shadow;
            ctx.fillRect(-13, -8, 26, 22);
            ctx.fillStyle = '#7a6a44';
            ctx.fillRect(-14, -12, 28, 22);
            ctx.strokeStyle = '#4a4028';
            ctx.lineWidth = 2;
            ctx.strokeRect(-14, -12, 28, 22);
            ctx.fillStyle = WORLD.gold;
            ctx.fillRect(-14, -4, 28, 4);
            ctx.restore();
        }
    }

    // --------------------------------------------------------------- building

    private drawFoundations(
        game: GameView,
        b: { x0: number; y0: number; x1: number; y1: number },
    ): void {
        const ctx = this.ctx;
        for (const s of game.build.structures) {
            if (s.kind !== 'foundation') continue;
            const x = s.gx * CELL;
            const y = s.gy * CELL;
            if (x + CELL < b.x0 || x > b.x1 || y + CELL < b.y0 || y > b.y1) continue;
            const def = TIER_DEFS[s.tier];
            ctx.fillStyle = def.color;
            ctx.globalAlpha = s.tier === 'twig' ? 0.55 : 0.95;
            ctx.fillRect(x, y, CELL, CELL);
            ctx.globalAlpha = 1;
            ctx.strokeStyle = def.edge;
            ctx.lineWidth = 2;
            ctx.strokeRect(x + 1, y + 1, CELL - 2, CELL - 2);
            if (s.owner !== 0) {
                ctx.fillStyle = game.clanSystem.clanByOwner(s.owner)?.color ?? WORLD.unclaimed;
                ctx.globalAlpha = 0.18;
                ctx.fillRect(x, y, CELL, CELL);
                ctx.globalAlpha = 1;
            }
            if (s.hp < s.maxHp) this.hpBar(x + 6, y + CELL - 8, CELL - 12, s.hp / s.maxHp);
        }
    }

    private drawWalls(game: GameView, b: { x0: number; y0: number; x1: number; y1: number }): void {
        const ctx = this.ctx;
        for (const s of game.build.structures) {
            if (s.kind === 'foundation' || !s.side) continue;
            const [x0, y0, x1, y1] = edgeSegment(s.gx, s.gy, s.side);
            if (Math.max(x0, x1) < b.x0 || Math.min(x0, x1) > b.x1) continue;
            if (Math.max(y0, y1) < b.y0 || Math.min(y0, y1) > b.y1) continue;
            const def = TIER_DEFS[s.tier];
            const horizontal = s.side === 'n';
            const t = 9;

            ctx.save();
            if (s.flash > 0) {
                ctx.shadowColor = WORLD.fuse;
                ctx.shadowBlur = 16;
            }

            if (s.kind === 'doorway' || (s.kind === 'door' && s.open)) {
                // Frame only, with a gap in the middle you can walk through.
                ctx.fillStyle = def.color;
                const seg = 18;
                if (horizontal) {
                    ctx.fillRect(x0, y0 - t / 2, seg, t);
                    ctx.fillRect(x1 - seg, y0 - t / 2, seg, t);
                } else {
                    ctx.fillRect(x0 - t / 2, y0, t, seg);
                    ctx.fillRect(x0 - t / 2, y1 - seg, t, seg);
                }
                if (s.kind === 'door' && s.open) {
                    ctx.fillStyle = 'rgba(200,180,120,0.5)';
                    if (horizontal) ctx.fillRect(x0 + seg, y0 - 2, CELL - seg * 2, 4);
                    else ctx.fillRect(x0 - 2, y0 + seg, 4, CELL - seg * 2);
                }
            } else {
                ctx.fillStyle = def.color;
                ctx.globalAlpha = s.tier === 'twig' ? 0.7 : 1;
                if (horizontal) ctx.fillRect(x0, y0 - t / 2, CELL, t);
                else ctx.fillRect(x0 - t / 2, y0, t, CELL);
                ctx.globalAlpha = 1;
                ctx.strokeStyle = def.edge;
                ctx.lineWidth = 2;
                if (horizontal) ctx.strokeRect(x0, y0 - t / 2, CELL, t);
                else ctx.strokeRect(x0 - t / 2, y0, t, CELL);

                if (s.kind === 'door') {
                    ctx.fillStyle = WORLD.gold;
                    const c = centerOf(s);
                    ctx.beginPath();
                    ctx.arc(c.x, c.y, 3.5, 0, TAU);
                    ctx.fill();
                }
            }

            // A soft-side pip, so you can see which way a wall is facing.
            const c = centerOf(s);
            ctx.fillStyle = 'rgba(255,255,255,0.28)';
            ctx.beginPath();
            ctx.arc(c.x + s.softX * 9, c.y + s.softY * 9, 2.2, 0, TAU);
            ctx.fill();

            if (s.owner !== 0) {
                ctx.fillStyle = game.clanSystem.clanByOwner(s.owner)?.color ?? WORLD.unclaimed;
                ctx.globalAlpha = 0.5;
                ctx.beginPath();
                ctx.arc(c.x, c.y, 3, 0, TAU);
                ctx.fill();
                ctx.globalAlpha = 1;
            }
            ctx.restore();

            if (s.hp < s.maxHp) {
                this.hpBar(c.x - 18, c.y + (horizontal ? 10 : 0), 36, s.hp / s.maxHp);
            }
        }
    }

    private drawDeployables(
        game: GameView,
        b: { x0: number; y0: number; x1: number; y1: number },
    ): void {
        for (const d of game.build.deployables) {
            if (d.x < b.x0 - 50 || d.x > b.x1 + 50 || d.y < b.y0 - 50 || d.y > b.y1 + 50) continue;
            if (game.regionHidden(game.build.regionAt(d.x, d.y))) continue;
            this.drawDeployable(d, game);
            if (d.hp < d.maxHp) this.hpBar(d.x - 18, d.y + 20, 36, d.hp / d.maxHp);
        }
    }

    private drawDeployable(d: Deployable, game: GameView): void {
        const ctx = this.ctx;
        ctx.save();
        ctx.translate(d.x, d.y);
        if (d.flash > 0) {
            ctx.shadowColor = WORLD.fuse;
            ctx.shadowBlur = 14;
        }
        ctx.fillStyle = WORLD.shadow;
        ctx.fillRect(-18, -12, 38, 32);

        switch (d.kind) {
            case 'campfire': {
                ctx.fillStyle = '#4a4238';
                ctx.beginPath();
                ctx.arc(0, 0, 16, 0, TAU);
                ctx.fill();
                if (d.lit) {
                    const f = 0.75 + Math.sin(performance.now() / 90) * 0.25;
                    ctx.fillStyle = WORLD.fire;
                    ctx.beginPath();
                    ctx.ellipse(0, -2, 8 * f, 12 * f, 0, 0, TAU);
                    ctx.fill();
                    ctx.fillStyle = WORLD.emberHot;
                    ctx.beginPath();
                    ctx.ellipse(0, -2, 4 * f, 6 * f, 0, 0, TAU);
                    ctx.fill();
                } else {
                    ctx.fillStyle = '#2e2a22';
                    ctx.beginPath();
                    ctx.arc(0, 0, 7, 0, TAU);
                    ctx.fill();
                }
                break;
            }
            case 'furnace': {
                ctx.fillStyle = '#6b5540';
                ctx.fillRect(-18, -18, 36, 36);
                ctx.strokeStyle = '#3f3226';
                ctx.lineWidth = 2;
                ctx.strokeRect(-18, -18, 36, 36);
                ctx.fillStyle = d.lit ? WORLD.fire : '#241f18';
                ctx.fillRect(-8, 0, 16, 14);
                break;
            }
            case 'tool_cupboard': {
                ctx.fillStyle = '#7a6034';
                ctx.fillRect(-16, -18, 32, 36);
                ctx.fillStyle = WORLD.gold;
                ctx.fillRect(-12, -14, 24, 8);
                ctx.strokeStyle = '#4a3a20';
                ctx.lineWidth = 2;
                ctx.strokeRect(-16, -18, 32, 36);
                if (
                    game.heldItem()?.id === 'building_plan' ||
                    game.heldItem()?.id === 'tool_cupboard'
                ) {
                    ctx.strokeStyle =
                        d.owner === 0 ? 'rgba(140, 220, 140, 0.28)' : 'rgba(220, 120, 120, 0.28)';
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.arc(0, 0, TC_RADIUS, 0, TAU);
                    ctx.stroke();
                }
                break;
            }
            case 'wooden_box': {
                ctx.fillStyle = WORLD.timber;
                ctx.fillRect(-17, -14, 34, 28);
                ctx.strokeStyle = '#4f3822';
                ctx.lineWidth = 2;
                ctx.strokeRect(-17, -14, 34, 28);
                ctx.beginPath();
                ctx.moveTo(-17, -14);
                ctx.lineTo(17, 14);
                ctx.stroke();
                break;
            }
            case 'sleeping_bag': {
                ctx.fillStyle = '#a05a5a';
                ctx.fillRect(-18, -11, 36, 22);
                ctx.fillStyle = '#c98a8a';
                ctx.fillRect(-14, -7, 12, 14);
                break;
            }
            default: {
                // Workbenches
                const lvl = d.kind === 'workbench3' ? 3 : d.kind === 'workbench2' ? 2 : 1;
                ctx.fillStyle = '#8a7a5a';
                ctx.fillRect(-20, -14, 40, 28);
                ctx.fillStyle = '#5a4c34';
                ctx.fillRect(-20, 6, 40, 8);
                ctx.fillStyle = WORLD.lamp;
                ctx.font = `bold 13px ${TYPE.display}`;
                ctx.textAlign = 'center';
                ctx.fillText(String(lvl), 0, -9);
                ctx.textAlign = 'left';
                break;
            }
        }
        ctx.restore();
    }

    /**
     * A sealed room you are not inside, and hold no cupboard authority over, is
     * drawn as a roof. Blowing one door does not let you see the whole base.
     *
     * A room needs a floor. Sealing off a patch of open ground: a courtyard in
     * the middle of a compound, or a stretch of forest that several buildings
     * happen to ring in between them: encloses it for reach and privacy, but it
     * does not put a roof over it. Without this the game grew dark slabs of
     * "ceiling" across open woodland, with the trees still drawn on top of them.
     */
    private drawRoofs(game: GameView, b: { x0: number; y0: number; x1: number; y1: number }): void {
        const ctx = this.ctx;
        const cells = game.build.enclosedCells();
        if (cells.size === 0) return;
        ctx.save();
        for (const [key, region] of cells) {
            if (!game.regionHidden(region)) continue;
            const [gxs, gys] = key.split(',');
            const gx = Number(gxs);
            const gy = Number(gys);
            if (!game.build.foundationAt(gx, gy)) continue;
            const x = gx * CELL;
            const y = gy * CELL;
            if (x + CELL < b.x0 || x > b.x1 || y + CELL < b.y0 || y > b.y1) continue;
            ctx.fillStyle = WORLD.roof;
            ctx.fillRect(x, y, CELL + 1, CELL + 1);
            // Shingle hatching, so a roof reads as a roof rather than a hole.
            ctx.strokeStyle = WORLD.roofHatch;
            ctx.lineWidth = 1;
            for (let i = 0; i < 4; i++) {
                ctx.beginPath();
                ctx.moveTo(x, y + i * 16);
                ctx.lineTo(x + CELL, y + i * 16);
                ctx.stroke();
            }
        }
        ctx.restore();
    }

    private drawBuildPreview(game: GameView): void {
        const prev = game.held.buildPreview();
        if (!prev) return;
        const ctx = this.ctx;
        ctx.save();
        ctx.globalAlpha = 0.5;
        ctx.fillStyle = prev.valid ? '#5fb85f' : '#c0392b';
        if (prev.kind === 'foundation') {
            ctx.fillRect(prev.gx * CELL, prev.gy * CELL, CELL, CELL);
        } else if (prev.side) {
            const [x0, y0] = edgeSegment(prev.gx, prev.gy, prev.side);
            if (prev.side === 'n') ctx.fillRect(x0, y0 - 5, CELL, 10);
            else ctx.fillRect(x0 - 5, y0, 10, CELL);
        }
        ctx.restore();
    }

    // ----------------------------------------------------------------- nodes

    /** Trees with somebody standing behind them, drawn after the people. */
    private covering: ResourceNode[] = [];
    private bodies: { x: number; y: number }[] = [];

    private drawNodes(game: GameView, b: { x0: number; y0: number; x1: number; y1: number }): void {
        const nodes = game.world.nodesInRect(b.x0, b.y0, b.x1, b.y1, nodeBuf);
        nodes.sort(byY);
        this.gatherBodies(game, b);
        this.covering.length = 0;
        for (const n of nodes) {
            if (n.kind === 'tree' && n.hp > 0 && this.hidesSomebody(n)) {
                this.covering.push(n);
                continue;
            }
            this.drawNode(n);
        }
    }

    /** Everyone alive on screen, as points: the player, the room, the npcs. */
    private gatherBodies(
        game: GameView,
        b: { x0: number; y0: number; x1: number; y1: number },
    ): void {
        const out = this.bodies;
        out.length = 0;
        if (game.player.alive) out.push(game.player);
        if (game.mode === 'online') {
            for (const o of game.net.others.values()) if (o.alive) out.push(o);
        }
        for (const n of game.npcs.list) {
            if (n.x < b.x0 || n.x > b.x1 || n.y < b.y0 || n.y > b.y1) continue;
            out.push(n);
        }
    }

    /**
     * Whether a tree's crown covers somebody standing behind it.
     *
     * A tree is drawn from its trunk up the screen, so "behind" is north of
     * the trunk and inside the canopy's outline: the same box the tiers are
     * painted in. Measured off the tiers in `drawNode`: the crown's apex is
     * 3.0 radii above the trunk and the widest tier reaches 1.55 radii out.
     */
    private hidesSomebody(n: ResourceNode): boolean {
        for (const e of this.bodies) {
            const dy = n.y - e.y;
            if (dy <= 0 || dy > n.radius * TREE_COVER.height) continue;
            if (Math.abs(e.x - n.x) > n.radius * TREE_COVER.halfWidth) continue;
            return true;
        }
        return false;
    }

    /** Scratch canvas the covering trees are painted into before fading. */
    private coverCanvas: HTMLCanvasElement | null = null;
    private coverInk: Ink | null = null;

    /**
     * The trees somebody is behind, drawn over them and faded so you can see
     * who is there: you are behind the tree, not standing on top of it.
     *
     * Painted whole into a scratch canvas and faded as one piece. Fading each
     * shape as it went down let the lower tiers show through the upper ones,
     * and the outline stayed solid, so it looked like a wireframe of a tree.
     */
    private drawCovering(): void {
        if (this.covering.length === 0) return;
        const main = this.ctx;
        const canvas = main.canvas;
        if (!this.coverCanvas) {
            this.coverCanvas = document.createElement('canvas');
            const c = this.coverCanvas.getContext('2d');
            if (!c) return;
            this.coverInk = new Ink(c);
        }
        const layer = this.coverCanvas;
        if (layer.width !== canvas.width || layer.height !== canvas.height) {
            layer.width = canvas.width;
            layer.height = canvas.height;
        }
        const lctx = layer.getContext('2d')!;
        lctx.setTransform(1, 0, 0, 1, 0, 0);
        lctx.clearRect(0, 0, layer.width, layer.height);
        lctx.setTransform(main.getTransform());

        const ink = this.ink;
        this.ctx = lctx;
        this.ink = this.coverInk!;
        this.ink.start();
        for (const n of this.covering) this.drawNode(n);
        this.ink.stop();
        this.ctx = main;
        this.ink = ink;

        main.save();
        main.setTransform(1, 0, 0, 1, 0, 0);
        main.globalAlpha = TREE_COVER.alpha;
        main.drawImage(layer, 0, 0);
        main.restore();
    }

    private drawNode(n: ResourceNode): void {
        const ctx = this.ctx;
        const def = NODES[n.kind];
        const shake = n.shake > 0 ? Math.sin(n.shake * 70) * 3 : 0;
        ctx.save();
        ctx.translate(n.x + shake, n.y);

        if (n.hp <= 0) {
            ctx.fillStyle = 'rgba(30,34,28,0.55)';
            ctx.beginPath();
            ctx.ellipse(0, 0, n.radius * 0.6, n.radius * 0.35, 0, 0, TAU);
            ctx.fill();
            ctx.restore();
            return;
        }

        ctx.fillStyle = WORLD.shadow;
        ctx.beginPath();
        ctx.ellipse(4, 5, n.radius, n.radius * 0.45, 0, 0, TAU);
        ctx.fill();

        if (n.kind === 'tree') {
            ctx.fillStyle = '#7b4a26';
            ctx.fillRect(-n.radius * 0.22, -n.radius * 0.5, n.radius * 0.44, n.radius);
            const tiers = [
                { y: -n.radius * 2.0, r: n.radius * 1.0 },
                { y: -n.radius * 1.35, r: n.radius * 1.3 },
                { y: -n.radius * 0.65, r: n.radius * 1.55 },
            ];
            // Grain up the trunk, so it is timber rather than a brown bar.
            this.lines(ctx, (d) => {
                d.moveTo(-n.radius * 0.08, -n.radius * 0.42);
                d.lineTo(-n.radius * 0.08, n.radius * 0.42);
                d.moveTo(n.radius * 0.1, -n.radius * 0.3);
                d.lineTo(n.radius * 0.1, n.radius * 0.36);
            });
            // Bottom tier first, crown last: a conifer's top sits in front of
            // the skirt below it, and painting downward buried every crown.
            for (let i = tiers.length - 1; i >= 0; i--) {
                ctx.fillStyle = i % 2 === 0 ? def.color : '#45a94f';
                ctx.beginPath();
                ctx.moveTo(0, tiers[i].y - tiers[i].r);
                ctx.lineTo(-tiers[i].r, tiers[i].y + tiers[i].r * 0.5);
                ctx.lineTo(tiers[i].r, tiers[i].y + tiers[i].r * 0.5);
                ctx.closePath();
                ctx.fill();
                this.markTier(ctx, tiers[i].y, tiers[i].r, n.seed + i);
            }
        } else if (n.kind === 'hemp') {
            ctx.fillStyle = def.color;
            for (let i = 0; i < 5; i++) {
                const a = (i / 5) * TAU;
                ctx.beginPath();
                ctx.ellipse(Math.cos(a) * 5, Math.sin(a) * 4 - 3, 4, 8, a, 0, TAU);
                ctx.fill();
            }
        } else {
            ctx.fillStyle = def.color;
            ctx.beginPath();
            const points = 7;
            for (let i = 0; i < points; i++) {
                const a = (i / points) * TAU;
                const wob = 0.75 + (((n.seed * (i + 3)) % 37) / 37) * 0.45;
                const px = Math.cos(a) * n.radius * wob;
                const py = Math.sin(a) * n.radius * 0.78 * wob;
                if (i === 0) ctx.moveTo(px, py);
                else ctx.lineTo(px, py);
            }
            ctx.closePath();
            ctx.fill();
            // A highlight is light, not an object: inked it became a pale
            // pebble stuck to the front of every boulder.
            this.ink.suspend(() => {
                ctx.fillStyle = 'rgba(255,255,255,0.16)';
                ctx.beginPath();
                ctx.ellipse(
                    -n.radius * 0.2,
                    -n.radius * 0.25,
                    n.radius * 0.4,
                    n.radius * 0.22,
                    -0.4,
                    0,
                    TAU,
                );
                ctx.fill();
            });
            this.markStone(n.radius, n.seed);
            if (n.kind === 'metal_node' || n.kind === 'sulfur_node') {
                ctx.fillStyle = n.kind === 'metal_node' ? '#e0d0b0' : '#f4ec9a';
                for (let i = 0; i < 4; i++) {
                    const a = (((n.seed + i * 23) % 100) / 100) * TAU;
                    ctx.beginPath();
                    ctx.arc(
                        Math.cos(a) * n.radius * 0.45,
                        Math.sin(a) * n.radius * 0.35,
                        2.4,
                        0,
                        TAU,
                    );
                    ctx.fill();
                }
            }
        }

        if (n.hp < n.maxHp) {
            ctx.fillStyle = 'rgba(0,0,0,0.6)';
            ctx.fillRect(-16, n.radius * 0.9, 32, 4);
            ctx.fillStyle = '#c8a35a';
            ctx.fillRect(-16, n.radius * 0.9, 32 * (n.hp / n.maxHp), 4);
        }
        ctx.restore();
    }

    // ------------------------------------------------------------------ npcs

    private npcBuf: Npc[] = [];

    private drawNpcs(game: GameView, b: { x0: number; y0: number; x1: number; y1: number }): void {
        // Reused buffer: allocating a fresh array and closure every frame is a
        // needless load on the collector. See docs/systems/performance.md.
        const list = this.npcBuf;
        list.length = 0;
        for (const n of game.npcs.list) {
            if (n.x < b.x0 - 60 || n.x > b.x1 + 60 || n.y < b.y0 - 60 || n.y > b.y1 + 60) continue;
            if (game.regionHidden(game.build.regionAt(n.x, n.y))) continue;
            list.push(n);
        }
        list.sort(byY);
        for (const n of list) this.drawNpc(n, game);
    }

    private drawNpc(n: Npc, game: GameView): void {
        const ctx = this.ctx;
        const def = NPCS[n.kind];
        const humanoid = n.kind === 'scientist' || n.kind === 'raider' || n.kind === 'gatherer';
        const gait = Math.sin(n.animPhase) * 2.5;
        const body = n.flash > 0 ? '#ffdede' : def.color;
        const dark = n.flash > 0 ? '#ffb0b0' : def.dark;

        ctx.save();
        ctx.translate(n.x, n.y);
        ctx.fillStyle = WORLD.shadowDeep;
        ctx.beginPath();
        ctx.ellipse(0, n.radius * 0.5, n.radius, n.radius * 0.45, 0, 0, TAU);
        ctx.fill();
        ctx.rotate(n.facing + (humanoid ? Math.PI / 2 : 0));

        if (humanoid) {
            drawHuman(ctx, {
                radius: n.radius,
                skin: HUMAN_PALETTE.skin[n.id % HUMAN_PALETTE.skin.length],
                hair: HUMAN_PALETTE.hair[(n.id * 7) % HUMAN_PALETTE.hair.length],
                shirt: def.color,
                legs: def.dark,
                phase: n.animPhase,
                stride: Math.min(1, Math.hypot(n.vx, n.vy) / 90),
                swimming: false,
                holding: !!def.gun,
                // Anyone armed carries a rifle, in the right hand like anyone else.
                held: def.gun ? (c) => drawHeldItem(c, 'rifle') : undefined,
                // A scientist is sealed in a suit.
                hood: n.kind === 'scientist' ? def.color : null,
                hurt: n.flash > 0 ? '#ffdede' : null,
            });
        } else {
            ctx.fillStyle = dark;
            for (const side of [-1, 1]) {
                ctx.fillRect(
                    -n.radius * 0.6,
                    side * n.radius * 0.55 - 2 + gait * side * 0.3,
                    n.radius * 0.5,
                    4,
                );
                ctx.fillRect(
                    n.radius * 0.35,
                    side * n.radius * 0.55 - 2 - gait * side * 0.3,
                    n.radius * 0.5,
                    4,
                );
            }
            ctx.fillStyle = body;
            ctx.beginPath();
            ctx.ellipse(0, 0, n.radius * 1.2, n.radius * 0.85, 0, 0, TAU);
            ctx.fill();
            ctx.fillStyle = dark;
            ctx.beginPath();
            ctx.ellipse(-n.radius * 0.2, 0, n.radius * 0.75, n.radius * 0.45, 0, 0, TAU);
            ctx.fill();
            ctx.fillStyle = body;
            ctx.beginPath();
            ctx.ellipse(n.radius * 1.0, 0, n.radius * 0.55, n.radius * 0.5, 0, 0, TAU);
            ctx.fill();
            ctx.fillStyle = n.kind === 'bear' ? '#ff5c3a' : '#ffd24a';
            for (const side of [-1, 1]) {
                ctx.beginPath();
                ctx.arc(n.radius * 1.2, side * n.radius * 0.22, n.radius * 0.1, 0, TAU);
                ctx.fill();
            }
        }
        ctx.restore();

        // Name everything, always: a bear and a boar are otherwise two brown
        // shapes, and you want to know which one you are walking toward well
        // before it has taken a scratch. The bar underneath is the damage cue.
        const hurt = n.hp < n.maxHp;
        const clan = n.clan > 0 ? game.clanSystem.clanByOwner(n.clan) : undefined;
        this.nameTag(
            clan ? `${def.name} · ${clan.name}` : def.name,
            n.x,
            n.y - n.radius - (hurt ? 24 : 16),
            clan ? clan.color : WORLD.nameTag,
        );
        if (hurt) {
            const w = n.radius * 2.4;
            ctx.fillStyle = WORLD.barTrack;
            ctx.fillRect(n.x - w / 2, n.y - n.radius - 14, w, 4);
            ctx.fillStyle = clan?.color ?? WORLD.hostile;
            ctx.fillRect(n.x - w / 2, n.y - n.radius - 14, w * (n.hp / n.maxHp), 4);
        }
    }

    // ---------------------------------------------------------------- player

    /** Everyone else in the room, drawn from the server's last word on them. */
    private drawRemotePlayers(game: GameView): void {
        if (game.mode !== 'online') return;
        const ctx = this.ctx;
        for (const other of game.net.others.values()) {
            if (!other.alive) continue;
            ctx.save();
            ctx.translate(other.x, other.y);
            ctx.fillStyle = WORLD.shadowDeep;
            ctx.beginPath();
            ctx.ellipse(0, 8, 13, 6, 0, 0, TAU);
            ctx.fill();
            ctx.rotate(other.facing + Math.PI / 2);
            drawHuman(ctx, {
                radius: PLAYER.radius,
                skin: WORLD.skin,
                hair: WORLD.woodDark,
                shirt: '#6a7f9a',
                legs: '#3a3f4a',
                // No walk cycle comes over the wire, so the stride is read off
                // where they are standing: their feet move as they do.
                phase: (other.x + other.y) * 0.08,
                stride: 0.7,
                swimming: false,
                holding: other.held !== null,
                held:
                    other.held && other.held in ITEMS
                        ? (c) => drawHeldItem(c, other.held as ItemId)
                        : undefined,
                hood: null,
                hurt: null,
            });
            ctx.restore();

            ctx.font = `11px ${TYPE.body}`;
            ctx.textAlign = 'center';
            ctx.fillStyle = WORLD.nameTagShadow;
            ctx.fillText(other.name, other.x + 1, other.y - 25);
            ctx.fillStyle = '#e8e2d4';
            ctx.fillText(other.name, other.x, other.y - 26);
            ctx.textAlign = 'left';

            if (other.health < 100) {
                ctx.fillStyle = WORLD.barTrack;
                ctx.fillRect(other.x - 16, other.y - 20, 32, 3.5);
                ctx.fillStyle = WORLD.hostile;
                ctx.fillRect(other.x - 16, other.y - 20, 32 * (other.health / 100), 3.5);
            }
        }
    }

    /**
     * How far a melee swing has turned the item in the hand: back, then
     * through, over the swing's 0.2 seconds. Nothing for a gun.
     */
    private swingTurn(swingAnim: number, id: ItemId): number {
        if (swingAnim <= 0 || !ITEMS[id].melee) return 0;
        const t = 1 - swingAnim / 0.2;
        return (-1.1 + t * 2.2) * 0.8;
    }

    /**
     * The progress arc, at the player's right.
     *
     * Fixed size for every job; only the fill rate differs. See PROGRESS_ARC.
     */
    private drawProgressArc(x: number, y: number, progress: number, color: string): void {
        const ctx = this.ctx;
        const { radius, center, halfAngle, width } = PROGRESS_ARC;
        const from = center - halfAngle;
        const to = center + halfAngle;
        // A non-finite progress means a total of zero somewhere upstream. Draw
        // nothing rather than an empty ring that looks like a stuck job.
        if (!Number.isFinite(progress)) return;
        const done = Math.max(0, Math.min(1, progress));
        ctx.save();
        ctx.lineCap = 'round';
        ctx.lineWidth = width;
        ctx.strokeStyle = 'rgba(8, 10, 12, 0.55)';
        ctx.beginPath();
        ctx.arc(x, y, radius, from, to);
        ctx.stroke();
        ctx.strokeStyle = color;
        ctx.lineWidth = width - 1.5;
        ctx.beginPath();
        ctx.arc(x, y, radius, from, from + (to - from) * done);
        ctx.stroke();
        ctx.restore();
    }

    private drawPlayer(game: GameView): void {
        const p = game.player;
        if (!p.alive) return;
        const ctx = this.ctx;
        const swimming = game.swimming;

        if (swimming) {
            // Expanding rings behind the swimmer, timed to the stroke.
            for (let i = 0; i < 3; i++) {
                const phase = (p.walkPhase * 0.5 + i / 3) % 1;
                ctx.strokeStyle = `rgba(190, 225, 245, ${0.34 * (1 - phase)})`;
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.ellipse(p.x, p.y + 3, 12 + phase * 22, (12 + phase * 22) * 0.45, 0, 0, TAU);
                ctx.stroke();
            }
        }

        ctx.save();
        ctx.translate(p.x, p.y);
        if (!swimming) {
            ctx.fillStyle = WORLD.shadowDeep;
            ctx.beginPath();
            ctx.ellipse(0, 8, 13, 6, 0, 0, TAU);
            ctx.fill();
        }
        ctx.rotate(p.facing + Math.PI / 2);

        const held = game.heldItem();
        if (p.swingAnim > 0 && held && ITEMS[held.id].melee) {
            const t = 1 - p.swingAnim / 0.2;
            ctx.save();
            ctx.rotate(-1.1 + t * 2.2);
            ctx.strokeStyle = 'rgba(255,255,255,0.3)';
            ctx.lineWidth = 3;
            ctx.beginPath();
            ctx.arc(0, 0, 34, -1.0, 0.2);
            ctx.stroke();
            ctx.restore();
        }

        // You wash up with nothing on: no shirt is bare skin, not a green
        // shirt. What you wear is the shirt, so a garment reads as a garment,
        // and the hazmat suit brings its hood.
        const worn = wornId(p);
        drawHuman(ctx, {
            radius: PLAYER.radius,
            skin: WORLD.skin,
            hair: WORLD.woodDark,
            shirt: worn ? ITEMS[worn].color : null,
            legs: worn ? darken(ITEMS[worn].color) : '#4a3a2c',
            phase: p.walkPhase,
            stride: Math.min(1, Math.hypot(p.vx, p.vy) / 90),
            swimming,
            holding: !!held && !swimming,
            held:
                held && !swimming
                    ? (c) => drawHeldItem(c, held.id, this.swingTurn(p.swingAnim, held.id))
                    : undefined,
            hood: worn === 'hazmat' ? ITEMS[worn].color : null,
            hurt: p.hurtFlash > 0 ? '#ff9a9a' : null,
        });

        ctx.restore();

        // One indicator for every timed action, always the same size. Drawn
        // after the restore so it is not rotated with the body.
        const job = game.progressJob;
        if (job) this.drawProgressArc(p.x, p.y, job.progress, job.color);
    }

    // ---------------------------------------------------------------- effects

    private drawGround(game: GameView): void {
        const ctx = this.ctx;
        for (const g of game.interaction.ground) {
            if (game.regionHidden(game.build.regionAt(g.x, g.y))) continue;
            const bob = Math.sin(g.bob) * 2;
            ctx.fillStyle = WORLD.shadow;
            ctx.beginPath();
            ctx.ellipse(g.x, g.y + 5, 7, 3, 0, 0, TAU);
            ctx.fill();
            drawItemIcon(ctx, g.item.id, g.x, g.y - 2 + bob, 20);
        }
    }

    private drawExplosives(game: GameView): void {
        const ctx = this.ctx;
        for (const t of game.combat.thrown) {
            const blink = Math.floor(t.fuse * 8) % 2 === 0;
            drawItemIcon(ctx, t.item, t.x, t.y, 20);
            ctx.fillStyle = blink ? '#ff3a3a' : '#5a1a1a';
            ctx.beginPath();
            ctx.arc(t.x, t.y - 9, 3, 0, TAU);
            ctx.fill();
            ctx.fillStyle = 'rgba(255,90,60,0.9)';
            ctx.font = `bold 11px ${TYPE.display}`;
            ctx.textAlign = 'center';
            ctx.fillText(t.fuse.toFixed(1), t.x, t.y - 24);
            ctx.textAlign = 'left';
        }
    }

    private drawShots(game: GameView): void {
        const ctx = this.ctx;
        ctx.lineCap = 'round';
        for (const s of game.combat.shots) {
            const a = Math.atan2(s.vy, s.vx);
            ctx.strokeStyle = s.color;
            ctx.lineWidth = 2.4;
            ctx.beginPath();
            ctx.moveTo(s.x, s.y);
            ctx.lineTo(s.x - Math.cos(a) * s.length, s.y - Math.sin(a) * s.length);
            ctx.stroke();
        }
        ctx.lineCap = 'butt';
    }

    private drawParticles(game: GameView): void {
        const ctx = this.ctx;
        for (const p of game.particles.items) {
            ctx.globalAlpha = Math.max(0, p.life / p.maxLife);
            ctx.fillStyle = p.color;
            ctx.fillRect(p.x - p.size / 2, p.y - p.size / 2, p.size, p.size);
        }
        ctx.globalAlpha = 1;
    }

    private drawFloatingText(game: GameView): void {
        const ctx = this.ctx;
        ctx.font = `bold 13px ${TYPE.display}`;
        ctx.textAlign = 'center';
        // White lettering inside a black line, whatever the text says. A
        // coloured number is lost against grass, sand or water in turn, and on
        // a printed page a word is ink round paper rather than a tinted glow.
        ctx.lineJoin = 'round';
        ctx.lineWidth = INK.uiWidth;
        ctx.strokeStyle = INK.line;
        for (const t of game.particles.texts) {
            ctx.globalAlpha = Math.min(1, t.life * 1.6);
            ctx.strokeText(t.text, t.x, t.y);
            ctx.fillStyle = '#ffffff';
            ctx.fillText(t.text, t.x, t.y);
        }
        ctx.globalAlpha = 1;
        ctx.textAlign = 'left';
    }

    private drawLighting(game: GameView): void {
        const dark = game.darkness;
        if (dark <= 0.02) return;
        const ctx = this.ctx;
        const cam = game.camera;
        ctx.save();
        ctx.globalCompositeOperation = 'multiply';
        ctx.fillStyle = `rgba(14, 22, 40, ${dark})`;
        ctx.fillRect(0, 0, cam.viewW, cam.viewH);

        ctx.globalCompositeOperation = 'lighter';
        const lights: { x: number; y: number; r: number }[] = [];
        if (game.player.alive) lights.push({ x: game.player.x, y: game.player.y, r: 170 });
        for (const d of game.build.deployables) {
            if (d.lit) lights.push({ x: d.x, y: d.y, r: 250 });
        }
        for (const l of lights) {
            const s = cam.worldToScreen(l.x, l.y);
            const r = l.r * cam.zoom;
            const g = ctx.createRadialGradient(s.x, s.y, 0, s.x, s.y, r);
            g.addColorStop(0, `rgba(255,225,170,${0.5 * dark})`);
            g.addColorStop(0.45, `rgba(255,200,140,${0.2 * dark})`);
            g.addColorStop(1, 'rgba(255,200,140,0)');
            ctx.fillStyle = g;
            ctx.beginPath();
            ctx.arc(s.x, s.y, r, 0, TAU);
            ctx.fill();
        }
        ctx.restore();
    }

    private hpBar(x: number, y: number, w: number, frac: number): void {
        const ctx = this.ctx;
        ctx.fillStyle = WORLD.barTrack;
        ctx.fillRect(x, y, w, 4);
        ctx.fillStyle = frac > 0.4 ? '#8ac96a' : '#c96a4a';
        ctx.fillRect(x, y, w * frac, 4);
    }

    // --------------------------------------------------------------- minimap

    /** Small caption above a health bar. */
    private nameTag(text: string, x: number, y: number, color: string): void {
        const ctx = this.ctx;
        ctx.save();
        ctx.font = `9px ${TYPE.body}`;
        ctx.textAlign = 'center';
        ctx.fillStyle = WORLD.nameTagShadow;
        ctx.fillText(text, x + 1, y + 1);
        ctx.fillStyle = color;
        ctx.fillText(text, x, y);
        ctx.textAlign = 'left';
        ctx.restore();
    }

    /**
     * The island, small or large. `step` is how many biome tiles one painted
     * block covers: coarse is plenty for the corner, the open map wants more.
     */
    drawMinimap(game: GameView, x: number, y: number, size: number, step = 4): void {
        const ctx = this.ctx;
        const scale = size / Math.max(WORLD_W, WORLD_H);
        ctx.save();
        ctx.fillStyle = '#0e120e';
        ctx.fillRect(x, y, size, size);

        // Biome thumbnail.
        for (let by = 0; by < BIOME_H; by += step) {
            for (let bx = 0; bx < BIOME_W; bx += step) {
                ctx.fillStyle = BIOME_COLOR[game.world.biomes[by * BIOME_W + bx]];
                ctx.fillRect(
                    x + bx * BIOME_TILE * scale,
                    y + by * BIOME_TILE * scale,
                    BIOME_TILE * scale * step + 1,
                    BIOME_TILE * scale * step + 1,
                );
            }
        }
        ctx.strokeStyle = '#3f5240';
        ctx.lineWidth = 2;
        ctx.strokeRect(x, y, size, size);

        for (const m of game.world.monuments) {
            const def = MONUMENTS.find((d) => d.id === m.defId);
            ctx.fillStyle = def && def.rads > 0 ? 'rgba(180,230,90,0.8)' : 'rgba(220,220,200,0.8)';
            ctx.beginPath();
            ctx.arc(x + m.x * scale, y + m.y * scale, 4, 0, TAU);
            ctx.fill();
        }
        // Clan holdings are hidden unless recon is on, finding them is the game.
        if (game.revealClans) {
            // Their pieces first, so compound markers sit on top.
            for (const st of game.build.structures) {
                if (st.owner === 0) continue;
                const clan = game.clanSystem.clanByOwner(st.owner);
                ctx.fillStyle = clan?.color ?? WORLD.unclaimedWarm;
                ctx.globalAlpha = 0.85;
                ctx.fillRect(x + st.gx * CELL * scale - 1, y + st.gy * CELL * scale - 1, 2.5, 2.5);
            }
            for (const d of game.build.deployables) {
                if (d.owner === 0) continue;
                const clan = game.clanSystem.clanByOwner(d.owner);
                ctx.fillStyle = clan?.color ?? WORLD.unclaimedWarm;
                ctx.fillRect(x + d.x * scale - 1.5, y + d.y * scale - 1.5, 3, 3);
            }
            ctx.globalAlpha = 1;
            for (const n of game.npcs.list) {
                if (n.clan <= 0) continue;
                const clan = game.clanSystem.clanByOwner(n.clan);
                ctx.fillStyle = clan?.color ?? WORLD.unclaimedWarm;
                ctx.beginPath();
                ctx.arc(x + n.x * scale, y + n.y * scale, 1.6, 0, TAU);
                ctx.fill();
            }
            for (const c of game.clanSystem.clans) {
                if (c.wiped) continue;
                const px = x + c.x * scale;
                const py = y + c.y * scale;
                ctx.strokeStyle = c.color;
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.arc(px, py, 7, 0, TAU);
                ctx.stroke();
                ctx.fillStyle = c.color;
                ctx.beginPath();
                ctx.moveTo(px, py - 4);
                ctx.lineTo(px + 3.5, py + 3);
                ctx.lineTo(px - 3.5, py + 3);
                ctx.closePath();
                ctx.fill();
            }
        }
        for (const d of game.build.deployables) {
            if (d.owner !== 0) continue;
            ctx.fillStyle = WORLD.lamp;
            ctx.fillRect(x + d.x * scale - 2, y + d.y * scale - 2, 4, 4);
        }
        if (game.clanSystem.raid) {
            ctx.strokeStyle = '#ff5a4a';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.arc(
                x + game.clanSystem.raid.x * scale,
                y + game.clanSystem.raid.y * scale,
                8 + Math.sin(performance.now() / 200) * 3,
                0,
                TAU,
            );
            ctx.stroke();
        }
        if (game.player.alive) {
            // You are an arrow, not a dot: the point shows which way you are facing,
            // and the white ring separates you from anything else on the map.
            const px = x + game.player.x * scale;
            const py = y + game.player.y * scale;
            ctx.save();
            ctx.translate(px, py);
            ctx.fillStyle = 'rgba(0,0,0,0.55)';
            ctx.beginPath();
            ctx.arc(0, 0, 7, 0, TAU);
            ctx.fill();
            ctx.strokeStyle = '#ffffff';
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            ctx.arc(0, 0, 7, 0, TAU);
            ctx.stroke();
            ctx.rotate(game.player.facing);
            ctx.fillStyle = '#ffe9a8';
            ctx.beginPath();
            ctx.moveTo(9, 0);
            ctx.lineTo(-4, -5);
            ctx.lineTo(-1.5, 0);
            ctx.lineTo(-4, 5);
            ctx.closePath();
            ctx.fill();
            ctx.strokeStyle = WORLD.barTrack;
            ctx.lineWidth = 1;
            ctx.stroke();
            ctx.restore();
        }
        ctx.restore();
    }
}
