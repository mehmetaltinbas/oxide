import { MELEE_KNOCKBACK } from 'src/features/combat/constants/melee-knockback.constant';
import { regrowthSeconds } from 'src/features/world/utils/regrowth-seconds.util';
import { BuildSystem } from 'src/features/building/building';
import { CELL } from 'src/features/building/constants/cell.constant';
import { TIER_DEFS } from 'src/features/building/constants/tier-defs.constant';
import { PendingPiece } from 'src/features/building/types/pending-piece.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { CLAN_MAX } from 'src/features/clans/constants/clan-max.constant';
import { CLAN_MIN_FROM_PLAYER } from 'src/features/clans/constants/clan-min-from-player.constant';
import { CLAN_MIN_SPACING } from 'src/features/clans/constants/clan-min-spacing.constant';
import { CLAN_MIN } from 'src/features/clans/constants/clan-min.constant';
import { CLAN_NAMES } from 'src/features/clans/constants/clan-names.constant';
import { CLAN_RESETTLE_DELAY } from 'src/features/clans/constants/clan-resettle-delay.constant';
import { CLAN_RESPAWN_DELAY } from 'src/features/clans/constants/clan-respawn-delay.constant';
import { CLAN_SKILLS } from 'src/features/clans/constants/clan-skills.constant';
import { CLAN_TECH_PERIOD } from 'src/features/clans/constants/clan-tech-period.constant';
import { RAID_BASE_COOLDOWN } from 'src/features/clans/constants/raid-base-cooldown.constant';
import { RAID_MIN_ELAPSED } from 'src/features/clans/constants/raid-min-elapsed.constant';
import { ClanHooks } from 'src/features/clans/types/clan-hooks.interface';
import { ClanState } from 'src/features/clans/types/clan-state.interface';
import { rollClanSkill } from 'src/features/clans/utils/roll-clan-skill.util';
import { Combat } from 'src/features/combat/combat';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { addItem } from 'src/features/items/utils/add-item.util';
import { countIn } from 'src/features/items/utils/count-in.util';
import { removeItem } from 'src/features/items/utils/remove-item.util';
import { NPC_ACTIVE_RADIUS } from 'src/features/npcs/constants/npc-active-radius.constant';
import { NPCS } from 'src/features/npcs/constants/npcs.constant';
import { SURVIVOR } from 'src/features/npcs/constants/survivor.constant';
import { WILDLIFE_SPAWN } from 'src/features/npcs/constants/wildlife-spawn.constant';
import { NpcSystem } from 'src/features/npcs/npcs';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';
import { Npc } from 'src/features/npcs/types/npc.interface';
import { SurvivorWorld } from 'src/features/npcs/types/survivor-world.interface';
import { NODES } from 'src/features/world/constants/nodes.constant';
import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';
import { LootCrate } from 'src/features/world/types/loot-crate.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';
import { World } from 'src/features/world/world';
import { TAU } from 'src/shared/constants/tau.constant';
import { Audio } from 'src/shared/core/audio';
import { Particles } from 'src/shared/core/particles';
import { clamp } from 'src/shared/utils/clamp.util';
import { dist } from 'src/shared/utils/dist.util';
import { randRange } from 'src/shared/utils/rand-range.util';

export class ClanSystem {
    clans: ClanState[] = [];
    /** The raid currently inbound, if any. */
    raid: { x: number; y: number; clan: number; timer: number } | null = null;

    private nextClanIndex = 0;
    clanTarget = CLAN_MIN;
    private resettleTimer = CLAN_RESETTLE_DELAY;
    private clanAccrual = 0;

    constructor(
        private world: World,
        private build: BuildSystem,
        private npcs: NpcSystem,
        private combat: Combat,
        private particles: Particles,
        private audio: Audio,
        private hooks: ClanHooks,
    ) {}

