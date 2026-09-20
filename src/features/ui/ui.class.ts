import { TYPE } from 'src/shared/design/constants/type.constant';
import { TextSize } from 'src/features/ui/types/text-size.type';
import { RADIUS } from 'src/shared/design/constants/radius.constant';
import { SPACE } from 'src/shared/design/constants/space.constant';
import { INK } from 'src/shared/design/constants/ink.constant';
import { UI } from 'src/shared/design/constants/ui.constant';

const SIZES: Record<TextSize, number> = {
    title: 20,
    heading: 17,
    body: 13,
    label: 12,
    caption: 11,
    micro: 9,
};

export class Ui {
    constructor(private ctx: CanvasRenderingContext2D) {}

    /**
     * The pen on the interface.
     *
     * Heavier than the world's: the HUD sits on top of everything, and a line
     * that reads as confident on a tree reads as a hairline on a box. Strokes
     * whatever path was built last, half a pen inside the shape so the ink
     * lands on the edge rather than outside it.
     */
    private pen(width: number = INK.uiWidth): void {
        const ctx = this.ctx;
        ctx.strokeStyle = INK.line;
        ctx.lineWidth = width;
        ctx.lineJoin = 'round';
        ctx.stroke();
    }

    roundRect(x: number, y: number, w: number, h: number, r: number): void {
        const ctx = this.ctx;
        ctx.beginPath();
        ctx.moveTo(x + r, y);
        ctx.arcTo(x + w, y, x + w, y + h, r);
        ctx.arcTo(x + w, y + h, x, y + h, r);
        ctx.arcTo(x, y + h, x, y, r);
        ctx.arcTo(x, y, x + w, y, r);
        ctx.closePath();
    }

