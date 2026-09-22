import { BOW_DRAW_SECONDS } from 'src/features/items/constants/bow-draw-seconds.constant';
import { UI } from 'src/shared/design/constants/ui.constant';
import { wornId } from 'src/features/items/utils/worn-id.util';
import { MONUMENT_NO_BUILD_MARGIN } from 'src/features/world/constants/monument-no-build.constant';
import { escapeSpentOnFullscreen } from 'src/shared/core/fullscreen';
import { addItem } from 'src/features/items/utils/add-item.util';
import { containerRoom } from 'src/features/items/utils/container-room.util';
import { transferSeconds } from 'src/features/items/utils/transfer-seconds.util';
import { Transfer } from 'src/features/items/types/transfer.interface';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { regrowthSeconds } from 'src/features/world/utils/regrowth-seconds.util';
import { REGROWTH } from 'src/features/world/constants/regrowth.constant';
import { SettingsTab } from 'src/features/ui/types/settings-tab.type';
import { GameMode } from 'src/app/types/game-mode.type';
import { Panel } from 'src/app/types/panel.type';
import { Phase } from 'src/app/types/phase.type';
import { BuildSystem } from 'src/features/building/building';
import { CELL } from 'src/features/building/constants/cell.constant';
import { CONTAINER_REACH_SLACK } from 'src/features/building/constants/container-reach-slack.constant';
import { RenderLerp } from 'src/features/render/render-lerp';
import { RenderLerpable } from 'src/features/render/render-lerp';
import { RemoteBuild } from 'src/features/building/remote-build';
import { StructureSystem } from 'src/features/building/structures';
import { BuildKind } from 'src/features/building/types/build-kind.type';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { ClanSystem } from 'src/features/clans/clans';
import { Combat } from 'src/features/combat/combat';
import { MELEE_KNOCKBACK } from 'src/features/combat/constants/melee-knockback.constant';
import { CraftSystem } from 'src/features/crafting/crafting';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { HeldItemSystem } from 'src/features/items/held-item';
import { InteractionSystem } from 'src/features/items/interaction';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { ItemStack } from 'src/features/items/types/item-stack.interface';
import { acquire } from 'src/features/items/utils/acquire.util';
import { makeContainer } from 'src/features/items/utils/make-container.util';
import { NetClient } from 'src/features/net/client';
import { WILDLIFE_SPAWN } from 'src/features/npcs/constants/wildlife-spawn.constant';
import { NpcSystem } from 'src/features/npcs/npcs';
import { ZOOM_MAX } from 'src/features/render/constants/zoom-max.constant';
import { ZOOM_MIN } from 'src/features/render/constants/zoom-min.constant';
import { AUTOSAVE_SECONDS } from 'src/features/session/constants/autosave-seconds.constant';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { SWIM } from 'src/features/survival/constants/swim.constant';
import { SurvivalSystem } from 'src/features/survival/survival';
import { PlayerState } from 'src/features/survival/types/player-state.interface';
import { DAY_SECONDS } from 'src/features/time/constants/day-seconds.constant';
import { NIGHT_DARKNESS } from 'src/features/time/constants/night-darkness.constant';
import { NIGHT_FRACTION } from 'src/features/time/constants/night-fraction.constant';
import { TWILIGHT } from 'src/features/time/constants/twilight.constant';
import { MONUMENTS } from 'src/features/world/constants/monuments.constant';
import { WILDLIFE } from 'src/features/world/constants/wildlife.constant';
import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';
import { LootCrate } from 'src/features/world/types/loot-crate.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { World } from 'src/features/world/world';
import { TAU } from 'src/shared/constants/tau.constant';
import { Audio } from 'src/shared/core/audio';
import { Camera } from 'src/shared/core/camera';
import { Input } from 'src/shared/core/input';
import { Particles } from 'src/shared/core/particles';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { clamp } from 'src/shared/utils/clamp.util';
import { dist } from 'src/shared/utils/dist.util';
import { randRange } from 'src/shared/utils/rand-range.util';

const HOTBAR_SIZE = 6;
const INVENTORY_SIZE = 24;

export class Game {
    world!: World;
    build!: BuildSystem;
    npcs!: NpcSystem;
    combat!: Combat;
    clanSystem!: ClanSystem;
    craftSystem!: CraftSystem;
    survival!: SurvivalSystem;
    held!: HeldItemSystem;
    interaction!: InteractionSystem;
    structures!: StructureSystem;
    /** The room's building, put on the local map. Online only. */
    remoteBuild!: RemoteBuild;
    particles = new Particles();
    camera: Camera;
    audio = new Audio();

    phase: Phase = 'title';
    /** Single-player against the AI, or an authoritative server room. */
    mode: GameMode = 'single';
    net: NetClient = new NetClient();
    panel: Panel = 'none';
    paused = false;
    /** Which page of the Esc settings panel is showing. */
    settingsTab: SettingsTab = 'controls';
    uiHover = false;
    private edgeStep = true;

    player!: PlayerState;

    /** Layered crafting menu state. */
    craftCategory = 'tools';
    craftSelection: number | null = null;
    /** How many of the selected recipe to queue. */
    craftAmount = 1;
    /** The inventory slot being inspected, if any. */
    inspecting: { container: Container; index: number } | null = null;
    /** Recon overlay: shows every clan compound and clan-owned piece on the map. */
    revealClans = false;
    /** Smoothed frames per second, fed by the render loop. */
    fps = 60;

