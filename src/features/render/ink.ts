import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { INK } from 'src/shared/design/constants/ink.constant';

/**
 * Inking, as a property of the canvas rather than of every shape.
 *
 * A comic look is "every solid shape has a line round it". That is one rule
 * applied everywhere, not two thousand separate decisions, so it belongs here
 * rather than copied into every `drawTree`, `drawRock` and `drawEnemy` in the
 * renderer. `fill` and `fillRect` are wrapped once, and from then on anything
 * the renderer fills comes out inked.
 *
 * Doing it this way has a real cost worth naming: a shape drawn with several
 * overlapping fills, which is most of how the bodies here are built, gets a
 * line round each part rather than round the silhouette. On a lumberjack that
 * reads as drawn detail and is what you want. On something that was only ever
 * meant to be a soft blob, it does not, which is what `suspend` is for.
 */
export class Ink {
    /**
     * The pen installed on each canvas. Icon painters are handed a bare
     * context and nothing else, and this is how they find its pen.
     */
    private static readonly pens = new WeakMap<CanvasRenderingContext2D, Ink>();

    /** The pen on this canvas, if it has one. */
    static of(ctx: CanvasRenderingContext2D): Ink | undefined {
        return Ink.pens.get(ctx);
    }

    /** Off by default: the HUD is not a comic panel and should not be inked. */
    private on = false;
    private width: number = INK.width;

    /**
     * Icon mode, set only while a glyph is painted. See `glyph`: in this mode
     * strokes are inked too, not just fills.
     */
    private glyphDetail = 0;
    private rawStroke: (...a: unknown[]) => void;
    private rawStrokeRect: (x: number, y: number, w: number, h: number) => void;

    /**
     * Fills that are shading rather than objects, and are never inked.
     *
     * A shadow and a text scrim have no edge in the real thing either: they are
     * a darkening, not a shape, and a line round one turns it into a drawn
     * pebble lying beside whatever cast it.
     */
    private readonly soft: Set<string>;

    constructor(private ctx: CanvasRenderingContext2D) {
        // Normalised through the canvas, because it rewrites what it is given:
        // `rgba(0,0,0,0.3)` comes back as `rgba(0, 0, 0, 0.3)`. Comparing a
        // token's own spelling against a value read from the context is a test
        // that quietly never passes.
        const was = ctx.fillStyle;
        this.soft = new Set<string>(
            [
                WORLD.shadow,
                WORLD.shadowDeep,
                WORLD.barTrack,
                WORLD.roofHatch,
                WORLD.nameTagShadow,
            ].map((c) => {
                ctx.fillStyle = c;
                return ctx.fillStyle as string;
            }),
        );
        ctx.fillStyle = was;

        const rawFill = ctx.fill.bind(ctx);
        const rawFillRect = ctx.fillRect.bind(ctx);
        this.rawStroke = ctx.stroke.bind(ctx) as (...a: unknown[]) => void;
        this.rawStrokeRect = ctx.strokeRect.bind(ctx);
        Ink.pens.set(ctx, this);

        ctx.fill = ((...args: unknown[]) => {
            if (!this.visible()) return;
            (rawFill as (...a: unknown[]) => void)(...args);
            if (!this.inking()) return;
            this.pen(() => this.rawStroke(...args.filter((a) => a instanceof Path2D)));
        }) as typeof ctx.fill;

        ctx.fillRect = ((x: number, y: number, w: number, h: number) => {
            if (!this.visible()) return;
            rawFillRect(x, y, w, h);
            if (!this.inking()) return;
            this.pen(() => this.rawStrokeRect(x, y, w, h));
        }) as typeof ctx.fillRect;

        ctx.stroke = ((...args: unknown[]) => {
            if (!this.glyphDetail) {
                this.rawStroke(...args);
                return;
            }
            this.inkGlyphStroke(args);
        }) as typeof ctx.stroke;
    }

    /**
     * Paint an item icon in the comic style.
     *
     * Icons are drawn in a unit square scaled up to their size, and a lot of
     * what is in them is strokes rather than fills. Those split two ways, and
     * `detail` is the line between them, in the icon's own units:
     *
     * - Thin strokes are detail: a pile's facets, a ring on a log end. They
     *   become bold black marks, because in a comic every mark is ink.
     * - Thick strokes are shapes that happen to be drawn as lines: an arrow's
     *   shaft, a bow's limb. Turning those black would turn the arrow into a
     *   black line, so they keep their colour and get the pen underneath them
     *   instead, which is how a drawn shape is inked.
     */
    glyph(detail: number, draw: () => void): void {
        const on = this.on;
        const was = this.glyphDetail;
        this.on = true;
        this.glyphDetail = detail;
        draw();
        this.glyphDetail = was;
        this.on = on;
    }