    /**
     * Everything the survivor brain is allowed to touch. Deliberately narrow: the
     * AI can only do things a player could do. See docs/systems/ai-survivor.md.
     */
    survivorWorld(): SurvivorWorld {
        return {
            clanOf: (id) => this.clanByOwner(id),
            clanStock: (id) => {
                const box = this.build.deployables.find(
                    (d) => d.kind === 'wooden_box' && d.owner === id,
                );
                return box?.container ?? null;
            },
            deployableOf: (id, kind) =>
                this.build.deployables.find((d) => d.owner === id && d.kind === kind) ?? null,
            nearestNode: (x, y, kinds, maxDist) => {
                let best: ResourceNode | null = null;
                let bestD = maxDist;
                for (const n of this.world.nodesNear(x, y, maxDist)) {
                    if (n.hp <= 0 || !kinds.includes(n.kind)) continue;
                    const d = dist(x, y, n.x, n.y);
                    if (d < bestD) {
                        bestD = d;
                        best = n;
                    }
                }
                return best;
            },
            nodeById: (id) => this.world.nodeById_(id) ?? null,
            npcById: (id) => this.npcs.list.find((n) => n.id === id) ?? null,
            nearestAnimal: (x, y, maxDist) => {
                let best: Npc | null = null;
                let bestD = maxDist;
                for (const n of this.npcs.list) {
                    if (n.clan > 0 || n.kind === 'scientist') continue;
                    const d = dist(x, y, n.x, n.y);
                    if (d < bestD) {
                        bestD = d;
                        best = n;
                    }
                }
                return best;
            },
            nearestCrate: (x, y, maxDist) => {
                let best: LootCrate | null = null;
                let bestD = maxDist;
                for (const c of this.world.crates) {
                    if (c.looted) continue;
                    const d = dist(x, y, c.x, c.y);
                    if (d < bestD) {
                        bestD = d;
                        best = c;
                    }
                }
                return best;
            },
            lootCrate: (crate) => {
                const taken: Partial<Record<ItemId, number>> = {};
                for (let i = 0; i < crate.container.slots.length; i++) {
                    const stack = crate.container.slots[i];
                    if (!stack) continue;
                    taken[stack.id] = (taken[stack.id] ?? 0) + stack.count;
                    crate.container.slots[i] = null;
                }
                crate.looted = true;
                crate.respawn = regrowthSeconds();
                return taken;
            },
            expansionSite: (clan) => {
                const owner = clan.index + 1;
                const stock = this.build.deployables.find(
                    (d) => d.kind === 'wooden_box' && d.owner === owner,
                )?.container;
                if (!stock) return null;
                // Extending costs a foundation plus a wall; only do it with a surplus.
                const cost: Partial<Record<ItemId, number>> = { wood: 120, stone: 80 };
                for (const [id, n] of Object.entries(cost) as [ItemId, number][]) {
                    if (countIn(stock, id) < n) return null;
                }
                // Look for a free cell touching what they already have.
                for (const s2 of this.build.structures) {
                    if (s2.owner !== owner || s2.kind !== 'foundation') continue;
                    for (const [dx, dy] of [
                        [1, 0],
                        [-1, 0],
                        [0, 1],
                        [0, -1],
                    ] as [number, number][]) {
                        const gx = s2.gx + dx;
                        const gy = s2.gy + dy;
                        if (this.build.foundationAt(gx, gy)) continue;
                        if (this.build.canPlaceFoundation(gx, gy, owner)) continue;
                        return { gx, gy, cost };
                    }
                }
                return null;
            },
            applyExpansion: (clan, gx, gy, cost) => {
                const owner = clan.index + 1;
                const stock = this.build.deployables.find(
                    (d) => d.kind === 'wooden_box' && d.owner === owner,
                )?.container;
                if (!stock) return;
                if (this.build.canPlaceFoundation(gx, gy, owner)) return;
                for (const [id, n] of Object.entries(cost) as [ItemId, number][])
                    removeItem(stock, id, n);
                const tier = CLAN_SKILLS[clan.skill].tier;
                this.build.placeFoundation(gx, gy, owner, tier);
                // Wall off whichever sides face open ground, so it stays a compound.
                for (const [dx, dy, side] of [
                    [0, -1, 'n'],
                    [0, 1, 'n'],
                    [-1, 0, 'w'],
                    [1, 0, 'w'],
                ] as [number, number, 'n' | 'w'][]) {
                    if (this.build.foundationAt(gx + dx, gy + dy)) continue;
                    const ex = dy > 0 || dx > 0 ? gx + Math.max(0, dx) : gx;
                    const ey = dy > 0 ? gy + 1 : gy;
                    if (this.build.edgeAt(ex, ey, side)) continue;
                    this.build.placeEdge(
                        ex,
                        ey,
                        side,
                        'wall',
                        owner,
                        { x: clan.x, y: clan.y },
                        tier,
                    );
                }
            },
            threatNear: (x, y, clanId, maxDist) => {
                if (
                    this.hooks.player().alive &&
                    dist(x, y, this.hooks.player().x, this.hooks.player().y) < maxDist
                ) {
                    return { x: this.hooks.player().x, y: this.hooks.player().y };
                }
                for (const n of this.npcs.list) {
                    if (n.clan === clanId || n.clan === 0) continue;
                    if (dist(x, y, n.x, n.y) < maxDist) return { x: n.x, y: n.y };
                }
                return null;
            },
            upgradeCandidate: (clan) => {
                const owner = clan.index + 1;
                const stock = this.build.deployables.find(
                    (d) => d.kind === 'wooden_box' && d.owner === owner,
                )?.container;
                if (!stock) return null;
                let weakest: Structure | null = null;
                for (const s2 of this.build.structures) {
                    if (s2.owner !== owner || !s2.side) continue;
                    const next = this.build.nextTier(s2);
                    if (!next) continue;
                    if (!weakest || s2.maxHp < weakest.maxHp) weakest = s2;
                }
                if (!weakest) return null;
                const next = this.build.nextTier(weakest);
                if (!next) return null;
                const cost = TIER_DEFS[next].cost;
                for (const [id, n] of Object.entries(cost) as [ItemId, number][]) {
                    if (countIn(stock, id) < n) return null;
                }
                return { piece: weakest, cost };
            },
            applyUpgrade: (clan, piece, cost) => {
                const owner = clan.index + 1;
                const stock = this.build.deployables.find(
                    (d) => d.kind === 'wooden_box' && d.owner === owner,
                )?.container;
                if (!stock) return;
                for (const [id, n] of Object.entries(cost) as [ItemId, number][])
                    removeItem(stock, id, n);
                const next = this.build.nextTier(piece);
                if (next) this.build.upgrade(piece, next);
            },
            damageNode: (node, amount) => {
                const def = NODES[node.kind];
                node.hp -= amount;
                node.shake = 0.16;
                this.particles.burst(node.x, node.y - 4, 4, def.color, {
                    speed: 110,
                    life: 0.35,
                    size: 2.2,
                    gravity: 220,
                });
                const out: Partial<Record<ItemId, number>> = {};
                for (const [id, per] of Object.entries(def.yield) as [ItemId, number][]) {
                    out[id] = Math.max(1, Math.round(per * 0.35));
                }
                if (node.hp <= 0) node.respawn = regrowthSeconds();
                return out;
            },
            attackAnimal: (attacker, target) => {
                const def = NPCS[attacker.kind];
                if (def.gun) {
                    if (attacker.gunTimer > 0) return;
                    attacker.gunTimer = def.gun.cooldown;
                    const a = Math.atan2(target.y - attacker.y, target.x - attacker.x);
                    this.combat.fire(
                        attacker.x + Math.cos(a) * 14,
                        attacker.y + Math.sin(a) * 14,
                        a,
                        def.gun.damage,
                        def.gun.speed,
                        def.gun.range,
                        'hostile',
                        '#ff9a6a',
                        attacker.clan,
                    );
                    return;
                }
                if (attacker.attackTimer > 0) return;
                attacker.attackTimer = def.attackCooldown;
                // Tagging the clan means the kill credits the hunter wherever it lands.
                this.npcs.damage(
                    target,
                    def.damage,
                    attacker.x,
                    attacker.y,
                    MELEE_KNOCKBACK,
                    attacker.clan,
                );
            },
            isNight: () => this.hooks.isNight(),
            step: (npc, x, y, dt, scale) => this.npcs.stepToward(npc, x, y, dt, scale ?? 1),
            say: (npc, text, color) => this.particles.text(npc.x, npc.y - 26, text, color),
        };
    }