    /** Seconds since the world was created; drives the day/night cycle. */
    clock = 0;
    day = 1;

    buildKind: BuildKind = 'foundation';
    openContainer: {
        container: Container;
        title: string;
        source: Deployable | LootCrate | null;
    } | null = null;
    /** Slot the player picked up in the inventory UI, if any. */
    dragging: { from: Container; index: number } | null = null;
    /**
     * Stacks being carried between containers by right-click.
     *
     * Several at once, each with its own spinner on its own slot, so emptying a
     * crate is a handful of clicks rather than a queue you wait through. One
     * per slot: right-clicking a slot that is already moving does nothing.
     */
    transfers: Transfer[] = [];

    message = '';
    /** Whether the title screen should offer to resume a saved island. */
    canContinue = false;
    private messageTimer = 0;
    private autosaveTimer = AUTOSAVE_SECONDS;

    /** Owner ids keep climbing so a new clan never inherits a dead one's pieces. */
    /** How many clans the island should currently support. */

    constructor(
        public input: Input,
        viewW: number,
        viewH: number,
    ) {
        this.camera = new Camera(viewW, viewH);
    }

    // ------------------------------------------------------------ life cycle

    newWorld(seed = Math.floor(Math.random() * 1e9)): void {
        this.world = new World(seed);
        this.build = new BuildSystem();
        this.particles.clear();
        // Wake up at dawn rather than at midnight in the pitch dark.
        this.clock = DAY_SECONDS * 0.25;
        this.day = 1;

        this.combat = new Combat(this.world, this.build, {
            npcs: () => this.npcs.list,
            playerPos: () => ({
                x: this.player.x,
                y: this.player.y,
                radius: PLAYER.radius,
                alive: this.player.alive,
            }),
            hitNpc: (n, dmg, fx, fy, byClan) =>
                this.npcs.damage(n, dmg, fx, fy, MELEE_KNOCKBACK, byClan ?? 0),
            hitPlayer: (dmg, fx, fy) => this.survival.hurtPlayer(dmg, fx, fy),
            hitBreakable: (n, dmg) => this.held.strikeBarrel(n, dmg),
            hitStructure: (id, dmg, fx, fy, melee) => {
                this.structures.damageBuilt(id, dmg, fx, fy, melee);
                // Raiding is shared: the server keeps the health everybody
                // agrees on and says when a piece is gone. Combat itself is
                // still client-side, so this is a report, not a request.
                if (this.net.online) this.net.reportDamage(id, dmg);
            },
            explosion: (x, y, r, dmg, owner, stuckTo) =>
                this.structures.explode(x, y, r, dmg, owner, stuckTo),
        });

        this.npcs = new NpcSystem(this.world, this.build, {
            damagePlayer: (dmg, fx, fy) => this.survival.hurtPlayer(dmg, fx, fy),
            fire: (x, y, a, dmg, speed, range, clan) =>
                this.combat.fire(
                    x,
                    y,
                    a,
                    dmg,
                    speed,
                    range,
                    'hostile',
                    WORLD.hostileTracer,
                    clan ?? 0,
                ),
            attackStructure: (id, dmg, fx, fy) =>
                this.structures.damageBuilt(id, dmg, fx, fy, true),
            plantCharge: (npc, s) => {
                // Read from the item, not retyped: these had already drifted
                // to a 3.4s fuse against the satchel's own 3.2s.
                const charge = ITEMS.satchel.boom!;
                this.combat.attachExplosive(
                    'satchel',
                    s,
                    charge.damage,
                    charge.radius,
                    charge.fuse,
                    'hostile',
                );
                this.particles.text(npc.x, npc.y - 26, 'charge planted', WORLD.hostileTracer);
                this.audio.build();
            },
            scheduleRegrowth: (kind) =>
                this.wildlifeRegrowth.push({ kind, seconds: regrowthSeconds() }),
            dropLoot: (kind, x, y, loot, weapon) =>
                this.structures.dropLootTable(kind, x, y, loot, weapon),
            raidTarget: () =>
                this.clanSystem.raid
                    ? { x: this.clanSystem.raid.x, y: this.clanSystem.raid.y }
                    : null,
            weakestStructureNear: (x, y, r) => this.structures.weakestStructureNear(x, y, r),
        });

        this.clanSystem = new ClanSystem(
            this.world,
            this.build,
            this.npcs,
            this.combat,
            this.particles,
            this.audio,
            {
                clock: () => this.clock,
                isNight: () => this.isNight,
                notify: (text) => this.notify(text),
                player: () => this.player,
            },
        );

        this.craftSystem = new CraftSystem(this.build, this.audio, {
            containers: () => this.containers,
            sandbox: () => this.sandbox,
            player: () => this.player,
            dropStack: (stack, x, y) => this.interaction.dropStack(stack, x, y),
            notify: (text) => this.notify(text),
        });

        this.survival = new SurvivalSystem(
            this.world,
            this.build,
            this.particles,
            this.audio,
            this.camera,
            {
                player: () => this.player,
                containers: () => this.containers,
                heldItem: () => this.heldItem(),
                sandbox: () => this.sandbox,
                darkness: () => this.darkness,
                dropStack: (stack, x, y) => this.interaction.dropStack(stack, x, y),
                notify: (text) => this.notify(text),
                die: () => this.die(),
            },
        );

        this.held = new HeldItemSystem(
            this.world,
            this.build,
            this.combat,
            this.npcs,
            this.particles,
            this.audio,
            this.camera,
            {
                player: () => this.player,
                containers: () => this.containers,
                heldItem: () => this.heldItem(),
                sprinting: () => this.sprinting,
                edgeStep: () => this.edgeStep,
                buildKind: () => this.buildKind,
                input: () => this.input,
                sandbox: () => this.sandbox,
                survival: () => this.survival,
                damageBuilt: (id, dmg, fx, fy, melee) =>
                    this.structures.damageBuilt(id, dmg, fx, fy, melee),
                notify: (text) => this.notify(text),
                reportBuild: (kind, gx, gy, side) => {
                    if (this.net.online) this.net.reportBuild(kind, gx, gy, side);
                },
            },
        );

        this.interaction = new InteractionSystem(
            this.world,
            this.build,
            this.particles,
            this.audio,
            {
                player: () => this.player,
                reportDoor: (id, open) => {
                    if (this.net.online) this.net.reportDoor(id, open);
                },
                heldItem: () => this.heldItem(),
                held: () => this.held,
                canReach: (x, y) => this.canReach(x, y),
                harvestableNear: () => this.harvestableNear(),
                nearWater: (x, y) => this.nearWater(x, y),
                openContainer: (oc) => {
                    this.openContainer = oc;
                    this.panel = 'container';
                },
                inspect: (container, index) => {
                    this.inspecting = { container, index };
                },
                notify: (text) => this.notify(text),
            },
        );

        this.remoteBuild = new RemoteBuild(this.build, () => this.net.youId ?? '');
        this.structures = new StructureSystem(
            this.build,
            this.npcs,
            this.particles,
            this.audio,
            this.camera,
            {
                player: () => this.player,
                interaction: () => this.interaction,
                survival: () => this.survival,
                viewingContainerOf: (d) => this.openContainer?.source === d,
                closeContainer: () => {
                    this.openContainer = null;
                    if (this.panel === 'container') this.panel = 'none';
                },
            },
        );

        // Nothing regrows through somebody's floor, inside their walls, or under
        // a deployable.
        this.build.naturalBlocked = (x, y, r) => {
            const n = this.world.naturalAt(x, y, r);
            if (!n) return null;
            return n.kind === 'tree'
                ? 'Chop the tree down first'
                : n.kind === 'nettle'
                  ? 'Pick the nettle first'
                  : 'Mine the node out first';
        };
        // Monument ground belongs to everybody. See MONUMENT_NO_BUILD_MARGIN.
        this.build.monumentBlocked = (x, y) => {
            for (const m of this.world.monuments) {
                if (dist(x, y, m.x, m.y) > m.radius + MONUMENT_NO_BUILD_MARGIN) continue;
                const def = MONUMENTS.find((d) => d.id === m.defId);
                return `${def?.name ?? 'This place'} is open ground. Nobody builds here.`;
            }
            return null;
        };
        this.world.respawnBlocked = (x, y) => {
            const gx = Math.floor(x / CELL);
            const gy = Math.floor(y / CELL);
            if (this.build.foundationAt(gx, gy)) return true;
            if (this.build.deployableAt(gx, gy)) return true;
            return this.build.regionAt(x, y) !== 0;
        };
        this.npcs.attachSurvivorWorld(this.clanSystem.survivorWorld());
        this.resetPlayer();
        this.spawnWildlife();
        this.spawnScientists();
        this.clanSystem.spawnClans();
        // Placed last, so the beach check can see every cupboard the clans just
        // put down and drop you somewhere that is nobody's back garden.
        const start = this.beachSpawn();
        this.player.x = start.x;
        this.player.y = start.y;
        this.camera.x = this.player.x;
        this.camera.y = this.player.y;
    }

