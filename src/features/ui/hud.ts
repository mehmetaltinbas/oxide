import { CELL } from 'src/features/building/constants/cell.constant';
import { Vital } from 'src/features/ui/types/vital.type';
import { drawVitalIcon } from 'src/features/ui/utils/draw-vital-icon.util';
import { TYPE } from 'src/shared/design/constants/type.constant';
import { isFullscreen, toggleFullscreen } from 'src/shared/core/fullscreen';
import { CONTROLS_REFERENCE } from 'src/features/ui/constants/controls-reference.constant';
import { SettingsTab } from 'src/features/ui/types/settings-tab.type';
import { Game } from 'src/app/game';
import { BuildKind } from 'src/features/building/types/build-kind.type';
import { CLAN_SKILLS } from 'src/features/clans/constants/clan-skills.constant';
import { CRAFT_QUEUE_MAX } from 'src/features/crafting/constants/craft-queue-max.constant';
import { RECIPES } from 'src/features/crafting/constants/recipes.constant';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { drawItemIcon } from 'src/features/items/icons';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { ItemStack } from 'src/features/items/types/item-stack.interface';
import { canCraft } from 'src/features/items/utils/can-craft.util';
import { countAcross } from 'src/features/items/utils/count-across.util';
import { isBeltItem } from 'src/features/items/utils/is-belt-item.util';
import { moveStack } from 'src/features/items/utils/move-stack.util';
import { defaultServerUrl } from 'src/features/net/server-url.util';
import { Renderer } from 'src/features/render/renderer';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { Ui } from 'src/features/ui/ui.class';
import { RADIUS } from 'src/shared/design/constants/radius.constant';
import { SPACE } from 'src/shared/design/constants/space.constant';
import { UI } from 'src/shared/design/constants/ui.constant';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';

/** Light, minimal palette for the crafting panel. */
/**
 * Legacy alias for the panel palette. Everything here now points at the shared
 * tokens in `ui.ts`, so the panels can never drift away from the rest of the
 * interface again. See docs/systems/ui.md.
 */

interface CraftCategory {
    id: string;
    label: string;
    match: (id: ItemId) => boolean;
}

/** Layered lists: category, then item, then detail. */
const CRAFT_CATEGORIES: CraftCategory[] = [
    { id: 'tools', label: 'Tools', match: (id) => ITEMS[id].category === 'tool' },
    { id: 'weapons', label: 'Weapons', match: (id) => ITEMS[id].category === 'weapon' },
    { id: 'ammo', label: 'Ammunition', match: (id) => ITEMS[id].category === 'ammo' },
    { id: 'build', label: 'Construction', match: (id) => ITEMS[id].category === 'deployable' },
    { id: 'medical', label: 'Medical', match: (id) => ITEMS[id].category === 'consumable' },
    { id: 'attire', label: 'Attire', match: (id) => ITEMS[id].category === 'clothing' },
    { id: 'resources', label: 'Resources', match: (id) => ITEMS[id].category === 'resource' },
    { id: 'explosives', label: 'Explosives', match: (id) => ITEMS[id].category === 'explosive' },
];

interface Rect {
    x: number;
    y: number;
    w: number;
    h: number;
}

function hit(r: Rect, mx: number, my: number): boolean {
    return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
}

const SANDBOX_AMOUNTS = [1, 10, 100, 1000];
const SANDBOX_ORDER = [
    'weapon',
    'ammo',
    'tool',
    'consumable',
    'clothing',
    'deployable',
    'resource',
];

export class Hud {
    private ui: Ui;
    private sandboxAmount = 10;
    /** Craft-amount text field: whether it has focus, and what has been typed. */
    private amountFocus = false;
    private amountText = '';

    constructor(
        private ctx: CanvasRenderingContext2D,
        private renderer: Renderer,
    ) {
        this.ui = new Ui(ctx);
    }

    draw(game: Game, w: number, h: number): void {
        const ctx = this.ctx;
        ctx.save();
        ctx.textBaseline = 'top';

        if (game.phase === 'title') {
            this.drawTitle(game, w, h);
            ctx.restore();
            return;
        }
        if (game.phase === 'lobby') {
            this.drawLobby(game, w, h);
            ctx.restore();
            return;
        }

        game.uiHover = game.panel !== 'none' || game.paused;

        this.drawTopBar(game, w);
        this.drawVitals(game, h);
        this.drawHotbar(game, w, h);
        this.drawMapPanel(game, w);
        this.drawContextHints(game, w, h);

        if (game.panel === 'inventory') this.drawInventory(game, w, h);
        if (game.panel === 'container') this.drawContainerPanel(game, w, h);
        if (game.panel === 'craft') this.drawCraft(game, w, h);
        if (game.panel === 'sandbox') this.drawSandbox(game, w, h);
        if (game.panel === 'help') this.drawHelp(w, h);
        if (game.panel === 'map') this.drawIslandMap(game, w, h);
        if (game.paused && game.panel !== 'help') this.drawSettings(game, w, h);
        if (game.phase === 'dead') this.drawDeath(game, w, h);
        this.drawDragged(game);

        ctx.restore();
    }

    // ------------------------------------------------------------------ bars

    private drawTopBar(game: Game, w: number): void {
        const ui = this.ui;
        const f = game.dayFraction;
        const hours = Math.floor(f * 24);
        const mins = Math.floor((f * 24 - hours) * 60);
        const clock = `${String(hours).padStart(2, '0')}:${String(mins).padStart(2, '0')}`;
        const mon = game.world.monumentAt(game.player.x, game.player.y);
        const biome = game.world.biomeAt(game.player.x, game.player.y);
        const place = mon ? mon.def.name : biome.charAt(0).toUpperCase() + biome.slice(1);
        const hot = mon ? game.world.radiationAt(game.player.x, game.player.y) > 0 : false;

        // Frame counter sits above everything, deliberately quiet.
        const fps = Math.round(game.fps);
        ui.textOnDark(`${Math.min(fps, 120)} / 120 fps`, 16, 12, {
            size: 'micro',
            color: fps >= 50 ? UI.onDarkFaint : fps >= 30 ? UI.hot : UI.warn,
        });

        ui.textOnDark(`Day ${game.day}`, 16, 28, { size: 'heading', weight: 600 });
        ui.textOnDark(clock, 16, 52, { size: 'label', color: UI.onDarkSubtle });
        ui.textOnDark(game.isNight ? 'night' : 'day', 70, 52, {
            size: 'label',
            color: game.isNight ? UI.accentInk : UI.onDarkSubtle,
        });
        ui.textOnDark(place, 130, 30, {
            size: 'label',
            weight: 600,
            color: hot ? WORLD.hazard : UI.onDark,
        });
        if (hot) ui.textOnDark('irradiated', 130, 50, { size: 'caption', color: '#e8836a' });

        // Centre line: only when something is actually happening.
        let banner = '';
        let bannerColor: string = UI.onDarkSubtle;
        if (game.clanSystem.raid) {
            const raider = game.clanSystem.clans.find(
                (c) => c.index === game.clanSystem.raid!.clan,
            );
            banner = `${raider?.name ?? 'A clan'} is raiding your base`;
            bannerColor = UI.warn;
        } else if (game.craftSystem.craftJob) {
            const r = RECIPES[game.craftSystem.craftJob.recipeIndex];
            const waiting = game.craftSystem.craftQueue.length - 1;
            banner =
                `Crafting ${ITEMS[r.out].name}, ${game.craftSystem.craftJob.remaining.toFixed(1)}s` +
                (waiting > 0 ? `  ·  ${waiting} more queued` : '');
        }
        if (banner) {
            ui.textOnDark(banner, w / 2, 22, {
                size: 'body',
                weight: 600,
                color: bannerColor,
                align: 'center',
            });
        }

        const bench = game.craftSystem.benchLevel();
        const hint = `${bench > 0 ? `bench ${bench}   ` : ''}tab · c · m · h`;
        ui.textOnDark(hint, w - 16, 22, { size: 'label', color: UI.onDarkFaint, align: 'right' });
    }

    private drawVitals(game: Game, h: number): void {
        const ui = this.ui;
        const p = game.player;
        const x = 20;
        const mw = 190;
        const baseY = h - 118;

        // An icon heads each gauge where its name used to be: the bar starts
        // after it, and the number still sits at the far end.
        const icon = 18;
        const barX = x + icon + 8;
        const meter = (
            vital: Vital,
            value: number,
            max: number,
            color: string,
            row: number,
        ): void => {
            const my = baseY + row * 26;
            drawVitalIcon(this.ctx, vital, x + icon / 2, my + 15, icon, color);
            ui.textOnDark(String(Math.round(value)), x + mw, my, {
                size: 'caption',
                weight: 600,
                align: 'right',
            });
            ui.meterOnDark(barX, my + 14, x + mw - barX, 5, value / max, color);
        };
        // While you bleed the health bar throbs between its own red and a dark
        // one, so the gauge itself says it is draining.
        const pulse = 0.5 + 0.5 * Math.sin(performance.now() / 140);
        const bleeding = p.bleeding > 0;
        const healthColor = bleeding
            ? pulse > 0.5
                ? WORLD.blood
                : WORLD.hostile
            : p.health > 35
              ? WORLD.hostile
              : '#ff6a5a';
        meter('health', p.health, PLAYER.maxHealth, healthColor, 0);
        if (bleeding) {
            // A blood drop beside the gauge, beating, with the seconds left.
            const bx = x + mw + 18;
            const by = baseY + 15;
            drawVitalIcon(this.ctx, 'water', bx, by, 16 + pulse * 3, WORLD.blood);
            ui.textOnDark(`${Math.ceil(p.bleeding)}s`, bx + 14, baseY + 9, {
                size: 'caption',
                weight: 600,
                color: UI.warn,
            });
        }
        meter('food', p.calories, PLAYER.maxCalories, '#ff8a1c', 1);
        meter('water', p.hydration, PLAYER.maxHydration, '#4a9ee8', 2);

        // Conditions read as a single quiet line, and only when they apply.
        const bits: [string, string][] = [
            [
                `${p.temperature.toFixed(0)}°`,
                p.temperature < PLAYER.comfortTemp - 12 ? UI.accentInk : UI.onDarkSubtle,
            ],
        ];
        if (game.swimming) bits.push(['swimming', UI.accentInk]);
        if (p.radiation > 1)
            bits.push([
                `rad ${p.radiation.toFixed(0)}`,
                p.radiation > 45 ? WORLD.hazard : UI.onDarkSubtle,
            ]);
        if (game.sprinting) bits.push(['running, hands busy', UI.onDarkFaint]);
        let cx = x;
        for (const [text, color] of bits) {
            ui.textOnDark(text, cx, baseY + 84, { size: 'caption', color });
            this.ctx.font = `11px ${TYPE.body}`;
            cx += this.ctx.measureText(text).width + 14;
        }
    }