    /** Rolls the starting population; upkeep happens every tick after that. */
    spawnClans(): void {
        this.clans = [];
        this.nextClanIndex = 0;
        this.clanTarget = CLAN_MIN + Math.floor(Math.random() * (CLAN_MAX - CLAN_MIN + 1));
        for (let i = 0; i < this.clanTarget; i++) {
            const clan = this.foundClan(false);
            if (!clan) break;
        }
    }

    /**
     * Move a new clan onto an empty plot. `staged` clans raise their compound
     * piece by piece in real time; the starting clans are placed complete.
     */
    private foundClan(staged: boolean): ClanState | null {
        const spot = this.findClanSpot();
        if (!spot) return null;

        const skill = rollClanSkill(Math.random);
        const def = CLAN_SKILLS[skill];
        const index = this.nextClanIndex++;
        const nameDef = CLAN_NAMES[index % CLAN_NAMES.length];
        const owner = index + 1;

        const clan: ClanState = {
            index,
            name: nameDef.name,
            tag: nameDef.tag,
            color: nameDef.color,
            skill,
            x: spot.x,
            y: spot.y,
            tech: 1,
            raidCooldown: (RAID_BASE_COOLDOWN / def.raidPace) * randRange(Math.random, 0.7, 1.4),
            wiped: false,
            tcId: null,
            founding: staged,
            buildQueue: [],
            buildTimer: 0,
            respawnTimer: CLAN_RESPAWN_DELAY,
        };
        this.clans.push(clan);

        // Clear the plot before anything goes up. A clan settling on a patch of
        // forest fells it first, the same as you would, which is why their
        // compounds no longer have trees standing in the middle of a room.
        const gxPlot = Math.floor(spot.x / CELL);
        const gyPlot = Math.floor(spot.y / CELL);
        this.world.clearNaturalIn(
            gxPlot * CELL - 8,
            gyPlot * CELL - 8,
            (gxPlot + def.baseSize) * CELL + 8,
            (gyPlot + def.baseSize) * CELL + 8,
        );
        this.npcs.invalidateNavigation();

        // The cupboard always goes down first: it claims the plot, and losing it is
        // what ends the clan later on.
        const gx0 = Math.floor(spot.x / CELL);
        const gy0 = Math.floor(spot.y / CELL);
        const tc = this.build.deploy('tool_cupboard', gx0 + 1, gy0 + 1, owner, 400);
        clan.tcId = tc.id;

        const queue = this.compoundPlan(spot.x, spot.y, def.baseSize);
        if (staged) {
            clan.buildQueue = queue;
            clan.buildTimer = def.buildStep;
            this.hooks.notify(`${clan.name} (${def.label}) is settling in.`);
        } else {
            for (const piece of queue) this.raisePiece(clan, piece);
        }

        this.spawnClanMembers(
            clan,
            staged ? Math.max(1, Math.round(def.members / 2)) : def.members,
        );
        return clan;
    }