    /** A free spot of sand anywhere on the coast, outside every cupboard's claim. */
    private beachSpawn(): { x: number; y: number } {
        return this.world.pickBeachSpawn((x, y) => this.build.privilegeAt(x, y) !== null);
    }

    private resetPlayer(): void {
        const hotbar = makeContainer(HOTBAR_SIZE);
        hotbar.slots[0] = { id: 'rock', count: 1 };
        this.player = {
            x: this.world.spawn.x,
            y: this.world.spawn.y,
            vx: 0,
            vy: 0,
            facing: -Math.PI / 2,
            health: PLAYER.maxHealth,
            calories: PLAYER.maxCalories * 0.75,
            hydration: PLAYER.maxHydration * 0.75,
            temperature: 20,
            radiation: 0,
            alive: true,
            respawnTimer: 0,
            invuln: 0,
            hurtFlash: 0,
            attackTimer: 0,
            swingAnim: 0,
            bowDraw: 0,
            walkPhase: 0,
            activeSlot: 0,
            inventory: makeContainer(INVENTORY_SIZE),
            hotbar,
            worn: makeContainer(1),
            healOverTime: 0,
            bleeding: 0,
            using: null,
            useLeft: 0,
            useTotal: 0,
            reloadLeft: 0,
            reloadTotal: 0,
        };
    }

    /**
     * Animals waiting to come back, each with a day on the clock.
     *
     * Kept as a queue of kinds rather than remembering where each one died: the
     * island keeps its population, but it repopulates somewhere else, so the
     * stretch you hunted out stays quiet even after the day is up.
     */
    private wildlifeRegrowth: { kind: NpcKind; seconds: number }[] = [];

