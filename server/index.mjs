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

const PORT = Number(process.env.PORT ?? 8787);
const PROTOCOL_VERSION = 1;
const TICK_HZ = 30;
const TICK_MS = 1000 / TICK_HZ;

// Movement constants must match the client's, or prediction will fight the
// server on every step.
const PLAYER_SPEED = 132;
const PLAYER_SPRINT = 1.4;
const PLAYER_RADIUS = 13;
const WORLD_W = 20736;
const WORLD_H = 20736;
const CELL = 64;

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

function clamp(v, lo, hi) {
    return v < lo ? lo : v > hi ? hi : v;
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
    };
}

/** Push a circle out of any solid wall in the room. Mirrors the client rule. */
function resolveAgainstBuilding(room, x, y, radius) {
    const thickness = 9;
    for (const s of room.structures.values()) {
        if (s.kind === 'foundation' || s.kind === 'doorway') continue;
        if (s.kind === 'door' && s.open) continue;
        const ax = s.gx * CELL;
        const ay = s.gy * CELL;
        const bx = s.side === 'n' ? ax + CELL : ax;
        const by = s.side === 'n' ? ay : ay + CELL;
        const hit = closestOnSegment(x, y, ax, ay, bx, by);
        const d = Math.hypot(x - hit.x, y - hit.y);
        const min = radius + thickness;
        if (d < min && d > 0.0001) {
            const push = (min - d) / d;
            x += (x - hit.x) * push;
            y += (y - hit.y) * push;
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
            const s = {
                id: room.nextBuildId++,
                kind: String(msg.kind),
                tier: 'twig',
                gx: msg.gx | 0,
                gy: msg.gy | 0,
                side: msg.side,
                hp: 10,
                maxHp: 10,
                owner: client.id,
                open: false,
            };
            room.structures.set(s.id, s);
            broadcast(room, { t: 'built', structure: s });
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
            const s = room.structures.get(msg.id);
            if (!s) return;
            s.hp -= Math.max(0, Number(msg.amount) || 0);
            if (s.hp <= 0) {
                room.structures.delete(s.id);
                broadcast(room, { t: 'destroyed', id: s.id });
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