    /**
     * Best-candidate sampling: gather several legal spots and take the one
     * furthest from everything already placed. Plain rejection sampling clusters
     * clans in whichever corner it happens to hit first and leaves the rest of
     * the island empty.
     */
    private findClanSpot(): { x: number; y: number } | null {
        const candidates: { x: number; y: number }[] = [];
        for (let tries = 0; tries < 600 && candidates.length < 14; tries++) {
            const spot = this.sampleClanSpot();
            if (spot) candidates.push(spot);
        }
        if (candidates.length === 0) return null;

        let best = candidates[0];
        let bestScore = -Infinity;
        for (const c of candidates) {
            let nearest = Infinity;
            for (const other of this.clans) {
                if (other.wiped) continue;
                nearest = Math.min(nearest, dist(c.x, c.y, other.x, other.y));
            }
            if (this.hooks.player)
                nearest = Math.min(
                    nearest,
                    dist(c.x, c.y, this.hooks.player().x, this.hooks.player().y),
                );
            if (nearest === Infinity) nearest = 0;
            // Prefer distance from neighbours, and mildly prefer inland ground.
            const edge = Math.min(c.x, WORLD_W - c.x, c.y, WORLD_H - c.y);
            const score = nearest + Math.min(edge, 2500) * 0.25;
            if (score > bestScore) {
                bestScore = score;
                best = c;
            }
        }
        return best;
    }