    private inkGlyphStroke(args: unknown[]): void {
        const ctx = this.ctx;
        const style = ctx.strokeStyle;
        const lw = ctx.lineWidth;
        const scale = this.scale();
        if (lw < this.glyphDetail) {
            // Detail: a bold black mark, whatever colour it was drawn in.
            ctx.strokeStyle = INK.line;
            ctx.lineWidth = Math.max(lw, INK.hatchMarkWidth / scale);
            this.rawStroke(...args);
        } else {
            // A shape drawn as a line: ink under it, its own colour on top.
            ctx.strokeStyle = INK.line;
            ctx.lineWidth = lw + (INK.width * 2) / scale;
            this.rawStroke(...args);
            ctx.strokeStyle = style;
            ctx.lineWidth = lw;
            this.rawStroke(...args);
        }
        ctx.strokeStyle = style;
        ctx.lineWidth = lw;
    }

    /**
     * A colour that paints nothing.
     *
     * Matched on the value the canvas gives back, not on the one the code set.
     * Assigning `rgba(0,0,0,0)` and reading `fillStyle` returns
     * `rgba(0, 0, 0, 0)`, with spaces, so comparing against the token's own
     * spelling never matched. Shadows were therefore drawn (invisibly, being
     * transparent) and then inked, which put a black ellipse under every rock,
     * animal, truck and building: the shadows you could see were nothing but
     * their outlines.
     */
    private static readonly INVISIBLE = /,\s*0?\.?0+\s*\)$/;

    /** Whether this particular fill should get a line round it. */
    private inking(): boolean {
        if (!this.on) return false;
        const style = this.ctx.fillStyle;
        if (typeof style !== 'string') return true;
        // Nothing invisible is ever inked, whatever it is: an outline round
        // something that paints nothing is an outline round nothing.
        if (Ink.INVISIBLE.test(style)) return false;
        return !this.soft.has(style);
    }

    /**
     * Whether this fill is worth drawing at all.
     *
     * Filling a transparent shape costs the same as filling an opaque one and
     * shows nothing, so it is skipped rather than drawn.
     */
    private visible(): boolean {
        const style = this.ctx.fillStyle;
        return typeof style !== 'string' || !Ink.INVISIBLE.test(style);
    }

    /** Ink everything filled from here until `stop`. */
    start(): void {
        this.on = true;
    }

    stop(): void {
        this.on = false;
    }

    /** Draw without ink, for things that should stay soft: light, particles, fog. */
    suspend(draw: () => void): void {
        const was = this.on;
        this.on = false;
        draw();
        this.on = was;
    }

    /**
     * How much the current transform magnifies, so the pen can cancel it out.
     *
     * The square root of the determinant is the average of the two axes, which
     * is the honest answer for a stroke: a line round a shape squashed on one
     * axis has no single width, and splitting the difference is what the eye
     * reads as an even pen.
     */
    private scale(): number {
        const t = this.ctx.getTransform();
        const area = Math.abs(t.a * t.d - t.b * t.c);
        return area > 0.000001 ? Math.sqrt(area) : 1;
    }

    /**
     * The outline pen's width in the current transform's units. Marks drawn
     * at this width land on the page exactly as heavy as the outline round
     * them, however far the camera is zoomed.
     */
    penWidth(): number {
        return this.width / this.scale();
    }

    /** Draw at a different pen weight, for whatever the eye should land on. */
    weight(width: number, draw: () => void): void {
        const was = this.width;
        this.width = width;
        draw();
        this.width = was;
    }

    /**
     * Stroke without disturbing what the caller had set up.
     *
     * The renderer sets `strokeStyle` for its own purposes all over the place,
     * and inking must not leak into the next thing it strokes by hand.
     */
    private pen(stroke: () => void): void {
        const ctx = this.ctx;
        const style = ctx.strokeStyle;
        const lw = ctx.lineWidth;
        const alpha = ctx.globalAlpha;
        const join = ctx.lineJoin;
        ctx.strokeStyle = INK.line;
        // A pen has one width on the page, whatever the thing being drawn
        // thinks it is working in. `lineWidth` is in the current transform's
        // units, and item glyphs are drawn in a unit square scaled up to the
        // icon size: at size 18 this pen came out eighteen times too thick, and
        // every stack on the ground turned into a black cloud with a shape
        // faintly visible in it.
        ctx.lineWidth = this.width / this.scale();
        ctx.lineJoin = 'round';
        // Ink is opaque even over something half-transparent: a printed line
        // does not fade because the colour under it is thin.
        ctx.globalAlpha = Math.max(alpha, 0.9);
        this.on = false;
        stroke();
        this.on = true;
        ctx.globalAlpha = alpha;
        ctx.strokeStyle = style;
        ctx.lineWidth = lw;
        ctx.lineJoin = join;
    }
}