    private updateWildlifeRegrowth(dt: number): void {
        for (let i = this.wildlifeRegrowth.length - 1; i >= 0; i--) {
            const pending = this.wildlifeRegrowth[i];
            pending.seconds -= dt;
            if (pending.seconds > 0) continue;
            // Try once; if there is nowhere clear right now, wait a little and
            // try again rather than losing the animal off the island for good.
            if (this.spawnOneAnimal(pending.kind)) this.wildlifeRegrowth.splice(i, 1);
            else pending.seconds = REGROWTH.blockedRetrySeconds;
        }
    }

    /** One animal, somewhere legal and away from the player. */
    private spawnOneAnimal(kind: NpcKind): boolean {
        for (let tries = 0; tries < WILDLIFE_SPAWN.tries; tries++) {
            const x = randRange(
                Math.random,
                WILDLIFE_SPAWN.margin,
                WORLD_W - WILDLIFE_SPAWN.margin,
            );
            const y = randRange(
                Math.random,
                WILDLIFE_SPAWN.margin,
                WORLD_H - WILDLIFE_SPAWN.margin,
            );
            const b = this.world.biomeAt(x, y);
            if (b === 'water' || b === 'road') continue;
            if (this.world.monumentAt(x, y)) continue;
            if (dist(x, y, this.player.x, this.player.y) < WILDLIFE_SPAWN.clearOfPlayer) continue;
            this.npcs.spawn(kind, x, y, { leash: WILDLIFE_SPAWN.leash });
            return true;
        }
        return false;
    }

    private spawnWildlife(): void {
        for (const [kind, n] of WILDLIFE) {
            for (let i = 0; i < n; i++) {
                for (let tries = 0; tries < WILDLIFE_SPAWN.tries; tries++) {
                    const x = randRange(
                        Math.random,
                        WILDLIFE_SPAWN.margin,
                        WORLD_W - WILDLIFE_SPAWN.margin,
                    );
                    const y = randRange(
                        Math.random,
                        WILDLIFE_SPAWN.margin,
                        WORLD_H - WILDLIFE_SPAWN.margin,
                    );
                    const b = this.world.biomeAt(x, y);
                    if (b === 'water' || b === 'road') continue;
                    if (this.world.monumentAt(x, y)) continue;
                    if (dist(x, y, this.player.x, this.player.y) < WILDLIFE_SPAWN.clearOfPlayer)
                        continue;
                    this.npcs.spawn(kind, x, y, { leash: WILDLIFE_SPAWN.leash });
                    break;
                }
            }
        }
        this.wildlifeRegrowth = [];
    }

    private spawnScientists(): void {
        for (const m of this.world.monuments) {
            const def = MONUMENTS.find((d) => d.id === m.defId);
            if (!def) continue;
            for (let i = 0; i < def.scientists; i++) {
                const a = (i / def.scientists) * TAU + Math.random();
                const r = randRange(Math.random, def.radius * 0.25, def.radius * 0.8);
                this.npcs.spawn('scientist', m.x + Math.cos(a) * r, m.y + Math.sin(a) * r, {
                    home: { x: m.x, y: m.y },
                    leash: def.radius,
                });
            }
        }
    }

    /** Start a fresh solo island against the AI. */
    startSingle(): void {
        this.mode = 'single';
        this.net.disconnect();
        this.newWorld();
        this.phase = 'play';
    }

    /** Creative mode: free building, free crafting, nothing can kill you. */
    startSandbox(): void {
        this.mode = 'sandbox';
        this.net.disconnect();
        this.newWorld();
        this.phase = 'play';
        this.notify('Sandbox. Nothing costs anything and nothing can kill you.');
    }

    get sandbox(): boolean {
        return this.mode === 'sandbox';
    }

    /** Open the server browser. The world is built once a room is joined. */
    startOnline(serverUrl: string, name: string): void {
        this.mode = 'online';
        this.phase = 'lobby';
        this.net.onJoined = (seed) => {
            // Everyone in a room generates the identical island from its seed, so
            // only players and their building need to travel over the wire.
            this.newWorld(seed);
            this.player.x = this.net.predictedX;
            this.player.y = this.net.predictedY;
            this.camera.x = this.player.x;
            this.camera.y = this.player.y;
            // The AI island belongs to single-player; a room is people against people.
            this.npcs.clear();
            this.clanSystem.clans = [];
            this.phase = 'play';
            this.notify('Joined the island.');
        };
        this.net.onChat = (from, text) => this.notify(`${from}: ${text}`);
        // The server owns what is built. These four put its answer on the map,
        // so a wall somebody else raised is the same object as one you raised.
        // Prediction has to answer the same question the server does about
        // what is solid, or replaying inputs walks through walls.
        this.net.solid = (x, y, radius) => this.build.resolve(x, y, radius);
        this.net.onBuildSnapshot = (structures, deployables) => {
            this.remoteBuild.applySnapshot(structures, deployables);
            this.npcs.invalidateNavigation();
        };
        this.net.onBuilt = (piece) => {
            this.remoteBuild.applyBuilt(piece);
            this.npcs.invalidateNavigation();
        };
        this.net.onDeployed = (piece) => {
            this.remoteBuild.applyDeployed(piece);
            this.npcs.invalidateNavigation();
        };
        this.net.onDestroyed = (id) => {
            this.remoteBuild.applyDestroyed(id);
            this.npcs.invalidateNavigation();
        };
        this.net.onDoor = (id, open) => this.remoteBuild.applyDoor(id, open);
        // Your own placement went down the moment you clicked. If the server
        // will not have it, say so rather than leaving you with a wall nobody
        // else can see.
        this.net.onRefused = (r) => {
            this.remoteBuild.applyRefused(r.kind, r.gx, r.gy, r.side);
            this.npcs.invalidateNavigation();
            this.notify(r.reason);
        };
        this.net.connect(serverUrl, name);
    }