    private sampleClanSpot(): { x: number; y: number } | null {
        for (let tries = 0; tries < WILDLIFE_SPAWN.tries; tries++) {
            const x = randRange(Math.random, 700, WORLD_W - 700);
            const y = randRange(Math.random, 700, WORLD_H - 700);
            if (this.world.biomeAt(x, y) === 'water') continue;
            if (this.world.monumentAt(x, y)) continue;
            if (
                this.hooks.player &&
                dist(x, y, this.hooks.player().x, this.hooks.player().y) < CLAN_MIN_FROM_PLAYER
            )
                continue;
            const base = this.playerBaseCenter();
            if (base && dist(x, y, base.x, base.y) < CLAN_MIN_FROM_PLAYER) continue;
            let clash = false;
            for (const c of this.clans) {
                if (!c.wiped && dist(x, y, c.x, c.y) < CLAN_MIN_SPACING) clash = true;
            }
            if (clash) continue;
            // Do not drop a compound on top of anything already standing.
            const gx0 = Math.floor(x / CELL);
            const gy0 = Math.floor(y / CELL);
            let occupied = false;
            for (let dy = -1; dy <= 5; dy++) {
                for (let dx = -1; dx <= 5; dx++) {
                    if (this.build.foundationAt(gx0 + dx, gy0 + dy)) occupied = true;
                }
            }
            if (occupied) continue;
            // Compounds need dry ground all the way across their footprint.
            let wet = false;
            for (let dy = 0; dy <= 4 && !wet; dy++) {
                for (let dx = 0; dx <= 4; dx++) {
                    if (this.world.biomeAt(x + dx * CELL, y + dy * CELL) === 'water') {
                        wet = true;
                        break;
                    }
                }
            }
            if (wet) continue;
            return { x, y };
        }
        return null;
    }

    /** The full build order for a compound: floors, then a wall ring with a gap. */
    private compoundPlan(x: number, y: number, size: number): PendingPiece[] {
        const gx0 = Math.floor(x / CELL);
        const gy0 = Math.floor(y / CELL);
        const out: PendingPiece[] = [];
        for (let dy = 0; dy < size; dy++) {
            for (let dx = 0; dx < size; dx++)
                out.push({ kind: 'foundation', gx: gx0 + dx, gy: gy0 + dy });
        }
        for (let dx = 0; dx < size; dx++) {
            out.push({ kind: 'wall', gx: gx0 + dx, gy: gy0, side: 'n' });
            const door = dx === Math.floor(size / 2);
            out.push({ kind: door ? 'doorway' : 'wall', gx: gx0 + dx, gy: gy0 + size, side: 'n' });
        }
        for (let dy = 0; dy < size; dy++) {
            out.push({ kind: 'wall', gx: gx0, gy: gy0 + dy, side: 'w' });
            out.push({ kind: 'wall', gx: gx0 + size, gy: gy0 + dy, side: 'w' });
        }
        // A survivor camp needs somewhere to bank, something to smelt with, and a
        // fire to cook on, the AI uses all three.
        // Distinct corners: on a 2x2 amateur compound the cupboard already owns
        // (size-1, size-1), so nothing else may claim it.
        out.push({ kind: 'deploy', gx: gx0, gy: gy0, deploy: 'wooden_box' });
        out.push({ kind: 'deploy', gx: gx0 + size - 1, gy: gy0, deploy: 'campfire' });
        out.push({ kind: 'deploy', gx: gx0, gy: gy0 + size - 1, deploy: 'furnace' });
        // Their respawn point. Killing a clansman is not permanent while this stands.
        out.push({
            kind: 'deploy',
            gx: gx0 + size - 1,
            gy: gy0 + size - 1,
            deploy: 'sleeping_bag',
        });
        return out;
    }

    /**
     * Fell whatever is standing on a cell. Clans do this before they build on
     * it, which is the same thing a player does by hand with an axe.
     */
    private clearCell(gx: number, gy: number): void {
        const n = this.world.clearNaturalIn(
            gx * CELL - 6,
            gy * CELL - 6,
            (gx + 1) * CELL + 6,
            (gy + 1) * CELL + 6,
        );
        if (n > 0) this.npcs.invalidateNavigation();
    }

