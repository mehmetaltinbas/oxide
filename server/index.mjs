/**
 * Oxide authoritative game server.
 *
 * Rooms hold a world seed and the authoritative state for players and their
 * building. Clients send input, the server simulates at a fixed tick and
 * broadcasts state; clients predict locally and reconcile. See
 * docs/NETWORKING.md — that document is the contract this implements.
 */
import { WebSocketServer } from 'ws';
import { randomUUID } from 'node:crypto';
import BUILD_NUMBERS from '../shared/building.json' with { type: 'json' };
import PLAYER_NUMBERS from '../shared/player.json' with { type: 'json' };
import WORLD_NUMBERS from '../shared/world.json' with { type: 'json' };

const PORT = Number(process.env.PORT ?? 8787);
const PROTOCOL_VERSION = 2;
const TICK_HZ = 30;
const TICK_MS = 1000 / TICK_HZ;

// Movement and building numbers come from shared/, imported by both sides.
// They used to be typed out again here, and a number that has to be matched up
// in two places eventually is not: the server's wall thickness and the client's
// were the same only by luck.
const PLAYER_SPEED = PLAYER_NUMBERS.speed;
const PLAYER_SPRINT = PLAYER_NUMBERS.sprint;
const PLAYER_RADIUS = PLAYER_NUMBERS.radius;
const WORLD_W = WORLD_NUMBERS.worldW;
const WORLD_H = WORLD_NUMBERS.worldH;
const CELL = WORLD_NUMBERS.cell;
const WALL_THICKNESS = BUILD_NUMBERS.wallThickness;
const DEPLOY_HALF = BUILD_NUMBERS.deployHalf;
const TC_RADIUS = BUILD_NUMBERS.toolCupboardRadius;
const GRID_W = Math.floor(WORLD_W / CELL);
const GRID_H = Math.floor(WORLD_H / CELL);
const TWIG_HP = BUILD_NUMBERS.twigHp;
const DEPLOYABLE_HP = BUILD_NUMBERS.deployableHp;
/**
 * Most damage one reported hit may do. The strongest thing in the game does
 * far less than a twig wall's whole health, so this only bites on a message
 * nobody's weapon could have sent.
 */
const MAX_DAMAGE_PER_HIT = 500;

const MAX_PLAYERS = 24;
const IDLE_ROOM_SECONDS = 300;

/** @type {Map<string, Room>} */
const rooms = new Map();

class Room {
    constructor(name, pvp) {
        this.id = randomUUID().slice(0, 8);
        this.name = name;
        this.pvp = pvp;
        this.seed = Math.floor(Math.random() * 1e9);
        this.createdAt = Date.now();
        this.tick = 0;
        /** @type {Map<string, any>} */
        this.players = new Map();
        /** @type {Map<number, any>} */
        this.structures = new Map();
        /** @type {Map<number, any>} */
        this.deployables = new Map();
        // Lookups by position, so validating a placement is not a scan of every
        // piece in the room. Kept in step with the two maps above by
        // `indexPiece` and `unindexPiece`, which are the only writers.
        /** @type {Map<string, any>} */
        this.foundationCells = new Map();
        /** @type {Map<string, any>} */
        this.edgeCells = new Map();
        /** @type {Map<string, any>} */
        this.deployCells = new Map();
        this.nextBuildId = 1;
        this.joinedThisTick = [];
        this.leftThisTick = [];
        this.emptySince = Date.now();
    }

    summary() {
        return {
            id: this.id,
            name: this.name,
            players: this.players.size,
            maxPlayers: MAX_PLAYERS,
            seed: this.seed,
            age: Math.round((Date.now() - this.createdAt) / 1000),
            pvp: this.pvp,
        };
    }

    add(client) {
        const player = {
            id: client.id,
            name: client.name,
            x: WORLD_W / 2 + (Math.random() * 400 - 200),
            y: WORLD_H / 2 + (Math.random() * 400 - 200),
            facing: 0,
            health: 100,
            held: null,
            alive: true,
            ack: 0,
            // 0 is nobody. Teaming up puts two players on the same id; it only
            // decides who can see whom, never who can hurt whom.
            team: 0,
            input: { up: false, down: false, left: false, right: false, run: false },
        };
        this.players.set(client.id, player);
        this.joinedThisTick.push(netPlayer(player));
        this.emptySince = 0;
        return player;
    }