    // ---------------------------------------------------------------- sandbox

    /** Creative mode only: conjure a stack straight into the pack. */
    giveItem(id: ItemId, amount: number): void {
        if (!this.sandbox) return;
        const left = acquire(this.player.hotbar, this.player.inventory, id, amount);
        const got = amount - left;
        if (got <= 0) {
            this.audio.deny();
            this.notify('No room for that.');
            return;
        }
        this.audio.craft();
        this.notify(`+${got} ${ITEMS[id].name}.`);
    }

    /** Creative mode only: shove the clock forward. */
    skipTime(hours: number): void {
        if (!this.sandbox) return;
        this.clock += hours * (DAY_SECONDS / 24);
        this.notify(`Clock moved ${hours > 0 ? 'forward' : 'back'} ${Math.abs(hours)}h.`);
    }

    notify(text: string): void {
        this.message = text;
        this.messageTimer = 5;
    }

    // ------------------------------------------------------------------ time

    get dayFraction(): number {
        return (this.clock % DAY_SECONDS) / DAY_SECONDS;
    }

    /** 0 at noon, 1 at midnight. The single source for everything time-of-day. */
    get nightness(): number {
        return Math.abs(this.dayFraction - 0.5) * 2;
    }

    get isNight(): boolean {
        return this.nightness > 1 - NIGHT_FRACTION;
    }

    /** Screen darkness. Ramps in through twilight and holds through the night. */
    get darkness(): number {
        const threshold = 1 - NIGHT_FRACTION - TWILIGHT;
        return clamp((this.nightness - threshold) / (TWILIGHT + 0.1), 0, 1) * NIGHT_DARKNESS;
    }

    // ------------------------------------------------------------------ loop

    /** Blending between simulation steps, so drawing is not tied to them. */
    private readonly renderLerp = new RenderLerp();

    /**
     * Smooth the other survivors, at the display's rate rather than the
     * simulation's. Their positions arrive 30 times a second, and drawing them
     * as they land made everybody else step while your own survivor glided.
     */
    smoothRoom(): void {
        if (this.mode === 'online') this.net.interpolate();
    }

    /**
     * Everything whose drawn position should be blended between steps.
     *
     * Other survivors are left out online: they are already smoothed once a
     * frame against the server's clock, and blending twice would only add lag.
     */
    private lerpables(): RenderLerpable[] {
        return [this.player, ...this.npcs.list];
    }

    /** Called before each frame is drawn, with how far through the step we are. */
    beginFrame(alpha: number): void {
        this.renderLerp.apply(this.lerpables(), alpha);
    }

    /** Called straight after drawing, so nothing else sees a blended position. */
    endFrame(): void {
        this.renderLerp.restore();
    }

    update(dt: number, firstStep = true): void {
        // Where everything is before this step is where the next frame blends
        // from.
        this.renderLerp.capture(this.lerpables());
        this.camera.update(dt);
        this.edgeStep = firstStep;

        if (this.phase === 'title') {
            if (firstStep && this.input.justPressed('Enter')) {
                this.audio.unlock();
                this.startSingle();
            }
            return;
        }

        if (this.phase === 'lobby') {
            this.particles.update(dt);
            return;
        }

        if (this.paused || this.panel === 'help') return;

        this.clock += dt;
        this.day = 1 + Math.floor(this.clock / DAY_SECONDS);
        if (this.messageTimer > 0) {
            this.messageTimer -= dt;
            if (this.messageTimer <= 0) this.message = '';
        }

        this.survival.updateSurvival(dt);
        this.smoothRoom();
        this.updatePlayer(dt);
        this.npcs.update(dt, this.player);
        this.combat.update(dt);
        this.build.update(dt);
        this.world.update(dt);
        this.updateWildlifeRegrowth(dt);
        this.updateTransfer(dt);
        this.structures.updateDeployables(dt);
        // Walking away from, or getting shut out of, an open container closes it.
        const oc = this.openContainer?.source as { x?: number; y?: number } | null;
        if (oc && oc.x !== undefined && oc.y !== undefined) {
            const away =
                dist(this.player.x, this.player.y, oc.x, oc.y) >
                PLAYER.interact + CONTAINER_REACH_SLACK;
            if (away || !this.canReach(oc.x, oc.y)) {
                this.openContainer = null;
                if (this.panel === 'container') this.panel = 'none';
            }
        }
        this.craftSystem.updateCrafting(dt);
        this.interaction.updateGround(dt);
        this.clanSystem.updateClans(dt);
        this.particles.update(dt);

        this.camera.follow(this.player.x, this.player.y, WORLD_W, WORLD_H, dt);

        this.autosaveTimer -= dt;
        if (this.autosaveTimer <= 0) {
            this.autosaveTimer = AUTOSAVE_SECONDS;
            this.requestSave();
        }
    }