    /** Put one queued piece up. Used both for instant and staged construction. */
    private raisePiece(clan: ClanState, piece: PendingPiece): void {
        const def = CLAN_SKILLS[clan.skill];
        const owner = clan.index + 1;
        if (piece.kind === 'foundation') {
            if (!this.build.foundationAt(piece.gx, piece.gy)) {
                this.clearCell(piece.gx, piece.gy);
                this.build.placeFoundation(piece.gx, piece.gy, owner, def.tier);
            }
            return;
        }
        if (piece.kind === 'deploy' && piece.deploy) {
            // canDeploy returns a reason string when blocked and null when free.
            this.clearCell(piece.gx, piece.gy);
            if (this.build.canDeploy(piece.gx, piece.gy, owner) === null) {
                const d = this.build.deploy(
                    piece.deploy,
                    piece.gx,
                    piece.gy,
                    owner,
                    piece.deploy === 'wooden_box' ? 200 : 300,
                );
                if (d.kind === 'wooden_box' && d.container)
                    this.stockClanBox(d.container, def.wealth);
            }
            return;
        }
        if (!piece.side) return;
        if (this.build.edgeAt(piece.gx, piece.gy, piece.side)) return;
        const edge = this.build.placeEdge(
            piece.gx,
            piece.gy,
            piece.side,
            piece.kind === 'doorway' ? 'doorway' : 'wall',
            owner,
            { x: clan.x, y: clan.y },
            def.tier,
        );
        // Nobody leaves an open hole in their own wall. A clan doorway always gets
        // a door, and it is always locked to anyone but them.
        if (piece.kind === 'doorway') {
            this.build.installDoor(edge, owner);
            edge.locked = true;
            edge.open = false;
        }
    }

    private stockClanBox(container: Container, wealth: number): void {
        const roll = (lo: number, hi: number): number =>
            Math.round(randRange(Math.random, lo, hi) * wealth);
        // A camp that has been lived in has food in it.
        addItem(container, 'meat_cooked', Math.round(randRange(Math.random, 5, 10)));
        addItem(container, 'meat_raw', Math.round(randRange(Math.random, 3, 7)));
        addItem(container, 'wood', Math.round(randRange(Math.random, 150, 320)));
        addItem(container, 'metal', roll(120, 400));
        addItem(container, 'scrap', roll(40, 140));
        addItem(container, 'gunpowder', roll(20, 90));
        addItem(container, 'cloth', roll(30, 90));
        if (wealth >= 1.8) addItem(container, 'sulfur', roll(40, 120));
        if (wealth >= 3) addItem(container, 'c4', 1);
    }

    private spawnClanMembers(clan: ClanState, count: number): void {
        const owner = clan.index + 1;
        for (let i = 0; i < count; i++) {
            const a = Math.random() * TAU;
            const r = randRange(Math.random, 90, 170);
            // Bigger, better clans field a higher share of fighters.
            const fighterShare =
                clan.skill === 'amateur' ? 0.34 : clan.skill === 'expert' ? 0.7 : 0.5;
            const kind: NpcKind = Math.random() < fighterShare ? 'raider' : 'gatherer';
            this.npcs.spawn(kind, clan.x + Math.cos(a) * r, clan.y + Math.sin(a) * r, {
                clan: owner,
                home: { x: clan.x, y: clan.y },
                leash: 460,
                tech: clan.tech,
            });
        }
    }

    clanByOwner(owner: number): ClanState | undefined {
        return this.clans.find((c) => c.index + 1 === owner);
    }