    private drawHotbar(game: Game, w: number, h: number): void {
        const ui = this.ui;
        const slotW = 56;
        const gap = 6;
        const count = game.player.hotbar.slots.length;
        const total = count * slotW + (count - 1) * gap;
        const x0 = Math.round(w / 2 - total / 2);
        const y = h - slotW - 34;

        for (let i = 0; i < count; i++) {
            const stack = game.player.hotbar.slots[i];
            const x = x0 + i * (slotW + gap);
            const active = game.player.activeSlot === i;
            const rect: Rect = { x, y, w: slotW, h: slotW };
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            ui.slotOnDark(rect.x, rect.y, rect.w, rect.h, { hovered, selected: active });

            ui.textOnDark(String(i + 1), x + 5, y + 4, {
                size: 'micro',
                color: active ? UI.accentInk : UI.onDarkFaint,
            });
            if (stack) {
                drawItemIcon(this.ctx, stack.id, x + slotW / 2, y + slotW / 2, slotW - 20);
                if (stack.count > 1) {
                    ui.textOnDark(String(stack.count), x + slotW - 5, y + slotW - 15, {
                        size: 'micro',
                        weight: 600,
                        align: 'right',
                    });
                }
                const def = ITEMS[stack.id];
                if (def.gun) {
                    // Magazine weapons read "in the gun / in the pack"; a bow just has
                    // however many arrows you are carrying.
                    const spare = countAcross(game.containers, def.gun.ammo);
                    const label = def.gun.magazine
                        ? `${stack.loaded ?? 0}/${spare}`
                        : String(spare);
                    const dry = def.gun.magazine ? (stack.loaded ?? 0) === 0 : spare === 0;
                    ui.textOnDark(label, x + 5, y + slotW - 15, {
                        size: 'micro',
                        color: dry ? UI.warn : UI.onDarkSubtle,
                    });
                }
            }
            if (hovered && game.input.mouseClicked) game.player.activeSlot = i;
            if (hovered) game.uiHover = true;
        }

        const held = game.heldItem();
        if (held?.id === 'building_plan') {
            const kinds: BuildKind[] = ['foundation', 'wall', 'doorway', 'door'];
            const label = kinds.map((k) => (k === game.buildKind ? `[${k}]` : k)).join('   ');
            ui.textOnDark(label, w / 2, y - 24, {
                size: 'caption',
                color: UI.accentInk,
                align: 'center',
            });
            ui.textOnDark('q or wheel to change piece  ·  10 wood', w / 2, y + slotW + 10, {
                size: 'caption',
                color: UI.onDarkFaint,
                align: 'center',
            });
        }
        // No general hint strip under the belt. It took up screen the whole run
        // to say something you need once, and the full list now lives in
        // Settings. The building-plan line above stays: that one is contextual.
    }

    /**
     * Minimap, the recon toggle and the clan roster. Frameless like the rest of
     * the in-game overlay: a soft wash behind the map for legibility, nothing more.
     */
    private drawMapPanel(game: Game, w: number): void {
        const ui = this.ui;
        const live = game.clanSystem.clans.filter((c) => !c.wiped);
        const size = 168;
        const x = w - size - 20;
        const y = 62;

        ui.glass(x - 4, y - 4, size + 8, size + 8, RADIUS.sm);
        this.renderer.drawMinimap(game, x, y, size);

        // Recon toggle.
        const btn: Rect = { x, y: y + size + 8, w: size, h: 24 };
        const hovered = hit(btn, game.input.mouseX, game.input.mouseY);
        ui.roundRect(btn.x, btn.y, btn.w, btn.h, RADIUS.sm);
        this.ctx.fillStyle = game.revealClans
            ? 'rgba(10,110,235,0.72)'
            : hovered
              ? 'rgba(255,255,255,0.14)'
              : UI.glass;
        this.ctx.fill();
        ui.textOnDark(
            game.revealClans ? 'Recon on' : 'Reveal clans',
            btn.x + btn.w / 2,
            btn.y + 7,
            {
                size: 'caption',
                weight: 600,
                align: 'center',
                color: game.revealClans ? '#ffffff' : UI.onDarkSubtle,
            },
        );
        if (hovered) {
            game.uiHover = true;
            if (game.input.mouseClicked) game.toggleReveal();
        }

        // Who is on the island is intelligence, not a given: it only shows while
        // recon is running.
        if (!game.revealClans) return;

        let ry = btn.y + 38;
        ui.textOnDark(`CLANS ${live.length}`, x, ry, { size: 'micro', color: UI.onDarkFaint });
        ry += 14;
        for (const c of live) {
            this.ctx.save();
            this.ctx.shadowColor = WORLD.nameTagShadow;
            this.ctx.shadowBlur = 3;
            this.ctx.fillStyle = c.color;
            this.ctx.fillRect(x, ry + 3, 6, 6);
            this.ctx.restore();
            const members = game.npcs.list.filter((n) => n.clan === c.index + 1).length;
            ui.fit(`${c.name}, ${CLAN_SKILLS[c.skill].label}`, x + 12, ry, size - 40, {
                size: 'caption',
                color: UI.onDarkSubtle,
            });
            ui.textOnDark(`${members}`, x + size, ry, {
                size: 'caption',
                weight: 600,
                align: 'right',
                color: members > 4 ? '#ff9a72' : UI.onDarkSubtle,
            });
            ry += 15;
        }
    }

    // No progress bar here. Every timed action draws one arc beside the player
    // instead, so you watch the job rather than a strip above the belt. See
    // PROGRESS_ARC and Game.progressJob.

    /** Contextual prompt for whatever is under the player's nose. */
    private drawContextHints(game: Game, w: number, h: number): void {
        const p = game.player;
        if (!p.alive) return;
        let hint = '';

        const prev = game.held.buildPreview();
        if (prev && !prev.valid && prev.reason) hint = prev.reason;
        if (!hint) {
            // Whatever E would do, from the same place E asks. Never scan for
            // interactables here: see `InteractionSystem.target`.
            const t = game.interaction.target();
            if (t) hint = game.interaction.prompt(t);
        }
        if (!hint) return;

        this.ctx.font = `12px ${TYPE.body}`;
        const tw = this.ctx.measureText(hint).width + 26;
        const y = h - 148;
        this.ui.glass(w / 2 - tw / 2, y, tw, 26, RADIUS.sm);
        this.ui.textOnDark(hint, w / 2, y + 7, { size: 'label', align: 'center' });
    }

    /**
     * A grid of slots in the light style, with press-and-drag movement.
     * See docs/systems/inventory.md for where items are allowed to live.
     */
    private slotGrid(
        game: Game,
        container: Container,
        x: number,
        y: number,
        cols: number,
        label: string,
        opts: {
            beltOnly?: boolean;
            across?: Container | null;
            /** Refuses anything without a `wear` block: this is the worn slot. */
            wearableOnly?: boolean;
            /** Refuses anything you cannot hold: this is the belt. */
            heldOnly?: boolean;
        } = {},
    ): void {
        const ctx = this.ctx;
        const ui = this.ui;
        const s = 46;
        if (label) {
            ctx.font = `11px ${TYPE.body}`;
            ctx.fillStyle = UI.subtle;
            ctx.fillText(label, x + 2, y - 17);
        }

        for (let i = 0; i < container.slots.length; i++) {
            const cx = x + (i % cols) * s;
            const cy = y + Math.floor(i / cols) * s;
            const rect: Rect = { x: cx, y: cy, w: s - 5, h: s - 5 };
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            const dragging = game.dragging;
            const isSource = dragging && dragging.from === container && dragging.index === i;

            ui.slotOnDark(rect.x, rect.y, rect.w, rect.h, {
                hovered,
                selected: !!(hovered && dragging),
            });

            const stack = container.slots[i];
            if (stack && !isSource) {
                this.drawStackIcon(stack, rect);
            }

            // Press to lift, release to place.
            if (hovered && game.input.mouseClicked && stack && !dragging) {
                game.dragging = { from: container, index: i };
                game.inspecting = { container, index: i };
            }
            if (hovered && game.input.mouseReleased && dragging && !isSource) {
                const incoming = dragging.from.slots[dragging.index];
                // You cannot wear a rock. Refusing here rather than in
                // moveStack keeps the move itself dumb and general.
                if (opts.wearableOnly && incoming && !ITEMS[incoming.id].wear) {
                    game.message = `${ITEMS[incoming.id].name} is not something you can wear.`;
                } else if (opts.heldOnly && incoming && !isBeltItem(incoming.id)) {
                    // The belt is for what you hold. Raw materials live in the
                    // pack; a slot of stone on your belt is a slot you cannot
                    // draw anything from.
                    game.message = 'Only things you hold go on the belt.';
                } else {
                    moveStack(dragging.from, dragging.index, container, i);
                }
                game.dragging = null;
            }
            // Right-click sends the whole stack to the other side, over time.
            // Nothing to aim at and nothing to hold down.
            if (hovered && game.input.rightClicked && stack && opts.across && !dragging) {
                game.beginTransfer(container, i, opts.across);
            }
            if (hovered && stack && !dragging)
                this.tooltip(stack.id, game.input.mouseX, game.input.mouseY);

            const t = game.transferAt(container, i);
            if (t) {
                this.transferSpinner(
                    rect.x + rect.w / 2,
                    rect.y + rect.h / 2,
                    t.elapsed / t.duration,
                );
            }
        }
    }