    /** Set by main.ts so the game can persist without importing storage here. */
    onSave: (() => void) | null = null;

    private requestSave(): void {
        this.onSave?.();
    }

    /**
     * Called once per rendered frame, before any simulation steps.
     *
     * This deliberately does NOT live inside `update`. With the frame rate capped
     * above the 60Hz simulation rate, plenty of frames run zero simulation steps,
     * and a keypress landing on one of those was swallowed when the input edges
     * were cleared at the end of the frame. That is why changing belt slot while
     * moving "did not always work".
     */
    pollInput(): void {
        if (this.phase === 'title' || this.phase === 'lobby') return;
        this.handleKeys();
    }

    private handleKeys(): void {
        const i = this.input;
        if (i.justPressed('F1') || i.justPressed('KeyH')) {
            this.panel = this.panel === 'help' ? 'none' : 'help';
        }
        // The browser spends [Esc] on leaving fullscreen and the page cannot
        // stop it. When that happens the keypress is gone: whatever is open
        // stays open and the next [Esc] closes it.
        if (i.justPressed('Escape') && escapeSpentOnFullscreen()) return;
        if (
            i.justPressed('Escape') &&
            this.mode === 'online' &&
            this.panel === 'none' &&
            !this.paused
        ) {
            this.net.leaveRoom();
            this.remoteBuild.reset();
            this.phase = 'lobby';
            return;
        }
        if (i.justPressed('Escape')) {
            if (this.panel !== 'none') {
                this.panel = 'none';
                this.openContainer = null;
                this.dragging = null;
            } else this.paused = !this.paused;
        }
        if (this.panel === 'help' || this.paused) return;

        // Tab is the one key that means "put this away". Whatever is open closes;
        // with nothing open it brings up the pack. Inventory, crafting and an open
        // crate are three tabs of one screen, so you switch between them by
        // clicking rather than by closing one to open the next.
        if (i.justPressed('Tab')) {
            this.panel = this.panel === 'none' ? 'inventory' : 'none';
            this.dragging = null;
        }
        if (i.justPressed('KeyC')) {
            this.panel = this.panel === 'craft' ? 'none' : 'craft';
        }
        for (let s = 0; s < HOTBAR_SIZE; s++) {
            if (i.justPressed(`Digit${s + 1}`)) this.player.activeSlot = s;
        }
        if (i.justPressed('KeyQ') && this.heldItem()?.id === 'building_plan') this.cycleBuildKind();
        if (i.justPressed('KeyE')) this.interaction.interact();
        if (i.justPressed('KeyR')) this.survival.reloadHeld();
        if (i.justPressed('KeyG')) this.interaction.dropHeld();
        // M opens the whole island. Recon is a button on the map, not a key.
        if (i.justPressed('KeyM')) this.panel = this.panel === 'map' ? 'none' : 'map';
        if (this.sandbox) {
            if (i.justPressed('KeyF')) this.panel = this.panel === 'sandbox' ? 'none' : 'sandbox';
            if (i.justPressed('BracketRight')) this.skipTime(1);
            if (i.justPressed('BracketLeft')) this.skipTime(-1);
        }

        if (i.wheel !== 0) {
            if (this.heldItem()?.id === 'building_plan') this.cycleBuildKind(i.wheel > 0 ? 1 : -1);
            else
                this.camera.zoom = clamp(
                    this.camera.zoom * (i.wheel > 0 ? 0.9 : 1.1),
                    ZOOM_MIN,
                    ZOOM_MAX,
                );
        }
    }

    toggleReveal(): void {
        this.revealClans = !this.revealClans;
        this.notify(this.revealClans ? 'Recon on: clan holdings marked.' : 'Recon off.');
    }

    private cycleBuildKind(dir = 1): void {
        const order: BuildKind[] = ['foundation', 'wall', 'doorway', 'door'];
        const idx = order.indexOf(this.buildKind);
        this.buildKind = order[(idx + dir + order.length) % order.length];
    }

    // -------------------------------------------------------------- survival

    // ---------------------------------------------------------------- player

    heldItem(): ItemStack | null {
        return this.player.hotbar.slots[this.player.activeSlot];
    }

    get containers(): Container[] {
        return [this.player.hotbar, this.player.inventory];
    }

    /** True while the player is running; recomputed every movement update. */
    sprinting = false;

    /** True while the player is standing in open water. */
    get swimming(): boolean {
        return this.player.alive && this.world.biomeAt(this.player.x, this.player.y) === 'water';
    }