    updateClans(dt: number): void {
        for (const clan of this.clans) {
            if (clan.wiped) continue;
            const def = CLAN_SKILLS[clan.skill];

            // A cupboard going down is what actually ends a clan, as in Rust.
            if (clan.tcId !== null && !this.build.byId(clan.tcId)) {
                this.wipeClan(clan);
                continue;
            }

            // Founding clans put their compound up piece by piece while you watch.
            if (clan.founding) {
                clan.buildTimer -= dt;
                if (clan.buildTimer <= 0) {
                    const piece = clan.buildQueue.shift();
                    if (piece) {
                        this.raisePiece(clan, piece);
                        clan.buildTimer = def.buildStep;
                        this.particles.burst(
                            piece.gx * CELL + CELL / 2,
                            piece.gy * CELL + CELL / 2,
                            6,
                            '#c9a227',
                            {
                                speed: 90,
                                life: 0.5,
                                size: 2.6,
                            },
                        );
                    } else {
                        clan.founding = false;
                        // The rest of the crew moves in once the walls are up.
                        this.spawnClanMembers(clan, Math.max(1, Math.round(def.members / 2)));
                        this.hooks.notify(`${clan.name} has finished their compound.`);
                    }
                }
                continue;
            }

            clan.tech = 1 + Math.floor(this.hooks.clock() / CLAN_TECH_PERIOD) * def.techPerDay;
            clan.raidCooldown -= dt;

            this.tickClanRespawn(clan, dt, def.members);

            // Clans nobody is watching still make progress, coarsely: their box fills
            // and walls occasionally go up, so leaving one alone has consequences.
            if (
                dist(clan.x, clan.y, this.hooks.player().x, this.hooks.player().y) >
                NPC_ACTIVE_RADIUS
            ) {
                this.offscreenClanTick(clan, dt, def.techPerDay);
            }

            // A clan only bothers with you once you have something worth taking.
            if (this.raid || clan.raidCooldown > 0 || this.hooks.clock() < RAID_MIN_ELAPSED)
                continue;
            const base = this.playerBaseCenter();
            if (!base) continue;
            clan.raidCooldown =
                (RAID_BASE_COOLDOWN / def.raidPace) * randRange(Math.random, 0.9, 1.6);
            this.launchRaid(clan, base);
        }

        this.maintainClanPopulation(dt);

        if (this.raid) {
            this.raid.timer -= dt;
            // Count every raider from that clan, not only ones already in the raid
            // state: on the frame a raid launches they have not been ticked yet, and
            // filtering by state would cancel the raid the instant it began.
            const alive = this.npcs.list.filter(
                (n) => n.kind === 'raider' && n.clan === this.raid!.clan + 1,
            ).length;
            if (this.raid.timer <= 0 || alive === 0) {
                if (alive === 0 && this.raid.timer > 0) this.hooks.notify('Raid beaten off.');
                this.raid = null;
            }
        }
    }

    /**
     * Killing a clansman is not permanent while their bed stands. They come back
     * on it, the same way you do: which is also why the bed is worth breaking.
     */
    private tickClanRespawn(clan: ClanState, dt: number, target: number): void {
        const owner = clan.index + 1;
        const alive = this.npcs.list.filter((n) => n.clan === owner).length;
        if (alive >= target) {
            clan.respawnTimer = CLAN_RESPAWN_DELAY;
            return;
        }
        const bag = this.build.deployables.find(
            (d) => d.kind === 'sleeping_bag' && d.owner === owner,
        );
        if (!bag) return;

        clan.respawnTimer -= dt;
        if (clan.respawnTimer > 0) return;
        clan.respawnTimer = CLAN_RESPAWN_DELAY;

        const fighterShare = clan.skill === 'amateur' ? 0.34 : clan.skill === 'expert' ? 0.7 : 0.5;
        const kind: NpcKind = Math.random() < fighterShare ? 'raider' : 'gatherer';
        this.npcs.spawn(
            kind,
            bag.x + randRange(Math.random, -22, 22),
            bag.y + randRange(Math.random, -22, 22),
            {
                clan: owner,
                home: { x: clan.x, y: clan.y },
                leash: 460,
                tech: clan.tech,
            },
        );
        this.particles.burst(bag.x, bag.y, 12, clan.color, { speed: 90, life: 0.6, size: 3 });
    }

