import { ITEMS } from 'src/features/items/constants/items.constant';
import { ItemId } from 'src/features/items/types/item-id.type';
import { Ink } from 'src/features/render/ink';
import { INK } from 'src/shared/design/constants/ink.constant';

/**
 * Every item is a drawn glyph, never a coloured square. See docs/systems/ui.md.
 * Glyphs are drawn inside a unit box centred on (0,0) spanning -0.5..0.5 and
 * scaled to whatever size the caller wants, so one glyph serves the belt, the
 * inventory, the crafting list and ground drops.
 */
export function drawItemIcon(
    ctx: CanvasRenderingContext2D,
    id: ItemId,
    cx: number,
    cy: number,
    size: number,
): void {
    ctx.save();
    ctx.translate(cx, cy);
    ctx.scale(size, size);
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    const draw = GLYPHS[id];
    const paint = (): void => {
        if (draw) draw(ctx);
        else fallback(ctx, ITEMS[id].color);
    };
    // Every icon in the game comes through here, in the pack, on the belt, in
    // the crafting list and lying on the ground, so this is where they are all
    // inked: fills get an outline, thin lines become bold black marks, and a
    // shape drawn as a line gets the pen underneath it. See `Ink.glyph`.
    const pen = Ink.of(ctx);
    if (pen) pen.glyph(INK.glyphDetail, paint);
    else paint();
    ctx.restore();
}

const WOOD = '#8a6034';
const WOOD_DARK = '#5d4022';
const STEEL = '#c0ccd8';
const STEEL_DARK = '#7d8894';
const STONE = '#9aa2aa';
const IRON = '#9c8060';
const CLOTH = '#c8b89a';

function poly(
    ctx: CanvasRenderingContext2D,
    pts: [number, number][],
    fill: string,
    stroke?: string,
): void {
    ctx.beginPath();
    ctx.moveTo(pts[0][0], pts[0][1]);
    for (let i = 1; i < pts.length; i++) ctx.lineTo(pts[i][0], pts[i][1]);
    ctx.closePath();
    ctx.fillStyle = fill;
    ctx.fill();
    if (stroke) {
        ctx.strokeStyle = stroke;
        ctx.lineWidth = 0.045;
        ctx.stroke();
    }
}

function rect(
    ctx: CanvasRenderingContext2D,
    x: number,
    y: number,
    w: number,
    h: number,
    fill: string,
): void {
    ctx.fillStyle = fill;
    ctx.fillRect(x, y, w, h);
}

function circle(
    ctx: CanvasRenderingContext2D,
    x: number,
    y: number,
    r: number,
    fill: string,
): void {
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fillStyle = fill;
    ctx.fill();
}

function handle(
    ctx: CanvasRenderingContext2D,
    x0: number,
    y0: number,
    x1: number,
    y1: number,
    w = 0.07,
): void {
    ctx.strokeStyle = WOOD;
    ctx.lineWidth = w;
    ctx.beginPath();
    ctx.moveTo(x0, y0);
    ctx.lineTo(x1, y1);
    ctx.stroke();
}

function fallback(ctx: CanvasRenderingContext2D, color: string): void {
    circle(ctx, 0, 0, 0.3, color);
}

/** A loose pile of chunks, used for the raw resources. */
function pile(ctx: CanvasRenderingContext2D, fill: string, edge: string): void {
    const bits: [number, number, number][] = [
        [-0.2, 0.12, 0.17],
        [0.16, 0.14, 0.15],
        [-0.02, -0.02, 0.19],
        [0.1, -0.18, 0.13],
    ];
    for (const [x, y, r] of bits) {
        ctx.beginPath();
        ctx.moveTo(x - r, y + r * 0.6);
        ctx.lineTo(x - r * 0.4, y - r);
        ctx.lineTo(x + r * 0.8, y - r * 0.5);
        ctx.lineTo(x + r, y + r * 0.7);
        ctx.closePath();
        ctx.fillStyle = fill;
        ctx.fill();
        ctx.strokeStyle = edge;
        ctx.lineWidth = 0.035;
        ctx.stroke();
    }
}

type Glyph = (ctx: CanvasRenderingContext2D) => void;