    /**
     * The spinner over a slot being carried across.
     *
     * A ring that fills rather than a bar, because it has to sit on top of the
     * item icon without hiding what is being moved.
     */
    private transferSpinner(cx: number, cy: number, progress: number): void {
        const ctx = this.ctx;
        const r = 14;
        ctx.save();
        ctx.fillStyle = 'rgba(12, 14, 16, 0.6)';
        ctx.beginPath();
        ctx.arc(cx, cy, r + 3, 0, Math.PI * 2);
        ctx.fill();
        ctx.strokeStyle = UI.subtle;
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.stroke();
        ctx.strokeStyle = UI.accentInk;
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * Math.min(1, progress));
        ctx.stroke();
        ctx.restore();
    }

    private drawStackIcon(stack: ItemStack, rect: Rect): void {
        drawItemIcon(this.ctx, stack.id, rect.x + rect.w / 2, rect.y + rect.h / 2 - 4, rect.h - 14);
        if (stack.count > 1) {
            this.ui.text(String(stack.count), rect.x + rect.w - 4, rect.y + rect.h - 13, {
                size: 'micro',
                weight: 600,
                color: UI.ink,
                align: 'right',
            });
        }
    }

    /** The stack currently in hand, drawn under the cursor. */
    private drawDragged(game: Game): void {
        const d = game.dragging;
        if (!d) return;
        const stack = d.from.slots[d.index];
        if (!stack) {
            game.dragging = null;
            return;
        }
        const ctx = this.ctx;
        const rect: Rect = { x: game.input.mouseX - 20, y: game.input.mouseY - 20, w: 41, h: 41 };
        ctx.save();
        ctx.globalAlpha = 0.92;
        ctx.shadowColor = WORLD.shadowDeep;
        ctx.shadowBlur = 12;
        ctx.shadowOffsetY = 4;
        this.roundRect(rect.x, rect.y, rect.w, rect.h, 9);
        ctx.fillStyle = UI.surface;
        ctx.fill();
        ctx.restore();
        this.drawStackIcon(stack, rect);

        // Released outside the panel entirely: that is a deliberate throw-away.
        if (game.input.mouseReleased) {
            if (this.dropZone && !hit(this.dropZone, game.input.mouseX, game.input.mouseY)) {
                game.interaction.dropSlot(d.from, d.index);
            }
            game.dragging = null;
        }
    }

    /** The panel currently accepting drops; anything released outside it is dropped. */
    private dropZone: Rect | null = null;

    /**
     * The tab strip that sits on top of the inventory, crafting and crate
     * panels. They are one screen with three pages, so you switch page by
     * clicking rather than by closing one panel to open the next.
     */
    /**
     * Creative-mode item shelf. Every item in the game, grouped by category,
     * with the amount you get per click on a dial at the top.
     */
    private drawSandbox(game: Game, w: number, h: number): void {
        const ui = this.ui;
        const panelW = Math.min(900, w - 80);
        const panelH = Math.min(600, h - 80);
        const x = Math.round(w / 2 - panelW / 2);
        const y = Math.round(h / 2 - panelH / 2);

        ui.scrim(w, h);
        ui.panel(x, y, panelW, panelH);
        this.drawPanelTabs(game, x, y);
        const top = ui.header(
            x,
            y,
            panelW,
            'Sandbox',
            'click an item to take it  ·  [ and ] move the clock an hour',
            'f or tab to close',
        );

        // Amount dial.
        let ax = x + SPACE.lg;
        ui.text('per click', ax, top + 14, { size: 'label', color: UI.subtle });
        ax += 76;
        for (const n of SANDBOX_AMOUNTS) {
            const rect: Rect = { x: ax, y: top + 8, w: 52, h: 26 };
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            // Only the chosen amount is lit; the rest read as available.
            ui.button(rect.x, rect.y, rect.w, rect.h, String(n), {
                enabled: this.sandboxAmount === n,
                hovered,
            });
            if (hovered && game.input.mouseClicked) this.sandboxAmount = n;
            ax += 58;
        }

        // Item grid, in category order so like sits with like.
        const ids = (Object.keys(ITEMS) as ItemId[]).slice().sort((a, b) => {
            const ca = SANDBOX_ORDER.indexOf(ITEMS[a].category);
            const cb = SANDBOX_ORDER.indexOf(ITEMS[b].category);
            return ca === cb ? ITEMS[a].name.localeCompare(ITEMS[b].name) : ca - cb;
        });
        const cell = 52;
        const gap = 6;
        const cols = Math.floor((panelW - SPACE.lg * 2 + gap) / (cell + gap));
        const gy0 = top + 50;
        ids.forEach((id, i) => {
            const cx = x + SPACE.lg + (i % cols) * (cell + gap);
            const cy = gy0 + Math.floor(i / cols) * (cell + gap);
            if (cy + cell > y + panelH - SPACE.lg) return;
            const rect: Rect = { x: cx, y: cy, w: cell, h: cell };
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            ui.slot(rect.x, rect.y, rect.w, rect.h, { hovered });
            drawItemIcon(this.ctx, id, cx + cell / 2, cy + cell / 2, cell - 18);
            if (hovered) {
                game.uiHover = true;
                ui.text(ITEMS[id].name, x + SPACE.lg, y + panelH - 28, {
                    size: 'label',
                    weight: 600,
                });
                if (game.input.mouseClicked) game.giveItem(id, this.sandboxAmount);
            }
        });
    }

    private drawPanelTabs(game: Game, x: number, y: number): void {
        const ctx = this.ctx;
        const tabs: { label: string; panel: 'inventory' | 'craft' | 'container' | 'sandbox' }[] = [
            { label: 'Inventory', panel: 'inventory' },
            { label: 'Craft', panel: 'craft' },
        ];
        if (game.sandbox) tabs.push({ label: 'Sandbox', panel: 'sandbox' });
        if (game.openContainer) tabs.push({ label: game.openContainer.title, panel: 'container' });

        const th = 30;
        const ty = y - th - 6;
        let tx = x;
        for (const t of tabs) {
            ctx.font = `600 13px ${TYPE.body}`;
            const tw = Math.round(ctx.measureText(t.label).width) + 30;
            const rect: Rect = { x: tx, y: ty, w: tw, h: th };
            const active = game.panel === t.panel;
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            this.roundRect(rect.x, rect.y, rect.w, rect.h, 8);
            ctx.fillStyle = active
                ? UI.surface
                : hovered
                  ? 'rgba(20,20,20,0.72)'
                  : 'rgba(20,20,20,0.55)';
            ctx.fill();
            ctx.strokeStyle = UI.hairline;
            ctx.lineWidth = 1;
            ctx.stroke();
            ctx.fillStyle = active ? UI.ink : 'rgba(242,241,236,0.72)';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(t.label, rect.x + rect.w / 2, rect.y + rect.h / 2);
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            if (hovered) {
                game.uiHover = true;
                if (game.input.mouseClicked && !active) {
                    game.panel = t.panel;
                    game.inspecting = null;
                }
            }
            tx += tw + 6;
        }
    }

    /**
     * The whole island, opened with M.
     *
     * The corner map is for a glance; this is for planning where to go. The
     * recon toggle lives here too, now that M is the map's key.
     */
    private drawIslandMap(game: Game, w: number, h: number): void {
        const ui = this.ui;
        const size = Math.max(200, Math.min(w, h) - 190);
        const panelW = size + SPACE.lg * 2;
        const panelH = size + 130;
        const x = Math.round(w / 2 - panelW / 2);
        const y = Math.round(h / 2 - panelH / 2);
        ui.scrim(w, h);
        ui.panel(x, y, panelW, panelH);
        const top = ui.header(x, y, panelW, 'Map', 'the whole island', 'm to close');
        this.renderer.drawMinimap(game, x + SPACE.lg, top + 12, size, 2);

        const btn: Rect = { x: x + SPACE.lg, y: top + 12 + size + 12, w: 200, h: 30 };
        const hovered = hit(btn, game.input.mouseX, game.input.mouseY);
        ui.button(btn.x, btn.y, btn.w, btn.h, game.revealClans ? 'Recon on' : 'Reveal clans', {
            hovered,
        });
        if (hovered && game.input.mouseClicked) game.toggleReveal();
    }

    private drawInventory(game: Game, w: number, h: number): void {
        const ui = this.ui;
        const panelW = Math.min(760, w - 80);
        const panelH = Math.min(500, h - 80);
        const x = Math.round(w / 2 - panelW / 2);
        const y = Math.round(h / 2 - panelH / 2);
        this.dropZone = { x, y, w: panelW, h: panelH };

        ui.scrim(w, h);
        ui.panel(x, y, panelW, panelH);
        this.drawPanelTabs(game, x, y);
        const top = ui.header(
            x,
            y,
            panelW,
            'Inventory',
            'drag to move  ·  drag out to drop  ·  click to inspect',
            'tab to close',
        );

        // An open crate is where a right-click sends things from, whichever tab
        // you are looking at. Without this, opening a box and then switching to
        // the inventory tab left right-click either doing nothing or filling
        // your own pack, which is the opposite of what you asked for.
        const away = game.openContainer?.container ?? null;
        this.slotGrid(game, game.player.hotbar, x + SPACE.lg, top + 36, 6, 'BELT, WHAT YOU HOLD', {
            heldOnly: true,
            across: away ?? game.player.inventory,
        });
        const mainTop = top + 128;
        this.slotGrid(game, game.player.inventory, x + SPACE.lg, mainTop, 6, 'MAIN', {
            across: away ?? undefined,
        });
        // What you have on, in a box of its own under the pack. It used to be
        // laid at a fixed height that landed on the pack's fourth row, so the
        // worn slot read as one more pack slot.
        const rows = Math.ceil(game.player.inventory.slots.length / 6);
        const wornTop = mainTop + rows * 46 + 34;
        this.ui.card(x + SPACE.lg - 10, wornTop - 28, 46 + 20, 46 + 32, 1);
        this.slotGrid(game, game.player.worn, x + SPACE.lg, wornTop, 1, 'WORN', {
            wearableOnly: true,
        });

        this.drawItemDetail(
            game,
            x + SPACE.lg + 6 * 46 + 24,
            top + 18,
            panelW - 6 * 46 - SPACE.lg * 2 - 24,
            panelH - 100,
        );
    }

    /** Right-hand column: what is selected, what it is, and what you can do with it. */
    private drawItemDetail(game: Game, x: number, y: number, w: number, h: number): void {
        const ui = this.ui;
        const sel = game.inspecting;
        const stack = sel ? sel.container.slots[sel.index] : null;
        if (!sel || !stack) {
            ui.text('Select an item', x, y + 10, { size: 'label', color: UI.subtle });
            return;
        }
        const def = ITEMS[stack.id];

        drawItemIcon(this.ctx, stack.id, x + 22, y + 22, 44);
        ui.fit(def.name, x + 56, y + 6, w - 56, { size: 'heading', weight: 600 });
        // "1 of 1 per slot" is not worth a line: a thing that only ever comes
        // as one has nothing to say about how many fit.
        if (def.stack > 1) {
            ui.text(`${stack.count} of ${def.stack} per slot`, x + 56, y + 28, {
                size: 'caption',
                color: UI.subtle,
            });
        }

        let ly = ui.wrap(def.desc, x, y + 62, w, 17, { size: 'label', color: UI.subtle });

        // Stats worth knowing, drawn only when the item has them.
        const rows: [string, string][] = [];
        rows.push(['Type', def.category]);
        if (def.melee) rows.push(['Damage', `${def.melee.damage}`]);
        if (def.gun) {
            rows.push(['Damage', `${def.gun.damage}`], ['Ammo', ITEMS[def.gun.ammo].name]);
            if (def.gun.magazine) {
                rows.push(
                    ['Magazine', `${def.gun.magazine}`],
                    ['Reload', `${def.gun.reloadSeconds ?? 2.5}s`],
                );
            }
        }
        if (def.food?.health) rows.push(['Heals', `${def.food.health}`]);
        if (def.useSeconds) rows.push(['Takes', `${def.useSeconds}s`]);
        if (def.wear?.warmth) rows.push(['Warmth', `+${def.wear.warmth}`]);
        if (def.wear?.radiation)
            rows.push(['Rad block', `${Math.round(def.wear.radiation * 100)}%`]);
        if (def.boom) rows.push(['Blast', `${def.boom.damage}`]);
        ly += 8;
        for (const [k, v] of rows) {
            ui.text(k, x, ly, { size: 'caption', color: UI.subtle });
            ui.text(v, x + w, ly, { size: 'caption', weight: 600, align: 'right' });
            ly += 18;
        }

        // Actions.
        const actions: { label: string; run: () => void }[] = [];
        if (def.food)
            actions.push({
                label: def.category === 'consumable' ? 'Use' : 'Eat',
                run: () => game.survival.consume(stack.id),
            });
        // "Wear" only makes sense from the pack; from the worn slot the same
        // button would put on what you already have on.
        if (def.wear && sel.container !== game.player.worn) {
            actions.push({ label: 'Wear', run: () => game.survival.wearItem(stack.id) });
        }
        if (sel.container === game.player.worn) {
            actions.push({
                label: 'Take off',
                run: () => game.survival.takeOff(),
            });
        }
        // Not from the worn slot: "Take off" already does that, and two
        // buttons doing the same thing is worse than one.
        actions.push({
            label: 'Drop',
            run: () => {
                game.interaction.dropSlot(sel.container, sel.index);
                game.inspecting = null;
            },
        });

        let by = y + h - 44 - (actions.length - 1) * 42;
        for (const a of actions) {
            const rect: Rect = { x, y: by, w, h: 34 };
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            const danger = a.label === 'Drop';
            ui.roundRect(rect.x, rect.y, rect.w, rect.h, RADIUS.md);
            this.ctx.fillStyle = danger
                ? hovered
                    ? 'rgba(194,69,47,0.16)'
                    : UI.surfaceAlt
                : hovered
                  ? UI.accentHover
                  : UI.accent;
            this.ctx.fill();
            ui.text(a.label, rect.x + rect.w / 2, rect.y + 9, {
                size: 'body',
                weight: 600,
                align: 'center',
                color: danger ? UI.warn : '#ffffff',
            });
            if (hovered && game.input.mouseClicked) a.run();
            by += 42;
        }
    }

    private drawContainerPanel(game: Game, w: number, h: number): void {
        const oc = game.openContainer;
        if (!oc) return;
        const ctx = this.ctx;
        const panelW = Math.min(700, w - 80);
        const panelH = Math.min(560, h - 60);
        const x = Math.round(w / 2 - panelW / 2);
        const y = Math.round(h / 2 - panelH / 2);

        this.dim(w, h, 0.4);
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.45)';
        ctx.shadowBlur = 40;
        ctx.shadowOffsetY = 12;
        this.roundRect(x, y, panelW, panelH, 16);
        ctx.fillStyle = UI.surface;
        ctx.fill();
        ctx.restore();
        this.roundRect(x, y, panelW, panelH, 16);
        ctx.strokeStyle = UI.hairline;
        ctx.lineWidth = 1;
        ctx.stroke();

        this.drawPanelTabs(game, x, y);
        ctx.font = `600 20px ${TYPE.display}`;
        ctx.fillStyle = UI.ink;
        ctx.fillText(oc.title, x + 26, y + 24);

        const src = oc.source as { lit?: boolean; fuel?: number } | null;
        ctx.font = `12px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        if (src && 'lit' in src) {
            ctx.fillStyle = src.lit ? '#c2732f' : UI.subtle;
            ctx.fillText(
                src.lit
                    ? `Burning, ${Math.max(0, src.fuel ?? 0).toFixed(0)}s of fuel`
                    : 'Not lit, press E to light',
                x + 26,
                y + 50,
            );
        } else {
            ctx.fillText('Drag to move  ·  right-click to send across', x + 26, y + 50);
        }
        ctx.fillStyle = UI.subtle;
        ctx.textAlign = 'right';
        ctx.fillText('e or esc to close', x + panelW - 26, y + 50);
        ctx.textAlign = 'left';

        ctx.strokeStyle = UI.hairline;
        ctx.beginPath();
        ctx.moveTo(x + 1, y + 72);
        ctx.lineTo(x + panelW - 1, y + 72);
        ctx.stroke();

        // Right-click moves a stack to the opposite side. Out of the box it
        // lands in your main pack; out of your pack or belt it lands in the box.
        this.slotGrid(game, oc.container, x + 26, y + 108, 6, 'CONTENTS', {
            across: game.player.inventory,
        });
        this.slotGrid(game, game.player.hotbar, x + 26, y + 250, 6, 'BELT', {
            across: oc.container,
            heldOnly: true,
        });
        this.slotGrid(game, game.player.inventory, x + 26, y + 342, 6, 'MAIN', {
            across: oc.container,
        });

        // Emptying a crate starts its respawn timer.
        const crate = oc.source as { looted?: boolean; respawn?: number } | null;
        if (
            crate &&
            'looted' in crate &&
            oc.container.slots.every((sl) => sl === null) &&
            !crate.looted
        ) {
            crate.looted = true;
            crate.respawn = 300;
        }
    }

    // ---------------------------------------------------------------- craft

    private drawCraft(game: Game, w: number, h: number): void {
        const ctx = this.ctx;
        const ui = this.ui;
        const panelW = Math.min(1080, w - 80);
        const panelH = Math.min(560, h - 80);
        const x = Math.round(w / 2 - panelW / 2);
        const y = Math.round(h / 2 - panelH / 2);
        const bench = game.craftSystem.benchLevel();
        const queueW = 218;

        this.dim(w, h, 0.4);

        // Light, quiet panel: soft shadow, hairline border, generous whitespace.
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.45)';
        ctx.shadowBlur = 40;
        ctx.shadowOffsetY = 12;
        this.roundRect(x, y, panelW, panelH, 16);
        ctx.fillStyle = UI.surface;
        ctx.fill();
        ctx.restore();
        this.roundRect(x, y, panelW, panelH, 16);
        ctx.strokeStyle = UI.hairline;
        ctx.lineWidth = 1;
        ctx.stroke();

        this.drawPanelTabs(game, x, y);

        // Header
        ctx.font = `600 20px ${TYPE.display}`;
        ctx.fillStyle = UI.ink;
        ctx.fillText('Crafting', x + 26, y + 24);
        ctx.font = `12px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        ctx.fillText(
            bench > 0
                ? `Workbench level ${bench} nearby`
                : 'No workbench nearby, level 0 recipes only',
            x + 26,
            y + 50,
        );
        ctx.textAlign = 'right';
        ctx.fillText('esc to close', x + panelW - 26, y + 50);
        ctx.textAlign = 'left';

        ctx.strokeStyle = UI.hairline;
        ctx.beginPath();
        ctx.moveTo(x + 1, y + 72);
        ctx.lineTo(x + panelW - 1, y + 72);
        ctx.stroke();

        // ---- column 1: categories
        const colX = x + 20;
        const colW = 190;
        const listY = y + 88;
        const cats = CRAFT_CATEGORIES;
        ctx.font = `11px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        ctx.fillText('CATEGORY', colX + 12, listY - 16);

        for (let i = 0; i < cats.length; i++) {
            const cat = cats[i];
            const rect: Rect = { x: colX, y: listY + i * 42, w: colW, h: 38 };
            const selected = game.craftCategory === cat.id;
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            if (selected || hovered) {
                this.roundRect(rect.x, rect.y, rect.w, rect.h, 10);
                ctx.fillStyle = selected ? UI.selected : UI.hover;
                ctx.fill();
            }
            ctx.font = `600 13px ${TYPE.body}`;
            ctx.fillStyle = selected ? UI.accentInk : UI.ink;
            ctx.fillText(cat.label, rect.x + 14, rect.y + 8);
            const count = RECIPES.filter((r) => cat.match(r.out)).length;
            ctx.font = `11px ${TYPE.body}`;
            ctx.fillStyle = UI.subtle;
            ctx.textAlign = 'right';
            ctx.fillText(String(count), rect.x + rect.w - 14, rect.y + 12);
            ctx.textAlign = 'left';
            if (hovered && game.input.mouseClicked) {
                game.craftCategory = cat.id;
                game.craftSelection = null;
            }
        }

        // ---- column 2: recipes in that category
        const midX = colX + colW + 16;
        const midW = 280;
        ctx.strokeStyle = UI.hairline;
        ctx.beginPath();
        ctx.moveTo(midX - 8, y + 80);
        ctx.lineTo(midX - 8, y + panelH - 20);
        ctx.stroke();

        const cat = cats.find((c) => c.id === game.craftCategory) ?? cats[0];
        const list = RECIPES.map((r, i) => ({ r, i })).filter(({ r }) => cat.match(r.out));
        ctx.font = `11px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        ctx.fillText(cat.label.toUpperCase(), midX + 12, listY - 16);

        for (let n = 0; n < list.length; n++) {
            const { r, i } = list[n];
            const rect: Rect = { x: midX, y: listY + n * 40, w: midW, h: 36 };
            if (rect.y + rect.h > y + panelH - 20) break;
            const locked = r.bench > bench;
            const affordable = canCraft(r, game.containers, bench);
            const selected = game.craftSelection === i;
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);

            if (selected || hovered) {
                this.roundRect(rect.x, rect.y, rect.w, rect.h, 10);
                ctx.fillStyle = selected ? UI.selected : UI.hover;
                ctx.fill();
            }
            ctx.save();
            if (locked) ctx.globalAlpha = 0.35;
            drawItemIcon(ctx, r.out, rect.x + 21, rect.y + rect.h / 2, 26);
            ctx.restore();

            ctx.font = `600 13px ${TYPE.body}`;
            ctx.fillStyle = locked ? UI.disabled : selected ? UI.accentInk : UI.ink;
            this.fit(
                `${ITEMS[r.out].name}${r.amount > 1 ? ` ×${r.amount}` : ''}`,
                rect.x + 42,
                rect.y + 5,
                midW - 60,
            );
            ctx.font = `11px ${TYPE.body}`;
            ctx.fillStyle = locked ? UI.disabled : affordable ? UI.ok : UI.warn;
            ctx.fillText(
                locked ? `Workbench ${r.bench}` : affordable ? 'Ready' : 'Missing materials',
                rect.x + 42,
                rect.y + 21,
            );

            if (hovered && game.input.mouseClicked) {
                game.craftSelection = i;
                game.craftAmount = 1;
            }
        }

        // ---- column 3: the selected recipe
        const detX = midX + midW + 24;
        const detW = x + panelW - detX - 24 - queueW - 24;
        ctx.strokeStyle = UI.hairline;
        ctx.beginPath();
        ctx.moveTo(detX - 12, y + 80);
        ctx.lineTo(detX - 12, y + panelH - 20);
        ctx.stroke();

        const sel = game.craftSelection !== null ? RECIPES[game.craftSelection] : null;
        if (!sel) {
            ctx.font = `12px ${TYPE.body}`;
            ctx.fillStyle = UI.subtle;
            ctx.fillText('Select an item', detX, listY);
            return;
        }

        drawItemIcon(ctx, sel.out, detX + 22, listY + 18, 44);
        ctx.font = `600 17px ${TYPE.display}`;
        ctx.fillStyle = UI.ink;
        this.fit(ITEMS[sel.out].name, detX + 56, listY + 2, detW - 56);
        ctx.font = `11px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        ctx.fillText(`${sel.seconds}s to craft`, detX + 56, listY + 22);

        ctx.font = `12px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        this.wrap(ITEMS[sel.out].desc, detX, listY + 60, detW, 17);

        ctx.font = `11px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        ctx.fillText('MATERIALS', detX, listY + 118);
        const costs = Object.entries(sel.cost) as [ItemId, number][];
        for (let i = 0; i < costs.length; i++) {
            const [id, need] = costs[i];
            const have = countAcross(game.containers, id);
            const ry = listY + 140 + i * 26;
            drawItemIcon(ctx, id, detX + 8, ry + 8, 18);
            ctx.font = `12px ${TYPE.body}`;
            ctx.fillStyle = UI.ink;
            ctx.fillText(ITEMS[id].name, detX + 26, ry + 2);
            ctx.textAlign = 'right';
            ctx.fillStyle = have >= need ? UI.ok : UI.warn;
            ctx.fillText(`${Math.min(have, need)} / ${need}`, detX + detW, ry + 2);
            ctx.textAlign = 'left';
        }

        // Quantity: steppers, a max button, and a field you can type into.
        const possible =
            game.craftSelection !== null ? game.craftSystem.craftableCount(game.craftSelection) : 0;
        const qy = y + panelH - 124;
        ui.text('AMOUNT', detX, qy - 18, { size: 'caption', color: UI.subtle });

        if (this.amountFocus) {
            // Typing owns the number while the field has focus, so clamping it back
            // into range on every frame would fight the person typing "12".
            this.amountText += game.input.typed.replace(/[^0-9]/g, '');
            for (const e of game.input.edits) {
                if (e === 'back') this.amountText = this.amountText.slice(0, -1);
                else this.amountFocus = false;
            }
            if (this.amountText.length > 4) this.amountText = this.amountText.slice(0, 4);
            const typedValue = parseInt(this.amountText, 10);
            if (!Number.isNaN(typedValue))
                game.craftAmount = Math.max(1, Math.min(typedValue, Math.max(1, possible)));
            if (!this.amountFocus) this.amountText = '';
        } else {
            game.craftAmount = Math.max(1, Math.min(game.craftAmount, Math.max(1, possible)));
        }

        const step = (label: string, sx: number, run: () => void, enabled: boolean): void => {
            const r: Rect = { x: sx, y: qy, w: 34, h: 34 };
            const hv = hit(r, game.input.mouseX, game.input.mouseY) && enabled;
            ui.roundRect(r.x, r.y, r.w, r.h, RADIUS.sm);
            ctx.fillStyle = hv ? UI.hover : UI.surfaceAlt;
            ctx.fill();
            ui.text(label, r.x + r.w / 2, r.y + 9, {
                size: 'body',
                weight: 600,
                align: 'center',
                color: enabled ? UI.ink : UI.disabled,
            });
            if (hv && game.input.mouseClicked) run();
        };
        step(
            '−',
            detX,
            () => {
                game.craftAmount = Math.max(1, game.craftAmount - 1);
            },
            game.craftAmount > 1,
        );

        const field: Rect = { x: detX + 40, y: qy, w: 58, h: 34 };
        const fieldHover = hit(field, game.input.mouseX, game.input.mouseY);
        ui.roundRect(field.x, field.y, field.w, field.h, RADIUS.sm);
        ctx.fillStyle = this.amountFocus ? UI.selected : fieldHover ? UI.hover : UI.surfaceAlt;
        ctx.fill();
        ctx.strokeStyle = this.amountFocus ? UI.accent : UI.hairline;
        ctx.lineWidth = 1;
        ctx.stroke();
        ui.text(
            this.amountFocus ? `${this.amountText || ''}_` : String(game.craftAmount),
            field.x + field.w / 2,
            field.y + 9,
            { size: 'body', weight: 600, align: 'center' },
        );
        if (fieldHover) {
            game.uiHover = true;
            if (game.input.mouseClicked) {
                this.amountFocus = true;
                this.amountText = '';
            }
        } else if (game.input.mouseClicked) {
            this.amountFocus = false;
        }

        step(
            '+',
            detX + 104,
            () => {
                game.craftAmount = Math.min(possible || 1, game.craftAmount + 1);
            },
            game.craftAmount < possible,
        );
        step(
            'max',
            detX + 144,
            () => {
                game.craftAmount = Math.max(1, possible);
            },
            possible > 1,
        );
        ui.text(possible > 0 ? `${possible} possible` : 'cannot afford any', detX + detW, qy + 11, {
            size: 'caption',
            color: possible > 0 ? UI.subtle : UI.warn,
            align: 'right',
        });

        // Craft button. Rust's modifiers: shift for ten, ctrl for as many as you
        // can pay for. Queuing is allowed while something is already on, so the
        // button only goes dead when you genuinely cannot start the run.
        const queueFull = game.craftSystem.craftQueue.length >= CRAFT_QUEUE_MAX;
        const canMake = possible > 0 && !queueFull;
        const btn: Rect = { x: detX, y: y + panelH - 74, w: detW, h: 42 };
        const btnHover = hit(btn, game.input.mouseX, game.input.mouseY) && canMake;
        ui.button(
            btn.x,
            btn.y,
            btn.w,
            btn.h,
            queueFull
                ? 'Queue full'
                : sel.bench > bench
                  ? `Needs workbench ${sel.bench}`
                  : game.craftAmount > 1
                    ? `Craft ${game.craftAmount}`
                    : 'Craft',
            { enabled: canMake, hovered: btnHover },
        );
        ui.text('shift +10  ·  ctrl all', detX + detW / 2, y + panelH - 26, {
            size: 'caption',
            color: UI.subtle,
            align: 'center',
        });
        if (btnHover && game.input.mouseClicked && game.craftSelection !== null) {
            const amount = game.input.ctrlClick
                ? Math.max(1, possible)
                : game.input.shiftClick
                  ? Math.min(Math.max(1, possible), game.craftAmount + 10)
                  : game.craftAmount;
            game.craftSystem.startCraft(game.craftSelection, amount);
            this.amountFocus = false;
        }

        this.drawCraftQueue(game, x + panelW - queueW - 24, listY, queueW, y + panelH - listY - 24);
    }

    /**
     * The queue column. The head is being made now and cannot be moved, because
     * shuffling it would throw away the seconds already spent on it; everything
     * behind it can be reordered or cancelled, and cancelling refunds.
     */
    private drawCraftQueue(game: Game, x: number, y: number, w: number, h: number): void {
        const ctx = this.ctx;
        ctx.strokeStyle = UI.hairline;
        ctx.beginPath();
        ctx.moveTo(x - 12, y - 8);
        ctx.lineTo(x - 12, y + h);
        ctx.stroke();

        ctx.font = `11px ${TYPE.body}`;
        ctx.fillStyle = UI.subtle;
        ctx.fillText(`QUEUE  ${game.craftSystem.craftQueue.length}/${CRAFT_QUEUE_MAX}`, x, y - 16);

        if (game.craftSystem.craftQueue.length === 0) {
            ctx.font = `12px ${TYPE.body}`;
            ctx.fillStyle = UI.disabled;
            ctx.fillText('nothing queued', x, y + 6);
            return;
        }

        const rowH = 46;
        game.craftSystem.craftQueue.forEach((job, i) => {
            const ry = y + i * (rowH + 6);
            if (ry + rowH > y + h) return;
            const recipe = RECIPES[job.recipeIndex];
            const active = i === 0;
            this.roundRect(x, ry, w, rowH, 10);
            ctx.fillStyle = active ? UI.selected : UI.hover;
            ctx.fill();

            // A progress sliver along the bottom of the row being made.
            if (active) {
                const done = 1 - job.remaining / Math.max(0.001, recipe.seconds);
                ctx.fillStyle = UI.accentInk;
                ctx.fillRect(x + 2, ry + rowH - 4, (w - 4) * Math.max(0, Math.min(1, done)), 2);
            }

            drawItemIcon(ctx, recipe.out, x + 22, ry + rowH / 2, 24);
            ctx.font = `600 12px ${TYPE.body}`;
            ctx.fillStyle = active ? UI.accentInk : UI.ink;
            this.fit(ITEMS[recipe.out].name, x + 40, ry + 8, w - 110);
            ctx.font = `11px ${TYPE.body}`;
            ctx.fillStyle = UI.subtle;
            ctx.fillText(
                active ? `${job.left} left · ${job.remaining.toFixed(1)}s` : `${job.left} queued`,
                x + 40,
                ry + 26,
            );

            const btn = (
                label: string,
                bx: number,
                by: number,
                enabled: boolean,
                run: () => void,
            ): void => {
                const r: Rect = { x: bx, y: by, w: 20, h: 18 };
                const hv = hit(r, game.input.mouseX, game.input.mouseY) && enabled;
                this.roundRect(r.x, r.y, r.w, r.h, 5);
                ctx.fillStyle = hv ? UI.hover : 'rgba(0,0,0,0.06)';
                ctx.fill();
                ctx.font = `11px ${TYPE.body}`;
                ctx.fillStyle = enabled ? UI.ink : UI.disabled;
                ctx.textAlign = 'center';
                ctx.fillText(label, r.x + r.w / 2, r.y + 4);
                ctx.textAlign = 'left';
                if (hv) {
                    game.uiHover = true;
                    if (game.input.mouseClicked) run();
                }
            };
            btn('^', x + w - 70, ry + 5, i > 1, () => game.craftSystem.reorderCraft(i, -1));
            btn('v', x + w - 47, ry + 5, i > 0 && i < game.craftSystem.craftQueue.length - 1, () =>
                game.craftSystem.reorderCraft(i, 1),
            );
            btn('x', x + w - 24, ry + 5, true, () => game.craftSystem.cancelCraft(i));
        });
    }

    /** Rounded rectangle path, used all over the light panels. */
    private roundRect(x: number, y: number, w: number, h: number, r: number): void {
        const ctx = this.ctx;
        ctx.beginPath();
        ctx.moveTo(x + r, y);
        ctx.arcTo(x + w, y, x + w, y + h, r);
        ctx.arcTo(x + w, y + h, x, y + h, r);
        ctx.arcTo(x, y + h, x, y, r);
        ctx.arcTo(x, y, x + w, y, r);
        ctx.closePath();
    }

    private wrap(text: string, x: number, y: number, maxW: number, lineH: number): void {
        const ctx = this.ctx;
        const words = text.split(' ');
        let line = '';
        let ly = y;
        for (const word of words) {
            const test = line ? `${line} ${word}` : word;
            if (ctx.measureText(test).width > maxW && line) {
                ctx.fillText(line, x, ly);
                line = word;
                ly += lineH;
            } else {
                line = test;
            }
        }
        if (line) ctx.fillText(line, x, ly);
    }

    // -------------------------------------------------------------- overlays

    private drawTitle(game: Game, w: number, h: number): void {
        const ui = this.ui;
        const ctx = this.ctx;
        ctx.fillStyle = WORLD.voidBackdrop;
        ctx.fillRect(0, 0, w, h);

        const cardW = Math.min(620, w - 80);
        const cardH = Math.min(500, h - 80);
        const x = Math.round(w / 2 - cardW / 2);
        const y = Math.round(h / 2 - cardH / 2);
        ui.panel(x, y, cardW, cardH, RADIUS.lg, true);

        ui.text('OXIDE', x + cardW / 2, y + 44, {
            size: 'title',
            weight: 600,
            align: 'center',
            color: UI.warn,
        });
        ui.text('A top-down survival island', x + cardW / 2, y + 74, {
            size: 'label',
            color: UI.subtle,
            align: 'center',
        });
        ui.divider(x + SPACE.lg, y + 104, cardW - SPACE.lg * 2);

        const rows: [string, string][] = [
            ['Move', 'WASD  ·  shift to sprint'],
            ['Use held item', 'left mouse'],
            ['Belt', '1-6  ·  tab for inventory'],
            ['Craft', 'c'],
            ['Interact', 'e  ·  g to drop'],
            ['Reload', 'r'],
            ['Map', 'm'],
            ['Build', 'hold the plan, q to change piece'],
        ];
        rows.forEach(([k, v], i) => {
            const ry = y + 126 + i * 26;
            ui.text(k, x + SPACE.lg, ry, { size: 'label', color: UI.subtle });
            ui.text(v, x + SPACE.lg + 150, ry, { size: 'label', weight: 600 });
        });

        ui.divider(x + SPACE.lg, y + cardH - 150, cardW - SPACE.lg * 2);
        ui.text(
            'Three to eight clans live here. Once you have something worth taking, they come for it.',
            x + cardW / 2,
            y + cardH - 136,
            { size: 'caption', color: UI.subtle, align: 'center' },
        );

        const btnW = 200;
        const bx = x + cardW / 2;
        const by = y + cardH - 68;

        // Sandbox sits on its own line above the two real ways to play, so it
        // never gets clicked by someone reaching for single player.
        const sand: Rect = { x: bx - btnW / 2, y: by - 44, w: btnW, h: 32 };
        const sandHover = hit(sand, game.input.mouseX, game.input.mouseY);
        ui.roundRect(sand.x, sand.y, sand.w, sand.h, RADIUS.md);
        this.ctx.fillStyle = sandHover ? UI.hover : UI.buttonOff;
        this.ctx.fill();
        this.ctx.strokeStyle = UI.hairline;
        this.ctx.lineWidth = 1;
        this.ctx.stroke();
        ui.text('Sandbox  ·  creative', sand.x + sand.w / 2, sand.y + 9, {
            size: 'label',
            weight: 600,
            align: 'center',
            color: UI.ink,
        });
        if (sandHover && game.input.mouseClicked) {
            game.audio.unlock();
            game.startSandbox();
        }

        const solo: Rect = { x: bx - btnW - 8, y: by, w: btnW, h: 38 };
        const soloHover = hit(solo, game.input.mouseX, game.input.mouseY);
        ui.button(solo.x, solo.y, solo.w, solo.h, 'Single player  ·  enter', {
            enabled: true,
            hovered: soloHover,
        });
        if (soloHover && game.input.mouseClicked) game.startSingle();

        const online: Rect = { x: bx + 8, y: by, w: btnW, h: 38 };
        const onlineHover = hit(online, game.input.mouseX, game.input.mouseY);
        ui.roundRect(online.x, online.y, online.w, online.h, RADIUS.md);
        this.ctx.fillStyle = onlineHover ? UI.hover : UI.buttonOff;
        this.ctx.fill();
        this.ctx.strokeStyle = UI.hairline;
        this.ctx.lineWidth = 1;
        this.ctx.stroke();
        ui.text('Play online', online.x + online.w / 2, online.y + 12, {
            size: 'body',
            weight: 600,
            align: 'center',
            color: UI.ink,
        });
        if (onlineHover && game.input.mouseClicked) {
            game.audio.unlock();
            game.startOnline(defaultServerUrl(), 'survivor');
        }

        ui.text(
            game.canContinue
                ? 'solo islands are generated fresh  ·  press c to continue your last one'
                : 'solo islands are generated fresh every time',
            x + cardW / 2,
            y + cardH - 26,
            { size: 'caption', color: UI.subtle, align: 'center' },
        );
    }

    /** Server browser: the rooms this server is running, and a button to add one. */
    private drawLobby(game: Game, w: number, h: number): void {
        const ui = this.ui;
        const ctx = this.ctx;
        ctx.fillStyle = WORLD.voidBackdrop;
        ctx.fillRect(0, 0, w, h);

        const cardW = Math.min(680, w - 80);
        const cardH = Math.min(500, h - 80);
        const x = Math.round(w / 2 - cardW / 2);
        const y = Math.round(h / 2 - cardH / 2);
        ui.panel(x, y, cardW, cardH, RADIUS.lg, true);

        const net = game.net;
        const statusText =
            net.status === 'connecting'
                ? 'Connecting…'
                : net.status === 'joining'
                  ? 'Joining…'
                  : net.error
                    ? net.error
                    : `${net.rooms.length} room${net.rooms.length === 1 ? '' : 's'}  ·  ${net.ping}ms`;
        const top = ui.header(x, y, cardW, 'Servers', statusText, 'esc to go back');

        // New room.
        const create: Rect = { x: x + SPACE.lg, y: top + 16, w: 180, h: 34 };
        const createHover = hit(create, game.input.mouseX, game.input.mouseY);
        ui.button(create.x, create.y, create.w, create.h, 'Host a new island', {
            enabled: net.connected,
            hovered: createHover,
        });
        if (createHover && game.input.mouseClicked && net.connected) {
            net.createRoom(`${'Island'} ${Math.floor(Math.random() * 900 + 100)}`, true);
        }

        const refresh: Rect = { x: x + SPACE.lg + 194, y: top + 16, w: 110, h: 34 };
        const refreshHover = hit(refresh, game.input.mouseX, game.input.mouseY);
        ui.roundRect(refresh.x, refresh.y, refresh.w, refresh.h, RADIUS.md);
        ctx.fillStyle = refreshHover ? UI.hover : UI.buttonOff;
        ctx.fill();
        ui.text('Refresh', refresh.x + refresh.w / 2, refresh.y + 11, {
            size: 'body',
            weight: 600,
            align: 'center',
            color: UI.ink,
        });
        if (refreshHover && game.input.mouseClicked) net.refreshRooms();

        ui.divider(x + SPACE.lg, top + 66, cardW - SPACE.lg * 2);

        if (net.rooms.length === 0) {
            ui.text(
                net.connected
                    ? 'No rooms yet. Host one.'
                    : 'No server. Run `npm run server` and reopen this screen.',
                x + cardW / 2,
                top + 110,
                { size: 'label', color: UI.subtle, align: 'center' },
            );
            return;
        }

        for (let i = 0; i < net.rooms.length; i++) {
            const room = net.rooms[i];
            const row: Rect = {
                x: x + SPACE.lg,
                y: top + 82 + i * 46,
                w: cardW - SPACE.lg * 2,
                h: 40,
            };
            if (row.y + row.h > y + cardH - SPACE.lg) break;
            const hovered = hit(row, game.input.mouseX, game.input.mouseY);
            ui.roundRect(row.x, row.y, row.w, row.h, RADIUS.md);
            ctx.fillStyle = hovered ? UI.selected : UI.surfaceAlt;
            ctx.fill();

            ui.text(room.name, row.x + 14, row.y + 8, { size: 'body', weight: 600 });
            ui.text(
                `${room.pvp ? 'pvp' : 'peaceful'}  ·  ${Math.floor(room.age / 60)}m old`,
                row.x + 14,
                row.y + 24,
                { size: 'caption', color: UI.subtle },
            );
            ui.text(`${room.players}/${room.maxPlayers}`, row.x + row.w - 14, row.y + 14, {
                size: 'body',
                weight: 600,
                align: 'right',
                color: room.players >= room.maxPlayers ? UI.warn : UI.ok,
            });
            if (hovered && game.input.mouseClicked) net.joinRoom(room.id);
        }
    }

    private drawHelp(w: number, h: number): void {
        const ui = this.ui;
        ui.scrim(w, h);
        const cardW = Math.min(720, w - 80);
        const cardH = Math.min(540, h - 80);
        const x = Math.round(w / 2 - cardW / 2);
        const y = Math.round(h / 2 - cardH / 2);
        ui.panel(x, y, cardW, cardH);
        const top = ui.header(x, y, cardW, 'Surviving the island', '', 'h or esc to close');

        const body: [string, string][] = [
            [
                'Food and water',
                'Both drain slowly. Hunt with a spear, cook on a campfire, drink at any shore with e.',
            ],
            ['Cold', 'Nights and snow will kill you. Build a fire or wear leather.'],
            [
                'Radiation',
                'Cabins and lighthouses are safe. Airfields and power plants need a hazmat suit.',
            ],
            [
                'Gathering',
                'Hatchets favour wood, pickaxes favour ore. Hemp is picked with e, not hit.',
            ],
            ['Smelting', 'Ore and wood into a furnace, then light it with e.'],
            [
                'Building',
                'Foundations on cells, walls on the edges between them. Everything starts as twig.',
            ],
            ['Upgrading', 'Hold the hammer and click your own piece to take it up a tier.'],
            [
                'Soft side',
                "The pale dot marks a wall's soft side. Melee barely scratches the hard side.",
            ],
            ['Tool cupboard', 'Claims the ground around it and decides who may build there.'],
            [
                'Privacy',
                'You cannot see into, or reach into, a sealed room you have no authority over.',
            ],
            ['Raiding', 'Satchels and C4 open walls. The clans use them on you too.'],
            ['Recon', 'Press m to mark every clan compound and piece on the map.'],
            ['Guns', 'r reloads what you hold. Firing dry starts the reload for you.'],
            ['Dropping', 'g throws the held stack on the ground, or drag it out of the pack.'],
            [
                'Medical',
                'Bandages and syringes apply while you walk. Break into a run and you lose them.',
            ],
            ['Death', 'You drop everything where you fall. A sleeping bag decides where you wake.'],
        ];
        body.forEach(([k, v], i) => {
            const ry = top + 20 + i * 28;
            ui.text(k, x + SPACE.lg, ry, { size: 'label', weight: 600, color: UI.accentInk });
            ui.fit(v, x + SPACE.lg + 150, ry, cardW - SPACE.lg * 2 - 150, {
                size: 'label',
                color: UI.ink,
            });
        });
    }

    /**
     * The pause screen, which is also where settings live.
     *
     * Controls and sound are what a player goes looking for mid-run, and Esc is
     * where they look. The help panel is left alone: that is how the island
     * works, which is a different question to which key does it.
     */
    private drawSettings(game: Game, w: number, h: number): void {
        const ui = this.ui;
        ui.scrim(w, h);
        const cardW = Math.min(660, w - 80);
        const cardH = Math.min(460, h - 80);
        const x = Math.round(w / 2 - cardW / 2);
        const y = Math.round(h / 2 - cardH / 2);
        ui.panel(x, y, cardW, cardH);
        const top = ui.header(x, y, cardW, 'Settings', '', 'esc to resume');

        const tabs: { id: SettingsTab; label: string }[] = [
            { id: 'controls', label: 'Controls' },
            { id: 'display', label: 'Display' },
            { id: 'sound', label: 'Sound' },
        ];
        const tabW = (cardW - SPACE.lg * 2) / tabs.length;
        tabs.forEach((t, i) => {
            const rect: Rect = { x: x + SPACE.lg + i * tabW, y: top + 4, w: tabW - 6, h: 30 };
            const hovered = hit(rect, game.input.mouseX, game.input.mouseY);
            ui.button(rect.x, rect.y, rect.w, rect.h, t.label, {
                enabled: game.settingsTab === t.id,
                hovered,
            });
            if (hovered && game.input.mouseClicked) game.settingsTab = t.id;
        });

        const bodyY = top + 54;
        if (game.settingsTab === 'controls') this.settingsControls(x + SPACE.lg, bodyY, cardW);
        else if (game.settingsTab === 'display')
            this.settingsDisplay(game, x + SPACE.lg, bodyY, cardW - SPACE.lg * 2);
        else this.settingsSound(game, x + SPACE.lg, bodyY, cardW - SPACE.lg * 2);

        // A button as well as the key.
        //
        // The browser reserves [Esc] for leaving fullscreen and no page can
        // take it back, so in fullscreen the key can only ever do one of the
        // two jobs at a time. A thing to click means you never have to choose:
        // close the panel and stay fullscreen.
        const close: Rect = { x: x + cardW - 130, y: y + cardH - 46, w: 106, h: 30 };
        const overClose = hit(close, game.input.mouseX, game.input.mouseY);
        ui.button(close.x, close.y, close.w, close.h, 'Resume', {
            enabled: true,
            hovered: overClose,
        });
        if (overClose && game.input.mouseClicked) game.paused = false;

        if (isFullscreen()) {
            ui.fit(
                'Click Resume to stay fullscreen. Esc leaves fullscreen first.',
                x + SPACE.lg,
                y + cardH - 38,
                cardW - 160,
                { size: 'caption', color: UI.subtle },
            );
        }
    }

    private settingsControls(x: number, y: number, cardW: number): void {
        const ui = this.ui;
        // Two columns, because the full list does not fit down one side.
        const colW = (cardW - SPACE.lg * 2) / 2;
        const cy = [y, y];
        let col = 0;
        for (const section of CONTROLS_REFERENCE) {
            if (col === 0 && cy[0] > y + 190) col = 1;
            const cx = x + col * (colW + SPACE.md);
            ui.text(section.group, cx, cy[col], {
                size: 'label',
                weight: 600,
                color: UI.accentInk,
            });
            cy[col] += 22;
            for (const [key, what] of section.rows) {
                ui.text(key, cx, cy[col], { size: 'caption', weight: 600 });
                ui.fit(what, cx + 74, cy[col], colW - 80, { size: 'caption', color: UI.subtle });
                cy[col] += 18;
            }
            cy[col] += 12;
        }
    }

    private settingsDisplay(game: Game, x: number, y: number, panelW: number): void {
        const ui = this.ui;
        const row: Rect = { x, y, w: panelW, h: 34 };
        const over = hit(row, game.input.mouseX, game.input.mouseY);
        const on = isFullscreen();
        ui.slot(row.x, row.y, row.w, row.h, { hovered: over });
        ui.text('Fullscreen', row.x + 12, row.y + 11, { size: 'label' });
        ui.text(on ? 'On' : 'Off', row.x + row.w - 12, row.y + 11, {
            size: 'label',
            weight: 600,
            align: 'right',
            color: on ? UI.accentInk : UI.subtle,
        });
        // Entering fullscreen has to happen inside a user gesture, which is
        // exactly what this click is.
        if (over && game.input.mouseClicked) toggleFullscreen();

        ui.fit(
            'The view widens rather than zooming, so fullscreen shows more island.',
            x,
            y + 52,
            panelW,
            { size: 'caption', color: UI.subtle },
        );
        ui.fit('Your browser will also leave fullscreen on esc or F11.', x, y + 70, panelW, {
            size: 'caption',
            color: UI.subtle,
        });
    }

    private settingsSound(game: Game, x: number, y: number, panelW: number): void {
        const ui = this.ui;

        // Mute is its own row rather than volume zero, so turning sound off and
        // back on returns you to the level you had chosen.
        const toggle: Rect = { x, y, w: panelW, h: 34 };
        const tHover = hit(toggle, game.input.mouseX, game.input.mouseY);
        ui.slot(toggle.x, toggle.y, toggle.w, toggle.h, { hovered: tHover });
        ui.text('Sound', toggle.x + 12, toggle.y + 11, { size: 'label' });
        ui.text(game.audio.enabled ? 'On' : 'Muted', toggle.x + toggle.w - 12, toggle.y + 11, {
            size: 'label',
            weight: 600,
            align: 'right',
            color: game.audio.enabled ? UI.accentInk : UI.subtle,
        });
        if (tHover && game.input.mouseClicked) {
            game.audio.setEnabled(!game.audio.enabled);
            if (game.audio.enabled) game.audio.craft();
        }

        const barY = y + 68;
        ui.text('Volume', x, barY - 20, { size: 'label' });
        ui.text(`${Math.round(game.audio.volume * 100)}%`, x + panelW, barY - 20, {
            size: 'label',
            align: 'right',
            color: UI.subtle,
        });
        ui.meter(x, barY, panelW, 14, game.audio.volume, UI.accentInk);

        // Dragging sets the level continuously, so you can hear where you are
        // putting it rather than clicking and guessing.
        const track: Rect = { x, y: barY - 8, w: panelW, h: 30 };
        if (hit(track, game.input.mouseX, game.input.mouseY) && game.input.mouseDown) {
            game.audio.setVolume((game.input.mouseX - x) / panelW);
            if (game.input.mouseClicked && game.audio.enabled) game.audio.hit();
        }

        ui.fit(
            'Everything you hear is synthesized on the fly. There are no audio files to load.',
            x,
            barY + 36,
            panelW,
            { size: 'caption', color: UI.subtle },
        );
    }

    /**
     * The death screen, and where to wake up.
     *
     * Every bag you own is a choice, as in Rust: a row each with how far and
     * which way it is from where you fell, then a beach at the bottom for a
     * fresh start. Click one, or press its number; B is the beach.
     */
    private drawDeath(game: Game, w: number, h: number): void {
        const ui = this.ui;
        this.ctx.fillStyle = 'rgba(20, 4, 4, 0.55)';
        this.ctx.fillRect(0, 0, w, h);
        const bags = game.myBags().slice(0, 9);
        const rowH = 40;
        const cardW = 440;
        const listTop = 150;
        const cardH = listTop + (bags.length + 1) * rowH + 20;
        const x = Math.round(w / 2 - cardW / 2);
        const y = Math.round(h / 2 - cardH / 2);
        ui.panel(x, y, cardW, cardH);

        ui.text('You died', x + cardW / 2, y + 34, {
            size: 'title',
            weight: 600,
            align: 'center',
            color: UI.warn,
        });
        ui.wrap(
            bags.length > 0
                ? 'Your things are on the ground where you fell. Choose where to wake up.'
                : 'Your things are on the ground where you fell. You have no bag to wake up in.',
            x + SPACE.lg,
            y + 72,
            cardW - SPACE.lg * 2,
            18,
            { size: 'label', color: UI.subtle },
        );
        ui.text(`Survived to day ${game.day}.`, x + cardW / 2, y + 116, {
            size: 'label',
            color: UI.ink,
            align: 'center',
        });

        const ready = game.player.respawnTimer <= 0;
        const p = game.player;
        const options: { label: string; bagId: number | null }[] = bags.map((b, i) => {
            const d = Math.round(Math.hypot(b.x - p.x, b.y - p.y) / CELL);
            return {
                label: `${i + 1}  ·  Sleeping bag, ${d} cells ${compass(b.x - p.x, b.y - p.y)}`,
                bagId: b.id,
            };
        });
        options.push({ label: 'B  ·  A beach, with nothing', bagId: null });
        options.forEach((o, i) => {
            const r: Rect = {
                x: x + SPACE.lg,
                y: y + listTop + i * rowH,
                w: cardW - SPACE.lg * 2,
                h: 32,
            };
            const hovered = ready && hit(r, game.input.mouseX, game.input.mouseY);
            ui.button(
                r.x,
                r.y,
                r.w,
                r.h,
                ready ? o.label : `Respawn in ${Math.ceil(p.respawnTimer)}`,
                { enabled: ready, hovered },
            );
            if (hovered && game.input.mouseClicked) game.respawn(o.bagId);
        });
    }

    // ------------------------------------------------------------------ util

    private tooltip(id: ItemId, mx: number, my: number): void {
        const ui = this.ui;
        const def = ITEMS[id];
        this.ctx.font = `11px ${TYPE.body}`;
        const w =
            Math.max(
                this.ctx.measureText(def.desc).width,
                this.ctx.measureText(def.name).width + 30,
            ) + 28;
        const x = mx + 14;
        const y = my + 14;
        ui.glass(x, y, w, 52, RADIUS.sm);
        drawItemIcon(this.ctx, id, x + 18, y + 18, 22);
        ui.textOnDark(def.name, x + 34, y + 10, { size: 'label', weight: 600 });
        ui.fit(def.desc, x + 14, y + 32, w - 28, { size: 'caption', color: UI.onDarkSubtle });
        ui.text(def.stack > 1 ? `max ${def.stack}` : 'one per slot', x + w - 14, y + 10, {
            size: 'micro',
            color: UI.onDarkFaint,
            align: 'right',
        });
    }

    private dim(w: number, h: number, alpha = 0.55): void {
        this.ctx.fillStyle = `rgba(0,0,0,${alpha})`;
        this.ctx.fillRect(0, 0, w, h);
    }

    /** Draw text clipped to a width, with an ellipsis when it will not fit. */
    private fit(text: string, x: number, y: number, maxW: number): void {
        const ctx = this.ctx;
        if (ctx.measureText(text).width <= maxW) {
            ctx.fillText(text, x, y);
            return;
        }
        let out = text;
        while (out.length > 1 && ctx.measureText(`${out}…`).width > maxW) out = out.slice(0, -1);
        ctx.fillText(`${out}…`, x, y);
    }
}

/** Which way something lies, as a point of the compass. Screen up is north. */
function compass(dx: number, dy: number): string {
    if (Math.hypot(dx, dy) < CELL) return 'here';
    const names = ['E', 'SE', 'S', 'SW', 'W', 'NW', 'N', 'NE'];
    const a = Math.atan2(dy, dx);
    return names[(Math.round(a / (Math.PI / 4)) + 8) % 8];
}