    /** Cheap stand-in for a clan's day-to-day work while it is out of simulation. */
    private offscreenClanTick(clan: ClanState, dt: number, rate: number): void {
        const owner = clan.index + 1;
        const box = this.build.deployables.find(
            (d) => d.kind === 'wooden_box' && d.owner === owner,
        );
        if (!box?.container) return;
        const gain = SURVIVOR.offscreenGatherRate * rate * dt;
        this.clanAccrual += gain;
        if (this.clanAccrual >= 1) {
            const whole = Math.floor(this.clanAccrual);
            this.clanAccrual -= whole;
            addItem(box.container, 'wood', whole * 3);
            addItem(box.container, 'stone', whole * 2);
            if (Math.random() < 0.3) addItem(box.container, 'metal_ore', whole);
        }
        if (Math.random() < SURVIVOR.offscreenUpgradeChance * rate) {
            const world = this.survivorWorld();
            const candidate = world.upgradeCandidate(clan);
            if (candidate) world.applyUpgrade(clan, candidate.piece, candidate.cost);
        }
    }

    /** A cleared plot does not stay cleared: somebody new always moves in. */
    private maintainClanPopulation(dt: number): void {
        // Drop wiped clans out of the roster once their people are gone too.
        for (let i = this.clans.length - 1; i >= 0; i--) {
            const c = this.clans[i];
            if (!c.wiped) continue;
            const stragglers = this.npcs.list.some((n) => n.clan === c.index + 1);
            if (!stragglers) this.clans.splice(i, 1);
        }

        const active = this.clans.filter((c) => !c.wiped).length;
        const settling = this.clans.some((c) => c.founding);

        // Re-roll the island's population every so often inside the allowed range.
        if (active >= this.clanTarget && !settling) {
            this.resettleTimer -= dt;
            if (this.resettleTimer <= 0) {
                this.resettleTimer = CLAN_RESETTLE_DELAY * 4;
                this.clanTarget = CLAN_MIN + Math.floor(Math.random() * (CLAN_MAX - CLAN_MIN + 1));
            }
            return;
        }
        if (settling) return;

        this.resettleTimer -= dt;
        if (this.resettleTimer > 0) return;
        this.resettleTimer = CLAN_RESETTLE_DELAY;
        this.foundClan(true);
    }

    private wipeClan(clan: ClanState): void {
        clan.wiped = true;
        clan.founding = false;
        clan.buildQueue.length = 0;
        this.hooks.notify(`${clan.name} has been wiped out.`);
        this.audio.waveClear();
        // Whoever is left scatters rather than defending a plot they no longer hold.
        for (const n of this.npcs.list) {
            if (n.clan !== clan.index + 1) continue;
            n.state = 'flee';
            n.leash = 99999;
            n.homeX = randRange(Math.random, 200, WORLD_W - 200);
            n.homeY = randRange(Math.random, 200, WORLD_H - 200);
        }
        // A cleared plot frees up quickly so the island keeps turning over.
        this.resettleTimer = Math.min(this.resettleTimer, CLAN_RESETTLE_DELAY);
    }

    /** Where the player's stuff is, if they have built anything worth raiding. */
    playerBaseCenter(): { x: number; y: number } | null {
        const tc = this.build.deployables.find((d) => d.kind === 'tool_cupboard' && d.owner === 0);
        if (tc) return { x: tc.x, y: tc.y };
        const box = this.build.deployables.find((d) => d.kind === 'wooden_box' && d.owner === 0);
        if (box) return { x: box.x, y: box.y };
        return null;
    }

    private launchRaid(clan: ClanState, base: { x: number; y: number }): void {
        const def = CLAN_SKILLS[clan.skill];
        const count = clamp(1 + Math.floor((clan.tech * def.raidPace) / 2), 1, 8);
        const a = Math.random() * TAU;
        for (let i = 0; i < count; i++) {
            const spawnX = base.x + Math.cos(a) * (520 + i * 30);
            const spawnY = base.y + Math.sin(a) * (520 + i * 30);
            this.npcs.spawn(
                'raider',
                clamp(spawnX, 60, WORLD_W - 60),
                clamp(spawnY, 60, WORLD_H - 60),
                {
                    clan: clan.index + 1,
                    tech: clan.tech,
                    home: { x: clan.x, y: clan.y },
                    leash: 4000,
                    charges: Math.min(6, Math.round(clan.tech * def.raidPace)),
                },
            );
        }
        this.raid = { clan: clan.index, x: base.x, y: base.y, timer: 240 };
        this.audio.waveStart();
        this.hooks.notify(
            `${clan.name} (${CLAN_SKILLS[clan.skill].label}) is raiding you. ${count} of them.`,
        );
    }
}