    private die(): void {
        const p = this.player;
        p.health = 0;
        p.alive = false;
        p.respawnTimer = 4;
        this.phase = 'dead';
        this.panel = 'none';
        this.openContainer = null;
        this.audio.gameOver();
        this.camera.shake(12, 0.6);
        this.particles.burst(p.x, p.y, 30, '#a01f1f', { speed: 220, life: 1.1, size: 4 });

        // Everything you carried spills onto the ground where you fell.
        for (const c of [p.hotbar, p.inventory]) {
            for (let i = 0; i < c.slots.length; i++) {
                const s = c.slots[i];
                if (!s) continue;
                this.interaction.dropStack(s, p.x, p.y);
                c.slots[i] = null;
            }
        }
        const worn = wornId(p);
        if (worn) {
            this.interaction.dropStack({ id: worn, count: 1 }, p.x, p.y);
            p.worn.slots[0] = null;
        }
    }
    /**
     * Start carrying a slot across to the other container.
     *
     * Refused rather than queued if one is already running, and refused if the
     * far side has no room, so the spinner never runs on a move that cannot
     * land.
     */
    beginTransfer(from: Container, index: number, to: Container): void {
        if (this.transferAt(from, index)) return;
        const stack = from.slots[index];
        if (!stack) return;
        const room = containerRoom(to, stack.id);
        if (room <= 0) {
            this.audio.deny();
            this.message = `No room for ${ITEMS[stack.id].name.toLowerCase()} over there.`;
            return;
        }
        const count = Math.min(stack.count, room);
        this.transfers.push({
            from,
            to,
            index,
            id: stack.id,
            count,
            elapsed: 0,
            duration: transferSeconds(count),
        });
    }

    /** The carry running on this slot, if any. Used by the panel to draw it. */
    transferAt(container: Container, index: number): Transfer | null {
        return this.transfers.find((t) => t.from === container && t.index === index) ?? null;
    }

    /**
     * Tick the carry, and land it when the time is up.
     *
     * The slot is re-checked on arrival rather than trusted: the stack can have
     * been dragged elsewhere, spent, or dropped while the spinner was running.
     */
    private updateTransfer(dt: number): void {
        for (let i = this.transfers.length - 1; i >= 0; i--) {
            const t = this.transfers[i];
            const stack = t.from.slots[t.index];
            // Re-checked on arrival rather than trusted: the stack can have been
            // dragged elsewhere, spent, or dropped while the spinner was running.
            if (!stack || stack.id !== t.id) {
                this.transfers.splice(i, 1);
                continue;
            }
            t.elapsed += dt;
            if (t.elapsed < t.duration) continue;

            this.transfers.splice(i, 1);
            const count = Math.min(t.count, stack.count, containerRoom(t.to, t.id));
            if (count <= 0) continue;
            stack.count -= count;
            if (stack.count <= 0) t.from.slots[t.index] = null;
            addItem(t.to, t.id, count);
            this.audio.pickup();
        }
    }

    /**
     * The single timed job the arc beside the player is showing, if any.
     *
     * One place decides, so the arc can never disagree with what is running,
     * and a new timed action means a case here rather than a new bar.
     */
    get progressJob(): { progress: number; color: string } | null {
        const p = this.player;
        if (p.using && p.useTotal > 0) {
            return { progress: 1 - p.useLeft / p.useTotal, color: WORLD.vital };
        }
        if (p.bowDraw > 0) {
            return { progress: p.bowDraw / BOW_DRAW_SECONDS, color: UI.accentInk };
        }
        if (p.reloadLeft > 0 && p.reloadTotal > 0) {
            return { progress: 1 - p.reloadLeft / p.reloadTotal, color: UI.accentInk };
        }
        return null;
    }

    /** Your sleeping bags, in the order the respawn screen lists them. */
    myBags(): Deployable[] {
        return this.build.deployables
            .filter((d) => d.kind === 'sleeping_bag' && d.owner === 0)
            .sort((a, b) => a.id - b.id);
    }

    /**
     * Wake up again: in the bag you chose, or on a beach if you chose none.
     * A bag that has been destroyed since the screen was drawn is a beach.
     */
    respawn(bagId: number | null = null): void {
        const p = this.player;
        const bag = bagId === null ? undefined : this.myBags().find((d) => d.id === bagId);
        const at = bag ? { x: bag.x, y: bag.y } : this.beachSpawn();
        p.x = at.x;
        p.y = at.y;
        p.health = PLAYER.maxHealth;
        p.calories = PLAYER.maxCalories * 0.5;
        p.hydration = PLAYER.maxHydration * 0.5;
        p.radiation = 0;
        p.bleeding = 0;
        p.alive = true;
        p.hotbar = makeContainer(HOTBAR_SIZE);
        p.hotbar.slots[0] = { id: 'rock', count: 1 };
        p.inventory = makeContainer(INVENTORY_SIZE);
        p.activeSlot = 0;
        this.phase = 'play';
        this.camera.x = p.x;
        this.camera.y = p.y;
        this.notify(bag ? 'Woke up in your bag.' : 'Woke up on the beach with nothing.');
    }

    /**
     * Start using a consumable. Anything with `useSeconds` is a channel: you can
     * keep walking through it, but breaking into a run drops it. The rest apply
     * straight away.
     */