    /**
     * A panel. In game these are translucent so the world stays visible behind
     * them: you can still see someone walking up on you while your pack is open.
     * Menu screens (title, lobby) pass `opaque` since there is nothing behind.
     */
    panel(x: number, y: number, w: number, h: number, radius = RADIUS.lg, opaque = false): void {
        const ctx = this.ctx;
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.6)';
        ctx.shadowBlur = 34;
        ctx.shadowOffsetY = 10;
        this.roundRect(x, y, w, h, radius);
        ctx.fillStyle = opaque ? UI.surface : UI.surfaceGlass;
        ctx.fill();
        ctx.restore();
        this.roundRect(x, y, w, h, radius);
        this.pen();
    }

    /** A compact card for HUD furniture that sits over the world. */
    card(x: number, y: number, w: number, h: number, alpha = 0.92): void {
        const ctx = this.ctx;
        ctx.save();
        ctx.globalAlpha = alpha;
        ctx.shadowColor = 'rgba(0,0,0,0.35)';
        ctx.shadowBlur = 18;
        ctx.shadowOffsetY = 4;
        this.roundRect(x, y, w, h, RADIUS.md);
        ctx.fillStyle = UI.surface;
        ctx.fill();
        ctx.restore();
        this.roundRect(x, y, w, h, RADIUS.md);
        this.pen();
    }

    /**
     * HUD backing: a soft translucent wash with no border. Used for everything
     * that floats over the world, as opposed to `panel`, which is a reading
     * surface you open on purpose.
     */
    glass(x: number, y: number, w: number, h: number, radius: number = RADIUS.md): void {
        const ctx = this.ctx;
        this.roundRect(x, y, w, h, radius);
        ctx.fillStyle = UI.glass;
        ctx.fill();
    }

    /** Light text with a soft shadow, so it stays readable on any background. */
    textOnDark(
        str: string,
        x: number,
        y: number,
        o: { size?: TextSize; weight?: 400 | 600; color?: string; align?: CanvasTextAlign } = {},
    ): void {
        const ctx = this.ctx;
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.85)';
        ctx.shadowBlur = 4;
        ctx.shadowOffsetY = 1;
        this.text(str, x, y, { ...o, color: o.color ?? UI.onDark });
        ctx.restore();
    }

    /** Meter for the overlay context: thin, dark track, no border. */
    meterOnDark(x: number, y: number, w: number, h: number, value: number, color: string): void {
        const ctx = this.ctx;
        this.roundRect(x, y, w, h, h / 2);
        ctx.fillStyle = UI.track;
        ctx.fill();
        const v = Math.max(0, Math.min(1, value));
        if (v > 0) {
            ctx.save();
            this.roundRect(x, y, w, h, h / 2);
            ctx.clip();
            ctx.fillStyle = color;
            ctx.fillRect(x, y, w * v, h);
            ctx.restore();
        }
        this.roundRect(x, y, w, h, h / 2);
        this.pen(INK.width);
    }

    /** Slot for the overlay context: translucent, hairline only. */
    slotOnDark(
        x: number,
        y: number,
        w: number,
        h: number,
        o: { hovered?: boolean; selected?: boolean } = {},
    ): void {
        const ctx = this.ctx;
        this.roundRect(x, y, w, h, RADIUS.md - 2);
        ctx.fillStyle = o.selected
            ? 'rgba(10,110,235,0.28)'
            : o.hovered
              ? 'rgba(255,255,255,0.10)'
              : 'rgba(18,20,18,0.34)';
        ctx.fill();
        ctx.strokeStyle = o.selected ? 'rgba(120,180,255,0.75)' : UI.glassEdge;
        ctx.lineWidth = 1;
        ctx.stroke();
    }

    divider(x: number, y: number, w: number): void {
        const ctx = this.ctx;
        ctx.strokeStyle = UI.hairline;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x, y + 0.5);
        ctx.lineTo(x + w, y + 0.5);
        ctx.stroke();
    }

    vDivider(x: number, y: number, h: number): void {
        const ctx = this.ctx;
        ctx.strokeStyle = UI.hairline;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x + 0.5, y);
        ctx.lineTo(x + 0.5, y + h);
        ctx.stroke();
    }

    text(
        str: string,
        x: number,
        y: number,
        o: { size?: TextSize; weight?: 400 | 600; color?: string; align?: CanvasTextAlign } = {},
    ): void {
        const ctx = this.ctx;
        const size = SIZES[o.size ?? 'body'];
        ctx.font = `${o.weight === 600 ? '600 ' : ''}${size}px ${TYPE.body}`;
        ctx.fillStyle = o.color ?? UI.ink;
        ctx.textAlign = o.align ?? 'left';
        ctx.fillText(str, x, y);
        ctx.textAlign = 'left';
    }

    /** Text clipped to a width, with an ellipsis when it will not fit. */
    fit(str: string, x: number, y: number, maxW: number, o: Parameters<Ui['text']>[3] = {}): void {
        const ctx = this.ctx;
        const size = SIZES[o.size ?? 'body'];
        ctx.font = `${o.weight === 600 ? '600 ' : ''}${size}px ${TYPE.body}`;
        let out = str;
        if (ctx.measureText(out).width > maxW) {
            while (out.length > 1 && ctx.measureText(`${out}…`).width > maxW)
                out = out.slice(0, -1);
            out = `${out}…`;
        }
        this.text(out, x, y, o);
    }

    wrap(
        str: string,
        x: number,
        y: number,
        maxW: number,
        lineH: number,
        o: Parameters<Ui['text']>[3] = {},
    ): number {
        const ctx = this.ctx;
        const size = SIZES[o.size ?? 'body'];
        ctx.font = `${o.weight === 600 ? '600 ' : ''}${size}px ${TYPE.body}`;
        const words = str.split(' ');
        let line = '';
        let ly = y;
        for (const word of words) {
            const test = line ? `${line} ${word}` : word;
            if (ctx.measureText(test).width > maxW && line) {
                this.text(line, x, ly, o);
                line = word;
                ly += lineH;
            } else {
                line = test;
            }
        }
        if (line) this.text(line, x, ly, o);
        return ly + lineH;
    }

    /** Inset track with a fill. Used for every meter in the game. */
    meter(x: number, y: number, w: number, h: number, value: number, color: string): void {
        const ctx = this.ctx;
        this.roundRect(x, y, w, h, h / 2);
        ctx.fillStyle = UI.surfaceAlt;
        ctx.fill();
        const v = Math.max(0, Math.min(1, value));
        if (v > 0) {
            ctx.save();
            this.roundRect(x, y, w, h, h / 2);
            ctx.clip();
            ctx.fillStyle = color;
            ctx.fillRect(x, y, w * v, h);
            ctx.restore();
        }
        this.roundRect(x, y, w, h, h / 2);
        this.pen(INK.width);
    }

    button(
        x: number,
        y: number,
        w: number,
        h: number,
        label: string,
        o: { enabled?: boolean; hovered?: boolean } = {},
    ): void {
        const ctx = this.ctx;
        const enabled = o.enabled !== false;
        this.roundRect(x, y, w, h, RADIUS.md);
        ctx.fillStyle = !enabled ? UI.buttonOff : o.hovered ? UI.accentHover : UI.accent;
        ctx.fill();
        this.roundRect(x, y, w, h, RADIUS.md);
        this.pen();
        this.text(label, x + w / 2, y + h / 2 - 7, {
            size: 'body',
            weight: 600,
            align: 'center',
            color: enabled ? '#ffffff' : UI.disabled,
        });
    }

    /** An inventory-style well. */
    slot(
        x: number,
        y: number,
        w: number,
        h: number,
        o: { hovered?: boolean; selected?: boolean } = {},
    ): void {
        const ctx = this.ctx;
        this.roundRect(x, y, w, h, RADIUS.md - 1);
        ctx.fillStyle = o.selected ? UI.selected : o.hovered ? UI.hover : UI.surfaceAlt;
        ctx.fill();
        // A selected slot is drawn with a heavier pen rather than a brighter
        // edge: on a printed page weight is how something is emphasised.
        this.roundRect(x, y, w, h, RADIUS.md - 1);
        this.pen(o.selected ? INK.uiWidth * 1.6 : INK.uiWidth);
    }

    scrim(w: number, h: number): void {
        this.ctx.fillStyle = UI.scrim;
        this.ctx.fillRect(0, 0, w, h);
    }

    /** Standard panel header: title, hint, divider. Returns the content top. */
    header(x: number, y: number, w: number, title: string, sub: string, hint: string): number {
        this.text(title, x + SPACE.lg, y + SPACE.lg, { size: 'title', weight: 600 });
        if (sub)
            this.text(sub, x + SPACE.lg, y + SPACE.lg + 26, { size: 'label', color: UI.subtle });
        if (hint)
            this.text(hint, x + w - SPACE.lg, y + SPACE.lg + 26, {
                size: 'label',
                color: UI.subtle,
                align: 'right',
            });
        this.divider(x + 1, y + SPACE.lg + 48, w - 2);
        return y + SPACE.lg + 48;
    }
}