    remove(id) {
        if (!this.players.delete(id)) return;
        this.leftThisTick.push(id);
        if (this.players.size === 0) this.emptySince = Date.now();
    }

    /** One authoritative step. Input has already been applied to each player. */
    step(dt) {
        this.tick++;
        for (const p of this.players.values()) {
            if (!p.alive) continue;
            let dx = (p.input.right ? 1 : 0) - (p.input.left ? 1 : 0);
            let dy = (p.input.down ? 1 : 0) - (p.input.up ? 1 : 0);
            const len = Math.hypot(dx, dy);
            if (len > 0) {
                dx /= len;
                dy /= len;
                const speed = PLAYER_SPEED * (p.input.run ? PLAYER_SPRINT : 1);
                p.x += dx * speed * dt;
                p.y += dy * speed * dt;
                // The server owns the final say on where you actually are.
                const solid = resolveAgainstBuilding(this, p.x, p.y, PLAYER_RADIUS);
                p.x = clamp(solid.x, PLAYER_RADIUS, WORLD_W - PLAYER_RADIUS);
                p.y = clamp(solid.y, PLAYER_RADIUS, WORLD_H - PLAYER_RADIUS);
            }
        }
    }
}

/**
 * How close you must be to ask someone to team up, in world units. Kept in
 * step with TEAM_INVITE_RANGE on the client by hand, like the movement
 * numbers: if they drift, the client offers an invitation the server refuses.
 */
const INVITE_RANGE = 120;

/** Team ids climb, so a new team never inherits an old one's members. */
let nextTeam = 1;

/** The socket a player id is on, or null if they have gone. */
function wsById(id) {
    for (const [ws, c] of clients) if (c.id === id) return ws;
    return null;
}