const GLYPHS: Partial<Record<ItemId, Glyph>> = {
    // ---- resources
    wood: (ctx) => {
        // Stacked logs, end on.
        for (const [x, y] of [
            [-0.17, 0.1],
            [0.17, 0.1],
            [0, -0.16],
        ] as [number, number][]) {
            circle(ctx, x, y, 0.19, WOOD);
            circle(ctx, x, y, 0.11, '#a97a44');
            circle(ctx, x, y, 0.045, WOOD_DARK);
        }
    },
    stone: (ctx) => pile(ctx, STONE, '#6f767d'),
    metal_ore: (ctx) => {
        pile(ctx, IRON, '#6d5840');
        circle(ctx, -0.05, -0.05, 0.05, '#d8c9a8');
        circle(ctx, 0.14, 0.13, 0.04, '#d8c9a8');
    },
    sulfur_ore: (ctx) => {
        pile(ctx, '#c9c05a', '#8a832f');
        circle(ctx, -0.04, -0.04, 0.05, '#f2ea9a');
    },
    metal: (ctx) => {
        // Flat fragments.
        poly(
            ctx,
            [
                [-0.3, 0.05],
                [-0.05, -0.12],
                [0.12, 0.02],
                [-0.1, 0.2],
            ],
            STEEL,
            STEEL_DARK,
        );
        poly(
            ctx,
            [
                [0.02, -0.06],
                [0.28, -0.18],
                [0.32, 0.06],
                [0.12, 0.12],
            ],
            '#dbe4ec',
            STEEL_DARK,
        );
    },
    sulfur: (ctx) => {
        poly(
            ctx,
            [
                [-0.26, 0.1],
                [-0.08, -0.16],
                [0.1, 0.04],
                [-0.06, 0.22],
            ],
            '#e8dc6a',
            '#a89a2f',
        );
        poly(
            ctx,
            [
                [0.04, -0.02],
                [0.28, -0.14],
                [0.3, 0.1],
                [0.1, 0.16],
            ],
            '#f2ea9a',
            '#a89a2f',
        );
    },
    charcoal: (ctx) => pile(ctx, '#4a4a4a', '#2a2a2a'),
    cloth: (ctx) => {
        // Folded bolt of fabric.
        poly(
            ctx,
            [
                [-0.3, -0.14],
                [0.3, -0.22],
                [0.3, 0.06],
                [-0.3, 0.14],
            ],
            CLOTH,
            '#9a8b6e',
        );
        poly(
            ctx,
            [
                [-0.3, 0.06],
                [0.3, -0.02],
                [0.3, 0.2],
                [-0.3, 0.28],
            ],
            '#dccbaa',
            '#9a8b6e',
        );
    },
    leather: (ctx) => {
        ctx.lineJoin = 'round';
        ctx.strokeStyle = '#5d3d22';
        ctx.lineWidth = 0.035;
        // The piece itself. Nothing on it is a ruled line: a cut of leather
        // holds no straight edge, and the square version read as a crate.
        ctx.fillStyle = '#a3714a';
        ctx.beginPath();
        ctx.moveTo(-0.3, -0.18);
        ctx.quadraticCurveTo(-0.02, -0.26, 0.18, -0.18);
        ctx.lineTo(0.3, 0.06);
        ctx.quadraticCurveTo(0.02, 0.3, -0.3, 0.18);
        ctx.closePath();
        ctx.fill();
        ctx.stroke();
        // The far corner curling back, its underside lighter. This is what
        // says leather at pack size, where the stitching is a few pixels.
        ctx.fillStyle = '#c9935c';
        ctx.beginPath();
        ctx.moveTo(0.18, -0.18);
        ctx.quadraticCurveTo(0.34, -0.14, 0.3, 0.06);
        ctx.quadraticCurveTo(0.2, -0.04, 0.18, -0.18);
        ctx.closePath();
        ctx.fill();
        ctx.stroke();
        // Stitching along the near edge, and one crease across the flat.
        ctx.lineWidth = 0.028;
        ctx.beginPath();
        for (let i = 0; i < 4; i++) {
            const t = i * 0.12;
            ctx.moveTo(-0.22 + t, 0.16);
            ctx.lineTo(-0.17 + t, 0.145);
        }
        ctx.moveTo(-0.2, -0.04);
        ctx.quadraticCurveTo(-0.02, 0.02, 0.12, -0.06);
        ctx.stroke();
    },
    bone: (ctx) => {
        ctx.strokeStyle = '#ddd6c0';
        ctx.lineWidth = 0.13;
        ctx.beginPath();
        ctx.moveTo(-0.2, 0.18);
        ctx.lineTo(0.2, -0.18);
        ctx.stroke();
        circle(ctx, -0.24, 0.18, 0.09, '#eee7d2');
        circle(ctx, -0.16, 0.25, 0.08, '#eee7d2');
        circle(ctx, 0.24, -0.18, 0.09, '#eee7d2');
        circle(ctx, 0.16, -0.25, 0.08, '#eee7d2');
    },
    scrap: (ctx) => {
        // Bent, riveted plate.
        poly(
            ctx,
            [
                [-0.3, -0.06],
                [-0.05, -0.24],
                [0.3, -0.1],
                [0.22, 0.2],
                [-0.16, 0.26],
            ],
            '#9a8c72',
            '#6a5f4b',
        );
        circle(ctx, -0.14, -0.02, 0.035, '#6a5f4b');
        circle(ctx, 0.12, 0.06, 0.035, '#6a5f4b');
    },
    gunpowder: (ctx) => {
        // Pouch of powder.
        poly(
            ctx,
            [
                [-0.22, 0.3],
                [-0.26, -0.02],
                [-0.1, -0.2],
                [0.1, -0.2],
                [0.26, -0.02],
                [0.22, 0.3],
            ],
            '#6d6a5a',
            '#43413a',
        );
        rect(ctx, -0.08, -0.3, 0.16, 0.12, '#4a483f');
        circle(ctx, -0.06, 0.08, 0.03, '#3a382f');
        circle(ctx, 0.06, 0.16, 0.03, '#3a382f');
    },
    animal_fat: (ctx) => {
        // Trimmed slab of fat: pale, marbled, faintly greasy.
        poly(
            ctx,
            [
                [-0.26, -0.06],
                [-0.14, -0.24],
                [0.16, -0.26],
                [0.28, -0.02],
                [0.16, 0.24],
                [-0.18, 0.22],
            ],
            '#f0e6c8',
            '#c2b48c',
        );
        circle(ctx, -0.04, -0.06, 0.07, '#fff8e4');
        circle(ctx, 0.11, 0.09, 0.045, '#fff8e4');
        circle(ctx, -0.12, 0.11, 0.035, '#d9cca6');
    },
    lowgrade: (ctx) => {
        // Jerry can.
        poly(
            ctx,
            [
                [-0.22, -0.2],
                [0.2, -0.2],
                [0.2, 0.28],
                [-0.22, 0.28],
            ],
            '#c9a94a',
            '#8a7226',
        );
        rect(ctx, -0.08, -0.3, 0.14, 0.1, '#8a7226');
        ctx.strokeStyle = '#8a7226';
        ctx.lineWidth = 0.04;
        ctx.beginPath();
        ctx.moveTo(-0.14, -0.1);
        ctx.lineTo(0.12, 0.2);
        ctx.stroke();
    },
    water: (ctx) => {
        poly(
            ctx,
            [
                [0, -0.3],
                [0.22, 0.02],
                [0.16, 0.22],
                [-0.16, 0.22],
                [-0.22, 0.02],
            ],
            '#5aa8d8',
            '#2f7aa8',
        );
        circle(ctx, -0.07, 0.06, 0.05, '#a8d8f0');
    },
    meat_raw: (ctx) => {
        poly(
            ctx,
            [
                [-0.26, 0.04],
                [-0.12, -0.22],
                [0.16, -0.24],
                [0.28, 0.02],
                [0.12, 0.26],
                [-0.16, 0.24],
            ],
            '#b45c5c',
            '#7d3535',
        );
        circle(ctx, 0.02, 0.0, 0.09, '#d78585');
        rect(ctx, -0.05, 0.16, 0.1, 0.14, '#eee7d2');
    },
    meat_cooked: (ctx) => {
        poly(
            ctx,
            [
                [-0.26, 0.04],
                [-0.12, -0.22],
                [0.16, -0.24],
                [0.28, 0.02],
                [0.12, 0.26],
                [-0.16, 0.24],
            ],
            '#8a5a34',
            '#573418',
        );
        circle(ctx, 0.02, 0.0, 0.09, '#a87244');
        rect(ctx, -0.05, 0.16, 0.1, 0.14, '#eee7d2');
    },

    // ---- tools
    rock: (ctx) => {
        poly(
            ctx,
            [
                [-0.28, 0.06],
                [-0.14, -0.22],
                [0.14, -0.26],
                [0.3, -0.02],
                [0.2, 0.24],
                [-0.12, 0.26],
            ],
            '#8b8b8b',
            '#5f5f5f',
        );
        poly(
            ctx,
            [
                [-0.1, -0.12],
                [0.08, -0.16],
                [0.12, 0.0],
                [-0.04, 0.06],
            ],
            '#a5a5a5',
        );
    },
    hatchet: (ctx) => {
        handle(ctx, 0.12, 0.32, -0.02, -0.16);
        poly(
            ctx,
            [
                [-0.04, -0.3],
                [0.2, -0.2],
                [0.16, 0.02],
                [-0.06, -0.06],
            ],
            STEEL,
            STEEL_DARK,
        );
        poly(
            ctx,
            [
                [-0.04, -0.3],
                [-0.2, -0.22],
                [-0.2, -0.02],
                [-0.06, -0.06],
            ],
            '#e2ebf3',
            STEEL_DARK,
        );
    },
    pickaxe: (ctx) => {
        handle(ctx, 0.0, 0.34, 0.0, -0.1);
        ctx.strokeStyle = STEEL;
        ctx.lineWidth = 0.1;
        ctx.beginPath();
        ctx.moveTo(-0.3, -0.02);
        ctx.quadraticCurveTo(0, -0.34, 0.3, -0.02);
        ctx.stroke();
        poly(
            ctx,
            [
                [-0.3, -0.02],
                [-0.34, -0.12],
                [-0.22, -0.1],
            ],
            STEEL_DARK,
        );
        poly(
            ctx,
            [
                [0.3, -0.02],
                [0.34, -0.12],
                [0.22, -0.1],
            ],
            STEEL_DARK,
        );
    },
    hammer: (ctx) => {
        handle(ctx, 0.0, 0.34, 0.0, -0.12);
        rect(ctx, -0.24, -0.3, 0.48, 0.2, '#d8c08a');
        rect(ctx, -0.24, -0.3, 0.12, 0.2, '#b09a63');
        ctx.strokeStyle = '#8f7c4e';
        ctx.lineWidth = 0.03;
        ctx.strokeRect(-0.24, -0.3, 0.48, 0.2);
    },
    building_plan: (ctx) => {
        // Rolled blueprint with a framing square.
        poly(
            ctx,
            [
                [-0.28, -0.24],
                [0.24, -0.24],
                [0.24, 0.28],
                [-0.28, 0.28],
            ],
            '#f2efe6',
            '#b9b3a2',
        );
        ctx.strokeStyle = '#0a6eeb';
        ctx.lineWidth = 0.045;
        ctx.strokeRect(-0.17, -0.13, 0.3, 0.3);
        ctx.beginPath();
        ctx.moveTo(-0.17, 0.02);
        ctx.lineTo(0.13, 0.02);
        ctx.stroke();
    },

    // ---- weapons
    spear: (ctx) => {
        handle(ctx, -0.24, 0.32, 0.14, -0.14, 0.055);
        poly(
            ctx,
            [
                [0.14, -0.14],
                [0.3, -0.34],
                [0.24, -0.06],
            ],
            '#cfd8e0',
            '#8f9aa4',
        );
    },
    bow: (ctx) => {
        ctx.strokeStyle = WOOD;
        ctx.lineWidth = 0.075;
        ctx.beginPath();
        ctx.arc(0.06, 0, 0.3, Math.PI * 0.62, Math.PI * 1.38);
        ctx.stroke();
        ctx.strokeStyle = '#e6e2d2';
        ctx.lineWidth = 0.028;
        ctx.beginPath();
        ctx.moveTo(-0.11, -0.28);
        ctx.lineTo(-0.11, 0.28);
        ctx.stroke();
    },
    arrow: (ctx) => {
        ctx.strokeStyle = WOOD;
        ctx.lineWidth = 0.05;
        ctx.beginPath();
        ctx.moveTo(-0.26, 0.26);
        ctx.lineTo(0.18, -0.18);
        ctx.stroke();
        poly(
            ctx,
            [
                [0.18, -0.18],
                [0.32, -0.32],
                [0.26, -0.1],
            ],
            '#cfd8e0',
        );
        poly(
            ctx,
            [
                [-0.26, 0.26],
                [-0.32, 0.1],
                [-0.14, 0.18],
            ],
            '#8fae6a',
        );
    },
    revolver: (ctx) => {
        rect(ctx, -0.1, -0.12, 0.38, 0.11, '#4a4a52');
        circle(ctx, -0.04, -0.02, 0.11, '#6a6a74');
        circle(ctx, -0.04, -0.02, 0.045, '#3a3a42');
        poly(
            ctx,
            [
                [-0.14, -0.02],
                [-0.04, -0.02],
                [-0.1, 0.28],
                [-0.24, 0.26],
            ],
            WOOD,
            WOOD_DARK,
        );
        ctx.strokeStyle = '#4a4a52';
        ctx.lineWidth = 0.035;
        ctx.beginPath();
        ctx.arc(-0.02, 0.12, 0.08, Math.PI * 0.1, Math.PI * 0.9);
        ctx.stroke();
    },
    rifle: (ctx) => {
        rect(ctx, -0.34, -0.06, 0.66, 0.09, '#4a4a52');
        rect(ctx, 0.1, -0.1, 0.24, 0.05, '#6a6a74');
        poly(
            ctx,
            [
                [-0.34, -0.06],
                [-0.2, -0.06],
                [-0.16, 0.16],
                [-0.34, 0.2],
            ],
            WOOD,
            WOOD_DARK,
        );
        rect(ctx, -0.06, 0.03, 0.06, 0.16, '#4a4a52');
        rect(ctx, -0.14, 0.14, 0.1, 0.16, '#3a3a42');
    },
    // An AK in profile, barrel to the right: wooden stock, dark receiver, the
    // curved magazine that makes it an AK, wooden handguard, a gas tube over
    // the barrel and the front sight post.
    ak47: (ctx) => {
        poly(
            ctx,
            [
                [-0.44, -0.07],
                [-0.2, -0.05],
                [-0.2, 0.03],
                [-0.42, 0.12],
            ],
            '#9a5a2e',
            WOOD_DARK,
        );
        rect(ctx, -0.2, -0.08, 0.26, 0.11, '#3c3c44');
        poly(
            ctx,
            [
                [-0.02, 0.03],
                [0.06, 0.03],
                [0.1, 0.3],
                [0.02, 0.32],
            ],
            '#35353c',
            '#1f1f24',
        );
        poly(
            ctx,
            [
                [-0.16, 0.03],
                [-0.08, 0.03],
                [-0.12, 0.18],
                [-0.19, 0.17],
            ],
            '#6e4122',
            WOOD_DARK,
        );
        rect(ctx, 0.06, -0.07, 0.18, 0.09, '#b0703a');
        rect(ctx, 0.06, -0.11, 0.26, 0.035, '#4a4a52');
        rect(ctx, 0.24, -0.05, 0.22, 0.035, '#4a4a52');
        rect(ctx, 0.4, -0.11, 0.025, 0.07, '#4a4a52');
    },
    // A launcher in profile, muzzle to the right: the tube, a flared rear
    // end, a grip and a trigger guard under it, and a sight on top.
    rocket_launcher: (ctx) => {
        rect(ctx, -0.42, -0.1, 0.84, 0.16, '#5f6b4a');
        poly(
            ctx,
            [
                [-0.42, -0.13],
                [-0.3, -0.1],
                [-0.3, 0.06],
                [-0.42, 0.09],
            ],
            '#4a5439',
            '#2f3624',
        );
        rect(ctx, 0.36, -0.12, 0.07, 0.2, '#3c3c44');
        rect(ctx, -0.04, 0.06, 0.08, 0.2, '#3c3c44');
        rect(ctx, 0.12, 0.06, 0.07, 0.14, '#3c3c44');
        rect(ctx, -0.1, -0.2, 0.14, 0.1, '#3c3c44');
    },
    // A rocket, nose to the right: red warhead, grey body, tail fins.
    rocket: (ctx) => {
        rect(ctx, -0.24, -0.07, 0.42, 0.14, '#9aa2aa');
        poly(
            ctx,
            [
                [0.18, -0.07],
                [0.36, 0],
                [0.18, 0.07],
            ],
            '#c8433a',
            '#7a1c1c',
        );
        poly(
            ctx,
            [
                [-0.24, -0.07],
                [-0.36, -0.18],
                [-0.3, -0.07],
            ],
            '#6f767d',
        );
        poly(
            ctx,
            [
                [-0.24, 0.07],
                [-0.36, 0.18],
                [-0.3, 0.07],
            ],
            '#6f767d',
        );
        ctx.strokeStyle = '#6f767d';
        ctx.lineWidth = 0.03;
        ctx.beginPath();
        ctx.moveTo(0.02, -0.07);
        ctx.lineTo(0.02, 0.07);
        ctx.stroke();
    },
    pistol_ammo: (ctx) => {
        for (const x of [-0.18, 0, 0.18]) {
            rect(ctx, x - 0.055, -0.05, 0.11, 0.3, '#c9a94a');
            poly(
                ctx,
                [
                    [x - 0.055, -0.05],
                    [x, -0.24],
                    [x + 0.055, -0.05],
                ],
                '#d8c08a',
            );
        }
    },
    rifle_ammo: (ctx) => {
        for (const x of [-0.16, 0.06]) {
            rect(ctx, x - 0.06, -0.1, 0.12, 0.36, '#b8a04a');
            poly(
                ctx,
                [
                    [x - 0.06, -0.1],
                    [x, -0.32],
                    [x + 0.06, -0.1],
                ],
                '#cfe2f5',
            );
        }
    },

    // ---- consumables
    bandage: (ctx) => {
        poly(
            ctx,
            [
                [-0.3, -0.12],
                [0.3, -0.24],
                [0.3, 0.06],
                [-0.3, 0.18],
            ],
            '#f0ece2',
            '#c3bcae',
        );
        rect(ctx, -0.07, -0.19, 0.14, 0.055, '#c2452f');
        rect(ctx, -0.0425, -0.235, 0.085, 0.145, '#c2452f');
    },
    medkit: (ctx) => {
        rect(ctx, -0.07, -0.3, 0.14, 0.4, '#e8e8ee');
        rect(ctx, -0.05, -0.26, 0.1, 0.28, '#d84a6a');
        rect(ctx, -0.03, 0.1, 0.06, 0.14, '#9aa2aa');
        ctx.strokeStyle = '#9aa2aa';
        ctx.lineWidth = 0.03;
        ctx.beginPath();
        ctx.moveTo(0, 0.24);
        ctx.lineTo(0, 0.32);
        ctx.stroke();
    },

    // ---- clothing
    clothing: (ctx) => {
        poly(
            ctx,
            [
                [-0.3, -0.14],
                [-0.12, -0.26],
                [0.12, -0.26],
                [0.3, -0.14],
                [0.2, -0.02],
                [0.2, 0.28],
                [-0.2, 0.28],
                [-0.2, -0.02],
            ],
            '#a3714a',
            '#6d4a2c',
        );
        ctx.strokeStyle = '#6d4a2c';
        ctx.lineWidth = 0.03;
        ctx.beginPath();
        ctx.moveTo(0, -0.26);
        ctx.lineTo(0, 0.28);
        ctx.stroke();
    },
    hazmat: (ctx) => {
        poly(
            ctx,
            [
                [-0.28, -0.1],
                [-0.1, -0.26],
                [0.1, -0.26],
                [0.28, -0.1],
                [0.18, 0.0],
                [0.18, 0.28],
                [-0.18, 0.28],
                [-0.18, 0.0],
            ],
            '#5ac8a0',
            '#2f8a6a',
        );
        circle(ctx, 0, -0.14, 0.11, '#2f3a3a');
        circle(ctx, -0.03, -0.17, 0.04, '#8fe8c8');
    },

    // ---- deployables
    campfire: (ctx) => {
        ctx.strokeStyle = WOOD;
        ctx.lineWidth = 0.07;
        ctx.beginPath();
        ctx.moveTo(-0.26, 0.24);
        ctx.lineTo(0.26, 0.12);
        ctx.moveTo(0.26, 0.24);
        ctx.lineTo(-0.26, 0.12);
        ctx.stroke();
        poly(
            ctx,
            [
                [0, -0.3],
                [0.16, -0.02],
                [0.1, 0.12],
                [-0.1, 0.12],
                [-0.16, -0.02],
            ],
            '#ff8c2e',
        );
        poly(
            ctx,
            [
                [0, -0.14],
                [0.08, 0.0],
                [0, 0.1],
                [-0.08, 0.0],
            ],
            '#ffd98a',
        );
    },
    furnace: (ctx) => {
        poly(
            ctx,
            [
                [-0.28, 0.3],
                [-0.2, -0.24],
                [0.2, -0.24],
                [0.28, 0.3],
            ],
            '#6b5540',
            '#3f3226',
        );
        rect(ctx, -0.12, 0.04, 0.24, 0.26, '#241f18');
        poly(
            ctx,
            [
                [0, 0.1],
                [0.08, 0.2],
                [0, 0.28],
                [-0.08, 0.2],
            ],
            '#ff8c2e',
        );
        rect(ctx, -0.2, -0.3, 0.4, 0.07, '#4f4238');
    },
    tool_cupboard: (ctx) => {
        rect(ctx, -0.24, -0.26, 0.48, 0.54, '#7a6034');
        rect(ctx, -0.24, -0.26, 0.48, 0.12, '#c9a227');
        ctx.strokeStyle = '#4a3a20';
        ctx.lineWidth = 0.035;
        ctx.strokeRect(-0.24, -0.26, 0.48, 0.54);
        circle(ctx, 0.13, 0.06, 0.035, '#c9a227');
    },
    wooden_box: (ctx) => {
        rect(ctx, -0.28, -0.2, 0.56, 0.44, WOOD);
        ctx.strokeStyle = WOOD_DARK;
        ctx.lineWidth = 0.04;
        ctx.strokeRect(-0.28, -0.2, 0.56, 0.44);
        ctx.beginPath();
        ctx.moveTo(-0.28, -0.2);
        ctx.lineTo(0.28, 0.24);
        ctx.moveTo(0.28, -0.2);
        ctx.lineTo(-0.28, 0.24);
        ctx.stroke();
    },
    sleeping_bag: (ctx) => {
        poly(
            ctx,
            [
                [-0.3, -0.16],
                [0.3, -0.16],
                [0.3, 0.18],
                [-0.3, 0.18],
            ],
            '#a05a5a',
            '#6d3838',
        );
        rect(ctx, -0.26, -0.12, 0.2, 0.26, '#c98a8a');
        ctx.strokeStyle = '#6d3838';
        ctx.lineWidth = 0.03;
        ctx.beginPath();
        ctx.moveTo(0.06, -0.16);
        ctx.lineTo(0.06, 0.18);
        ctx.stroke();
    },
    lock: (ctx) => {
        ctx.strokeStyle = '#9aa2aa';
        ctx.lineWidth = 0.06;
        ctx.beginPath();
        ctx.arc(0, -0.08, 0.14, Math.PI, 0);
        ctx.stroke();
        rect(ctx, -0.2, -0.08, 0.4, 0.32, '#d8c08a');
        ctx.strokeStyle = '#8f7c4e';
        ctx.lineWidth = 0.03;
        ctx.strokeRect(-0.2, -0.08, 0.4, 0.32);
        circle(ctx, 0, 0.08, 0.05, '#5f5340');
    },

    // ---- explosives
    satchel: (ctx) => {
        poly(
            ctx,
            [
                [-0.26, -0.12],
                [0.26, -0.12],
                [0.22, 0.26],
                [-0.22, 0.26],
            ],
            '#c9702e',
            '#8a4a1c',
        );
        rect(ctx, -0.26, -0.12, 0.52, 0.08, '#8a4a1c');
        ctx.strokeStyle = '#3a3a42';
        ctx.lineWidth = 0.035;
        ctx.beginPath();
        ctx.moveTo(0.06, -0.12);
        ctx.quadraticCurveTo(0.22, -0.3, 0.06, -0.34);
        ctx.stroke();
    },
    c4: (ctx) => {
        poly(
            ctx,
            [
                [-0.28, -0.1],
                [0.28, -0.1],
                [0.28, 0.24],
                [-0.28, 0.24],
            ],
            '#d8d0c0',
            '#9a9384',
        );
        rect(ctx, -0.28, 0.0, 0.56, 0.08, '#c2452f');
        rect(ctx, -0.06, -0.28, 0.12, 0.18, '#4a4a52');
        circle(ctx, 0, -0.3, 0.05, '#c2452f');
    },
};