    private updatePlayer(dt: number): void {
        const p = this.player;
        if (!p.alive) return;

        if (p.invuln > 0) p.invuln -= dt;
        if (p.hurtFlash > 0) p.hurtFlash -= dt;
        if (p.attackTimer > 0) p.attackTimer -= dt;
        if (p.swingAnim > 0) p.swingAnim -= dt;

        // Menus do not root you to the spot. You can walk while your pack is open,
        // the same as in Rust; only using the held item is blocked.
        const menuOpen = this.panel !== 'none';
        const axis = this.input.moveAxis();
        const moving = axis.x !== 0 || axis.y !== 0;

        const swimming = this.swimming;
        const wantsSprint = this.input.isDown('ShiftLeft') || this.input.isDown('ShiftRight');
        // Sprinting costs nothing to keep up. What it costs is your hands: you
        // cannot swing, shoot, place or throw while you are running.
        const sprinting = !swimming && wantsSprint && moving && p.calories > 0;
        this.sprinting = sprinting;
        // Walking while you wrap a wound is fine. Breaking into a run is not.
        this.survival.updateUse(dt, sprinting);
        this.survival.updateReload(dt, sprinting);

        let speed = PLAYER.speed * (sprinting ? PLAYER.sprint : 1);
        if (p.calories <= 0 || p.hydration <= 0) speed *= 0.7;
        if (swimming) speed *= SWIM.speedMultiplier;

        p.vx = axis.x * speed;
        p.vy = axis.y * speed;

        if (this.mode === 'online' && this.net.status === 'playing') {
            // Online, the server owns your position. We predict locally and let the
            // netcode fold in corrections. See docs/systems/networking.md.
            this.net.predict(
                dt,
                {
                    up: this.input.isDown('KeyW') || this.input.isDown('ArrowUp'),
                    down: this.input.isDown('KeyS') || this.input.isDown('ArrowDown'),
                    left: this.input.isDown('KeyA') || this.input.isDown('ArrowLeft'),
                    right: this.input.isDown('KeyD') || this.input.isDown('ArrowRight'),
                    run: sprinting,
                },
                p.facing,
            );
            const solid = this.build.resolve(
                this.net.predictedX,
                this.net.predictedY,
                PLAYER.radius,
            );
            this.net.predictedX = solid.x;
            this.net.predictedY = solid.y;
            p.x = solid.x;
            p.y = solid.y;
        } else {
            const world = this.world.clampToWorld(p.x + p.vx * dt, p.y + p.vy * dt, PLAYER.radius);
            const solid = this.build.resolve(world.x, world.y, PLAYER.radius);
            p.x = solid.x;
            p.y = solid.y;
        }
        // Swimming has its own slower stroke, and it keeps ticking while you float.
        if (swimming) p.walkPhase += dt * (moving ? SWIM.strokeRate : SWIM.strokeRate * 0.45);
        else p.walkPhase += moving ? dt * (sprinting ? 14 : 9) : -p.walkPhase * dt * 8;

        const m = this.camera.screenToWorld(this.input.mouseX, this.input.mouseY);
        // Aim stays put while a menu is open, so the cursor can be used for the UI.
        if (!menuOpen) p.facing = Math.atan2(m.y - p.y, m.x - p.x);

        // Drawing a bow: hold the right button. It takes a full draw before an
        // arrow can go, and letting go, running, swimming or opening a menu
        // lets the string back down.
        const drawing =
            this.heldItem()?.id === 'bow' &&
            this.input.rightDown &&
            !menuOpen &&
            !this.uiHover &&
            !swimming &&
            !sprinting;
        p.bowDraw = drawing ? Math.min(BOW_DRAW_SECONDS, (p.bowDraw ?? 0) + dt) : 0;

        if (menuOpen || this.uiHover) return;
        if (swimming) {
            // Hands are busy staying afloat.
            if (this.input.mouseClicked && this.edgeStep)
                this.notify('You cannot use that while swimming.');
            return;
        }
        if (this.input.mouseDown && p.attackTimer <= 0) this.held.primaryUse(m.x, m.y);
    }

    // ------------------------------------------------------------------- use

    // -------------------------------------------------------------- building

    // ------------------------------------------------------------- interact

    /**
     * Whether the player may reach something. You cannot open a box through a
     * wall, and you cannot work on anything sealed inside a room you are not
     * standing in and have no cupboard authority over.
     */
    canReach(x: number, y: number): boolean {
        const p = this.player;
        if (this.build.blocksLine(p.x, p.y, x, y)) return false;
        const region = this.build.regionAt(x, y);
        if (region === 0) return true;
        if (this.build.regionAt(p.x, p.y) === region) return true;
        return this.build.regionOwner(region) === 0;
    }

    /** True when the interior of a sealed room should be hidden from the player. */
    regionHidden(region: number): boolean {
        if (region === 0) return false;
        if (this.build.regionAt(this.player.x, this.player.y) === region) return false;
        return this.build.regionOwner(region) !== 0;
    }

    /** The plant under the player's nose, if any. */
    private harvestableNear(): ResourceNode | null {
        let best: ResourceNode | null = null;
        let bestD: number = PLAYER.interact;
        for (const n of this.world.nodesNear(this.player.x, this.player.y, PLAYER.interact + 30)) {
            if (n.hp <= 0 || n.kind !== 'nettle') continue;
            const d = dist(this.player.x, this.player.y, n.x, n.y);
            if (d < bestD) {
                bestD = d;
                best = n;
            }
        }
        return best;
    }

    /**
     * Whether a lake is within arm's reach of this point.
     *
     * Only fresh water: the sea is salt. Probed at the interact range, like
     * everything else E reaches for, rather than the 70 it used to use.
     */
    private nearWater(x: number, y: number): boolean {
        for (let a = 0; a < 8; a++) {
            const ang = (a / 8) * TAU;
            const r = PLAYER.interact + PLAYER.radius;
            if (this.world.freshAt(x + Math.cos(ang) * r, y + Math.sin(ang) * r)) return true;
        }
        return false;
    }

    // ------------------------------------------------------------- workshops

    // ----------------------------------------------------------------- items

    // ----------------------------------------------------------------- damage

    // ----------------------------------------------------------------- clans
}