function clamp(v, lo, hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

/** What everybody is told when a piece goes up. */
function netPieceMessage(piece) {
    return piece.x === undefined
        ? { t: 'built', structure: piece }
        : { t: 'deployed', deployable: piece };
}

function netPlayer(p) {
    return {
        id: p.id,
        name: p.name,
        x: Math.round(p.x * 100) / 100,
        y: Math.round(p.y * 100) / 100,
        facing: Math.round(p.facing * 1000) / 1000,
        health: p.health,
        held: p.held,
        alive: p.alive,
        ack: p.ack,
        team: p.team ?? 0,
    };
}

// ----------------------------------------------------------------- building
//
// The authoritative build state. This mirrors the rules in
// src/features/building/building.ts, because the client runs them too and the
// two answers have to agree; where it cannot, it says so.
//
// What the server checks: the map bounds, whether the spot is already taken,
// that a wall has one of its own foundations beside it, that a door goes in
// your own doorway, and tool cupboard privilege.
//
// What it does not check yet: trees and rocks in the way, and monument ground.
// Both need the island itself, and worldgen lives only on the client for now.
// They stay client-side refusals, so a modified client could build on a rock.
// See docs/systems/networking.md.

const cellKey = (gx, gy) => `${gx},${gy}`;
const edgeKey = (gx, gy, side) => `${gx},${gy},${side}`;

/** The two cells an edge separates. */
function edgeCells(gx, gy, side) {
    return side === 'n'
        ? [
              [gx, gy - 1],
              [gx, gy],
          ]
        : [
              [gx - 1, gy],
              [gx, gy],
          ];
}

function edgeCenter(gx, gy, side) {
    return side === 'n'
        ? { x: gx * CELL + CELL / 2, y: gy * CELL }
        : { x: gx * CELL, y: gy * CELL + CELL / 2 };
}

function indexPiece(room, piece) {
    if (piece.kind === 'foundation') room.foundationCells.set(cellKey(piece.gx, piece.gy), piece);
    else if (piece.side) room.edgeCells.set(edgeKey(piece.gx, piece.gy, piece.side), piece);
    else room.deployCells.set(cellKey(piece.gx, piece.gy), piece);
}

function unindexPiece(room, piece) {
    if (piece.kind === 'foundation') room.foundationCells.delete(cellKey(piece.gx, piece.gy));
    else if (piece.side) room.edgeCells.delete(edgeKey(piece.gx, piece.gy, piece.side));
    else room.deployCells.delete(cellKey(piece.gx, piece.gy));
}

/** Whose tool cupboard covers this point, or null when nobody's does. */
function privilegeAt(room, x, y) {
    let best = null;
    let bestD = TC_RADIUS;
    for (const d of room.deployables.values()) {
        if (d.kind !== 'tool_cupboard') continue;
        const dd = Math.hypot(x - d.x, y - d.y);
        if (dd < bestD) {
            bestD = dd;
            best = d;
        }
    }
    return best ? best.owner : null;
}

function canBuildAt(room, x, y, owner) {
    const claim = privilegeAt(room, x, y);
    return claim === null || claim === owner;
}

const inBounds = (gx, gy) => gx >= 1 && gy >= 1 && gx < GRID_W - 1 && gy < GRID_H - 1;

/**
 * Why this placement is refused, or null when it stands. Returning the reason
 * rather than a boolean is what lets the client be told why its own optimistic
 * placement was taken back off it.
 */
function refuseBuild(room, owner, kind, gx, gy, side) {
    if (!inBounds(gx, gy)) return 'Outside the map';

    if (kind === 'foundation') {
        if (room.foundationCells.has(cellKey(gx, gy))) return 'Already a foundation here';
        if (!canBuildAt(room, gx * CELL + CELL / 2, gy * CELL + CELL / 2, owner)) {
            return 'Building blocked by a tool cupboard';
        }
        return null;
    }

    if (kind === 'wall' || kind === 'doorway' || kind === 'door') {
        if (side !== 'n' && side !== 'w') return 'Walls go on a cell edge';
        const existing = room.edgeCells.get(edgeKey(gx, gy, side));
        if (kind === 'door') {
            if (!existing) return 'Doors go in a doorway';
            if (existing.kind !== 'doorway') return 'That is not a doorway';
            if (existing.owner !== owner) return 'Not your doorway';
            return null;
        }
        if (existing) return 'Already something on this edge';
        // A wall needs something of yours to stand on.
        const [[ax, ay], [bx, by]] = edgeCells(gx, gy, side);
        const a = room.foundationCells.get(cellKey(ax, ay));
        const b = room.foundationCells.get(cellKey(bx, by));
        if (!((a && a.owner === owner) || (b && b.owner === owner))) {
            return 'Needs a foundation beside it';
        }
        const c = edgeCenter(gx, gy, side);
        if (!canBuildAt(room, c.x, c.y, owner)) return 'Building blocked by a tool cupboard';
        return null;
    }

    // Anything else is a deployable: a box, a furnace, a cupboard.
    if (room.deployCells.has(cellKey(gx, gy))) return 'Something is already here';
    if (!canBuildAt(room, gx * CELL + CELL / 2, gy * CELL + CELL / 2, owner)) {
        return 'Blocked by a tool cupboard';
    }
    return null;
}

const EDGE_KINDS = new Set(['wall', 'doorway', 'door']);

/** Put a validated piece into the room, and hand back what to tell everybody. */
function addPiece(room, owner, kind, gx, gy, side) {
    // A door is not a new piece: it is a doorway that grew a door.
    if (kind === 'door') {
        const doorway = room.edgeCells.get(edgeKey(gx, gy, side));
        doorway.kind = 'door';
        doorway.open = false;
        return doorway;
    }

    const piece = {
        id: room.nextBuildId++,
        kind,
        tier: 'twig',
        gx,
        gy,
        hp: EDGE_KINDS.has(kind) || kind === 'foundation' ? TWIG_HP : DEPLOYABLE_HP,
        maxHp: EDGE_KINDS.has(kind) || kind === 'foundation' ? TWIG_HP : DEPLOYABLE_HP,
        owner,
        open: false,
    };
    if (EDGE_KINDS.has(kind)) piece.side = side;

    if (EDGE_KINDS.has(kind) || kind === 'foundation') {
        room.structures.set(piece.id, piece);
    } else {
        // Deployables sit at the middle of their cell, and the wire carries the
        // point rather than the cell, because that is what collision needs.
        piece.x = gx * CELL + CELL / 2;
        piece.y = gy * CELL + CELL / 2;
        room.deployables.set(piece.id, piece);
    }
    indexPiece(room, piece);
    return piece;
}

function removePiece(room, piece) {
    unindexPiece(room, piece);
    room.structures.delete(piece.id);
    room.deployables.delete(piece.id);
}

/** Push a circle out of any solid wall in the room. Mirrors the client rule. */
function resolveAgainstBuilding(room, x, y, radius) {
    for (const s of room.structures.values()) {
        if (s.kind === 'foundation' || s.kind === 'doorway') continue;
        if (s.kind === 'door' && s.open) continue;
        const ax = s.gx * CELL;
        const ay = s.gy * CELL;
        const bx = s.side === 'n' ? ax + CELL : ax;
        const by = s.side === 'n' ? ay : ay + CELL;
        const hit = closestOnSegment(x, y, ax, ay, bx, by);
        const d = Math.hypot(x - hit.x, y - hit.y);
        const min = radius + WALL_THICKNESS;
        if (d < min && d > 0.0001) {
            const push = (min - d) / d;
            x += (x - hit.x) * push;
            y += (y - hit.y) * push;
        }
    }
    // Boxes, furnaces and cupboards are solid too. The client has always pushed
    // players out of these; the server only knew about walls, so online you
    // could walk through every deployable on the island.
    for (const dep of room.deployables.values()) {
        if (dep.kind === 'sleeping_bag') continue;
        const nx = clamp(x, dep.x - DEPLOY_HALF, dep.x + DEPLOY_HALF);
        const ny = clamp(y, dep.y - DEPLOY_HALF, dep.y + DEPLOY_HALF);
        const d = Math.hypot(x - nx, y - ny);
        if (d < radius && d > 0.0001) {
            const push = (radius - d) / d;
            x += (x - nx) * push;
            y += (y - ny) * push;
        }
    }
    return { x, y };
}

function closestOnSegment(px, py, ax, ay, bx, by) {
    const dx = bx - ax;
    const dy = by - ay;
    const len2 = dx * dx + dy * dy;
    let t = len2 > 0 ? ((px - ax) * dx + (py - ay) * dy) / len2 : 0;
    t = clamp(t, 0, 1);
    return { x: ax + dx * t, y: ay + dy * t };
}

// ------------------------------------------------------------------- server

/**
 * An HTTP server in front of the socket, for two reasons that only matter once
 * this runs somewhere real.
 *
 * A reverse proxy needs something to health-check, and "did the port open" is
 * not the same question as "is the game server alive". And a browser on an
 * https page refuses a plain ws:// socket, so in production this sits behind a
 * proxy that terminates TLS and forwards here; the proxy needs a plain HTTP
 * upgrade to forward to.
 */
const http = await import('node:http');
const httpServer = http.createServer((req, res) => {
    if (req.url === '/health') {
        res.writeHead(200, { 'content-type': 'application/json' });
        res.end(JSON.stringify({ ok: true, rooms: rooms.size, uptime: process.uptime() }));
        return;
    }
    res.writeHead(404);
    res.end();
});

/**
 * Which pages may open a socket here, as a comma-separated list of origins.
 *
 * Unset means anybody, which is what local development wants. Set it in
 * production to your own site: without it, a public game server is one anyone
 * can point their own client at.
 */
const ALLOWED_ORIGINS = (process.env.ALLOWED_ORIGINS ?? '')
    .split(',')
    .map((o) => o.trim())
    .filter(Boolean);

const wss = new WebSocketServer({
    server: httpServer,
    verifyClient: ({ origin }) => {
        if (ALLOWED_ORIGINS.length === 0) return true;
        return ALLOWED_ORIGINS.includes(origin);
    },
});
/** @type {Map<import('ws').WebSocket, any>} */
const clients = new Map();

function send(ws, msg) {
    if (ws.readyState === ws.OPEN) ws.send(JSON.stringify(msg));
}

function broadcast(room, msg, except) {
    for (const [ws, c] of clients) {
        if (c.room !== room || ws === except) continue;
        send(ws, msg);
    }
}

wss.on('connection', (ws) => {
    const client = { id: randomUUID().slice(0, 8), name: 'survivor', room: null };
    clients.set(ws, client);
    send(ws, { t: 'welcome', version: PROTOCOL_VERSION, you: client.id });

    ws.on('message', (raw) => {
        let msg;
        try {
            msg = JSON.parse(String(raw));
        } catch {
            return;
        }
        handle(ws, client, msg);
    });

    ws.on('close', () => {
        if (client.room) client.room.remove(client.id);
        clients.delete(ws);
    });
});

function handle(ws, client, msg) {
    switch (msg.t) {
        case 'hello': {
            if (msg.version !== PROTOCOL_VERSION) {
                send(ws, { t: 'error', message: 'Client and server versions do not match.' });
                return;
            }
            client.name = String(msg.name ?? 'survivor').slice(0, 20) || 'survivor';
            break;
        }
        case 'rooms': {
            send(ws, { t: 'rooms', rooms: [...rooms.values()].map((r) => r.summary()) });
            break;
        }
        case 'create': {
            const room = new Room(
                String(msg.name ?? 'Island').slice(0, 28) || 'Island',
                msg.pvp !== false,
            );
            rooms.set(room.id, room);
            joinRoom(ws, client, room);
            break;
        }
        case 'join': {
            const room = rooms.get(msg.roomId);
            if (!room) {
                send(ws, { t: 'error', message: 'That room is gone.' });
                return;
            }
            if (room.players.size >= MAX_PLAYERS) {
                send(ws, { t: 'error', message: 'That room is full.' });
                return;
            }
            joinRoom(ws, client, room);
            break;
        }
        case 'leave': {
            if (client.room) {
                client.room.remove(client.id);
                client.room = null;
                send(ws, { t: 'left' });
            }
            break;
        }
        case 'input': {
            const room = client.room;
            if (!room) return;
            const p = room.players.get(client.id);
            if (!p) return;
            p.input = {
                up: !!msg.up,
                down: !!msg.down,
                left: !!msg.left,
                right: !!msg.right,
                run: !!msg.run,
            };
            p.facing = Number(msg.facing) || 0;
            // The ack is what lets the client throw away inputs the server has seen.
            p.ack = Number(msg.seq) || p.ack;
            break;
        }
        case 'build': {
            const room = client.room;
            if (!room) return;
            const kind = String(msg.kind);
            const gx = msg.gx | 0;
            const gy = msg.gy | 0;
            const side = msg.side === 'n' || msg.side === 'w' ? msg.side : undefined;
            const refused = refuseBuild(room, client.id, kind, gx, gy, side);
            if (refused) {
                // The client placed this optimistically the moment you clicked,
                // so it has to be told to take it back rather than left with a
                // wall nobody else can see.
                send(ws, { t: 'refused', what: 'build', kind, gx, gy, side, reason: refused });
                return;
            }
            const piece = addPiece(room, client.id, kind, gx, gy, side);
            broadcast(room, netPieceMessage(piece));
            break;
        }
        case 'door': {
            const room = client.room;
            if (!room) return;
            const s = room.structures.get(msg.id);
            if (!s || s.kind !== 'door') return;
            s.open = !!msg.open;
            broadcast(room, { t: 'door', id: s.id, open: s.open });
            break;
        }
        case 'damage': {
            const room = client.room;
            if (!room) return;
            const piece = room.structures.get(msg.id) ?? room.deployables.get(msg.id);
            if (!piece) return;
            // Clamped, because the amount is a client's word for how hard it
            // hit. Combat is not server-side yet, so this is the one number
            // here that is still taken on trust, capped so a single message
            // cannot flatten a base. See docs/systems/networking.md.
            const amount = Math.min(Math.max(0, Number(msg.amount) || 0), MAX_DAMAGE_PER_HIT);
            piece.hp -= amount;
            if (piece.hp <= 0) {
                removePiece(room, piece);
                broadcast(room, { t: 'destroyed', id: piece.id });
            }
            break;
        }
        case 'chat': {
            const room = client.room;
            if (!room) return;
            const text = String(msg.text ?? '').slice(0, 200);
            if (text) broadcast(room, { t: 'chat', from: client.name, text });
            break;
        }
        case 'invite': {
            const room = client.room;
            if (!room) return;
            const me = room.players.get(client.id);
            const them = room.players.get(String(msg.to ?? ''));
            if (!me || !them || them === me) return;
            // Close enough to speak to: an invite is something you offer the
            // person in front of you, not a message across the island.
            if (Math.hypot(me.x - them.x, me.y - them.y) > INVITE_RANGE) {
                send(ws, { t: 'teamed', message: 'Too far away to ask.' });
                return;
            }
            const theirWs = wsById(them.id);
            if (theirWs) send(theirWs, { t: 'invited', from: me.id, fromName: me.name });
            send(ws, { t: 'teamed', message: `Asked ${them.name} to team up.` });
            break;
        }
        case 'inviteReply': {
            const room = client.room;
            if (!room) return;
            const me = room.players.get(client.id);
            const them = room.players.get(String(msg.from ?? ''));
            if (!me || !them) return;
            const theirWs = wsById(them.id);
            if (!msg.accept) {
                if (theirWs) send(theirWs, { t: 'teamed', message: `${me.name} said no.` });
                break;
            }
            // Join whichever team already exists, or start a new one.
            const team = them.team || me.team || nextTeam++;
            them.team = team;
            me.team = team;
            const names = [...room.players.values()]
                .filter((p) => p.team === team)
                .map((p) => p.name)
                .join(', ');
            for (const p of room.players.values()) {
                if (p.team !== team) continue;
                const w = wsById(p.id);
                if (w) send(w, { t: 'teamed', message: `Team: ${names}.` });
            }
            break;
        }
        case 'leaveTeam': {
            const room = client.room;
            if (!room) return;
            const me = room.players.get(client.id);
            if (!me || !me.team) return;
            me.team = 0;
            send(ws, { t: 'teamed', message: 'You left the team.' });
            break;
        }
        case 'ping': {
            send(ws, { t: 'pong', at: msg.at });
            break;
        }
        default:
            break;
    }
}

function joinRoom(ws, client, room) {
    if (client.room) client.room.remove(client.id);
    client.room = room;
    room.add(client);
    send(ws, { t: 'joined', roomId: room.id, seed: room.seed, you: client.id, tick: room.tick });
    send(ws, {
        t: 'snapshot',
        tick: room.tick,
        players: [...room.players.values()].map(netPlayer),
        structures: [...room.structures.values()],
        deployables: [...room.deployables.values()],
    });
}

// --------------------------------------------------------------- tick loop

let last = Date.now();
setInterval(() => {
    const now = Date.now();
    const dt = Math.min((now - last) / 1000, 0.25);
    last = now;

    for (const room of rooms.values()) {
        room.step(dt);
        const state = {
            t: 'state',
            tick: room.tick,
            players: [...room.players.values()].map(netPlayer),
        };
        if (room.joinedThisTick.length) state.joined = room.joinedThisTick;
        if (room.leftThisTick.length) state.left = room.leftThisTick;
        broadcast(room, state);
        room.joinedThisTick = [];
        room.leftThisTick = [];

        // Reap rooms nobody has been in for a while.
        if (
            room.players.size === 0 &&
            room.emptySince &&
            now - room.emptySince > IDLE_ROOM_SECONDS * 1000
        ) {
            rooms.delete(room.id);
        }
    }
}, TICK_MS);

httpServer.listen(PORT, () => {
    console.log(`Oxide server listening on ws://localhost:${PORT} (${TICK_HZ} Hz)`);
    console.log(
        ALLOWED_ORIGINS.length
            ? `Accepting sockets from: ${ALLOWED_ORIGINS.join(', ')}`
            : 'Accepting sockets from any origin (set ALLOWED_ORIGINS in production)',
    );
});
