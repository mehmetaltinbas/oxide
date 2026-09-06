import PLAYER_NUMBERS from 'shared/player.json';
import {
    INTERPOLATION_DELAY,
    SNAPSHOT_BUFFER,
} from 'src/features/net/constants/interpolation-delay.constant';
import { PROTOCOL_VERSION } from 'src/features/net/constants/protocol-version.constant';
import { ClientMessage } from 'src/features/net/types/client-message.type';
import { NetPlayer } from 'src/features/net/types/net-player.interface';
import { NetStatus } from 'src/features/net/types/net-status.type';
import { NetDeployable } from 'src/features/net/types/net-deployable.interface';
import { NetStructure } from 'src/features/net/types/net-structure.interface';
import { RoomSummary } from 'src/features/net/types/room-summary.interface';
import { ServerMessage } from 'src/features/net/types/server-message.type';
import { decode } from 'src/features/net/utils/decode.util';
import { encode } from 'src/features/net/utils/encode.util';

interface PendingInput {
    seq: number;
    dt: number;
    up: boolean;
    down: boolean;
    left: boolean;
    right: boolean;
    run: boolean;
}

// The same numbers the server moves you with. They were typed out again here,
// so changing the player's speed would have moved the server and left
// prediction running at the old one, which is a rubber-band nobody would have
// connected to the edit that caused it.
const SPEED = PLAYER_NUMBERS.speed;
const SPRINT = PLAYER_NUMBERS.sprint;
const RADIUS = PLAYER_NUMBERS.radius;
/** How hard the client is pulled back onto the server's answer, per second. */
const RECONCILE_RATE = 9;
/** Past this gap, snap rather than sliding, the client is simply wrong. */
const SNAP_DISTANCE = 220;

/**
 * Client half of the netcode: one socket, a room list, and an input queue that
 * powers prediction and reconciliation. See docs/systems/networking.md.
 */
export class NetClient {
    status: NetStatus = 'offline';
    error = '';
    youId = '';
    roomId = '';
    seed = 0;
    ping = 0;

    rooms: RoomSummary[] = [];
    /** Everyone in the room except you, ready to draw. */
    others = new Map<string, NetPlayer>();
    /**
     * The last few states, kept with the moment each arrived, so other players
     * can be drawn smoothly between two of them rather than snapped to the
     * newest. See `interpolate`.
     */
    private snapshots: { at: number; players: Pose[] }[] = [];
    structures = new Map<number, NetStructure>();
    deployables = new Map<number, NetDeployable>();

    /** Server's last word on where you are. */
    serverX = 0;
    serverY = 0;
    /** Where the client thinks you are, after prediction. */
    predictedX = 0;
    predictedY = 0;

    onChat: ((from: string, text: string) => void) | null = null;
    /**
     * Push a circle out of the room's building, the same rule the server runs.
     * Set by the game, because the net client does not know what a wall is.
     */
    solid: ((x: number, y: number, radius: number) => { x: number; y: number }) | null = null;
    onJoined: ((seed: number) => void) | null = null;
    /**
     * The room's building, handed to whoever knows how to put it on the map.
     * The net client holds the wire state and nothing else: it does not know
     * what a foundation is.
     */
    onBuildSnapshot: ((s: NetStructure[], d: NetDeployable[]) => void) | null = null;
    onBuilt: ((s: NetStructure) => void) | null = null;
    onDeployed: ((d: NetDeployable) => void) | null = null;
    onDestroyed: ((id: number) => void) | null = null;
    onDoor: ((id: number, open: boolean) => void) | null = null;
    onRefused:
        | ((r: { kind: string; gx: number; gy: number; side?: 'n' | 'w'; reason: string }) => void)
        | null = null;

    private ws: WebSocket | null = null;
    private seq = 0;
    private pending: PendingInput[] = [];
    private lastSent = '';
    private pingTimer = 0;
    private name = 'survivor';

    connect(url: string, name: string): void {
        this.disconnect();
        this.name = name;
        this.status = 'connecting';
        this.error = '';
        try {
            this.ws = new WebSocket(url);
        } catch {
            this.status = 'error';
            this.error = 'Could not open a connection.';
            return;
        }
        this.ws.onopen = () => {
            this.send({ t: 'hello', version: PROTOCOL_VERSION, name: this.name });
            this.send({ t: 'rooms' });
            this.status = 'lobby';
        };
        this.ws.onmessage = (ev) => {
            const msg = decode<ServerMessage>(String(ev.data));
            if (msg) this.receive(msg);
        };
        this.ws.onerror = () => {
            this.status = 'error';
            this.error = 'Connection failed. Is the server running?';
        };
        this.ws.onclose = () => {
            if (this.status !== 'error') this.status = 'offline';
            this.ws = null;
        };
    }

    disconnect(): void {
        this.ws?.close();
        this.ws = null;
        this.status = 'offline';
        this.others.clear();
        this.structures.clear();
        this.pending.length = 0;
    }

    get connected(): boolean {
        return this.ws !== null && this.ws.readyState === WebSocket.OPEN;
    }

    private send(msg: ClientMessage): void {
        if (this.connected) this.ws!.send(encode(msg));
    }

    refreshRooms(): void {
        this.send({ t: 'rooms' });
    }

    createRoom(name: string, pvp: boolean): void {
        this.status = 'joining';
        this.send({ t: 'create', name, pvp });
    }

    joinRoom(id: string): void {
        this.status = 'joining';
        this.send({ t: 'join', roomId: id });
    }

    leaveRoom(): void {
        this.send({ t: 'leave' });
        this.status = 'lobby';
        this.others.clear();
        this.structures.clear();
        this.deployables.clear();
    }

    chat(text: string): void {
        this.send({ t: 'chat', text });
    }

    reportBuild(kind: string, gx: number, gy: number, side?: 'n' | 'w'): void {
        this.send({ t: 'build', kind, gx, gy, side });
    }

    reportDoor(id: number, open: boolean): void {
        this.send({ t: 'door', id, open });
    }

    reportDamage(id: number, amount: number): void {
        this.send({ t: 'damage', id, amount });
    }

    /** True while this client is in a room, so callers know to report at all. */
    get online(): boolean {
        return this.status === 'playing';
    }

    // ------------------------------------------------------------- prediction

    /**
     * Advance the local player by one step and remember the input, so the same
     * steps can be replayed after the server corrects us.
     */
    predict(dt: number, input: Omit<PendingInput, 'seq' | 'dt'>, facing: number): void {
        if (this.status !== 'playing') return;
        this.seq++;
        const entry: PendingInput = { seq: this.seq, dt, ...input };
        this.pending.push(entry);
        if (this.pending.length > 240) this.pending.shift();

        const moved = applyInput(this.predictedX, this.predictedY, entry);
        this.predictedX = moved.x;
        this.predictedY = moved.y;

        // Only talk when something changed, plus a keepalive every so often.
        const signature = `${input.up}${input.down}${input.left}${input.right}${input.run}${Math.round(facing * 20)}`;
        if (signature !== this.lastSent) {
            this.lastSent = signature;
            this.send({ t: 'input', seq: this.seq, ...input, facing });
        }

        this.pingTimer -= dt;
        if (this.pingTimer <= 0) {
            this.pingTimer = 2;
            this.send({ t: 'ping', at: Date.now() });
            this.send({ t: 'input', seq: this.seq, ...input, facing });
        }
    }

    /**
     * Fold the server's answer back in: replay every input it has not seen yet,
     * then ease onto the result rather than snapping.
     */
    /**
     * Draw other players where they were a moment ago, smoothly.
     *
     * Called once a frame, before anything reads `others`. They used to be
     * snapped to the newest state, so everybody else moved in 33ms steps while
     * your own survivor, predicted locally, glided. This walks back
     * `INTERPOLATION_DELAY` and blends the two states either side of it.
     *
     * Positions and facings only. Health and the rest stay at the newest the
     * server sent: a stale number is a lie, a stale position is just where
     * somebody was.
     */
    interpolate(): void {
        if (this.snapshots.length < 2) return;
        const target = performance.now() - INTERPOLATION_DELAY * 1000;

        // Past the end of the buffer, hold on the newest pair rather than
        // running ahead of what the server has actually said.
        let older = this.snapshots[this.snapshots.length - 2];
        let newer = this.snapshots[this.snapshots.length - 1];
        for (let i = 0; i < this.snapshots.length - 1; i++) {
            if (this.snapshots[i].at <= target && this.snapshots[i + 1].at >= target) {
                older = this.snapshots[i];
                newer = this.snapshots[i + 1];
                break;
            }
        }

        const span = newer.at - older.at;
        const t = span > 0 ? clamp01((target - older.at) / span) : 1;
        for (const p of this.others.values()) {
            const a = older.players.find((x) => x.id === p.id);
            const b = newer.players.find((x) => x.id === p.id);
            if (!a || !b) continue;
            p.x = a.x + (b.x - a.x) * t;
            p.y = a.y + (b.y - a.y) * t;
            p.facing = lerpAngle(a.facing, b.facing, t);
        }
    }

    private reconcile(me: NetPlayer, dt = 1 / 60): void {
        this.serverX = me.x;
        this.serverY = me.y;

        this.pending = this.pending.filter((p) => p.seq > me.ack);
        let x = me.x;
        let y = me.y;
        for (const p of this.pending) {
            const moved = applyInput(x, y, p);
            // Push out of walls and boxes on every replayed step, because the
            // server does that on every step it takes. Replaying without it
            // meant the two disagreed exactly where it shows: running along
            // your own wall, the replay walked through it and the correction
            // dragged you back out.
            const solid = this.solid?.(moved.x, moved.y, RADIUS) ?? moved;
            x = solid.x;
            y = solid.y;
        }

        const gap = Math.hypot(x - this.predictedX, y - this.predictedY);
        if (gap > SNAP_DISTANCE) {
            this.predictedX = x;
            this.predictedY = y;
            return;
        }
        const t = 1 - Math.pow(0.0001, dt * (RECONCILE_RATE / 9));
        this.predictedX += (x - this.predictedX) * t;
        this.predictedY += (y - this.predictedY) * t;
    }

    private receive(msg: ServerMessage): void {
        switch (msg.t) {
            case 'welcome':
                this.youId = msg.you;
                break;
            case 'rooms':
                this.rooms = msg.rooms;
                break;
            case 'joined':
                this.roomId = msg.roomId;
                this.seed = msg.seed;
                this.youId = msg.you;
                this.status = 'playing';
                this.pending.length = 0;
                this.onJoined?.(msg.seed);
                break;
            case 'left':
                this.status = 'lobby';
                this.others.clear();
                break;
            case 'error':
                this.error = msg.message;
                if (this.status === 'joining') this.status = 'lobby';
                break;
            case 'snapshot': {
                this.others.clear();
                for (const p of msg.players) {
                    if (p.id === this.youId) {
                        this.serverX = p.x;
                        this.serverY = p.y;
                        this.predictedX = p.x;
                        this.predictedY = p.y;
                    } else this.others.set(p.id, p);
                }
                this.structures.clear();
                for (const s of msg.structures) this.structures.set(s.id, s);
                this.deployables.clear();
                for (const d of msg.deployables) this.deployables.set(d.id, d);
                this.onBuildSnapshot?.(msg.structures, msg.deployables);
                break;
            }
            case 'state': {
                // Kept with the moment it landed, so `interpolate` can draw
                // between two states instead of jumping to this one.
                // Copies, not the objects themselves. `others` points at the
                // very objects the message arrived in, so keeping those would
                // mean blending mutated the state it was blending from: each
                // frame pulled the newest position toward the older one, and
                // instead of gliding, people stalled and jumped.
                this.snapshots.push({ at: performance.now(), players: msg.players.map(pose) });
                if (this.snapshots.length > SNAPSHOT_BUFFER) this.snapshots.shift();
                const seen = new Set<string>();
                for (const p of msg.players) {
                    seen.add(p.id);
                    if (p.id === this.youId) this.reconcile(p);
                    else this.others.set(p.id, p);
                }
                for (const id of [...this.others.keys()]) {
                    if (!seen.has(id)) this.others.delete(id);
                }
                break;
            }
            case 'built':
                this.structures.set(msg.structure.id, msg.structure);
                this.onBuilt?.(msg.structure);
                break;
            case 'deployed':
                this.deployables.set(msg.deployable.id, msg.deployable);
                this.onDeployed?.(msg.deployable);
                break;
            case 'refused':
                this.onRefused?.({
                    kind: msg.kind,
                    gx: msg.gx,
                    gy: msg.gy,
                    side: msg.side,
                    reason: msg.reason,
                });
                break;
            case 'destroyed':
                this.structures.delete(msg.id);
                this.deployables.delete(msg.id);
                this.onDestroyed?.(msg.id);
                break;
            case 'door': {
                const s = this.structures.get(msg.id);
                if (s) s.open = msg.open;
                this.onDoor?.(msg.id, msg.open);
                break;
            }
            case 'chat':
                this.onChat?.(msg.from, msg.text);
                break;
            case 'pong':
                this.ping = Date.now() - msg.at;
                break;
            default:
                break;
        }
    }
}

/** The movement rule. Must stay identical to the server's, or they will fight. */
/** Just enough of a player to draw them between two moments. */
interface Pose {
    id: string;
    x: number;
    y: number;
    facing: number;
}

function pose(p: { id: string; x: number; y: number; facing: number }): Pose {
    return { id: p.id, x: p.x, y: p.y, facing: p.facing };
}

function clamp01(v: number): number {
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

/** Blend two angles the short way round, so facing never spins the long way. */
function lerpAngle(a: number, b: number, t: number): number {
    let d = b - a;
    while (d > Math.PI) d -= Math.PI * 2;
    while (d < -Math.PI) d += Math.PI * 2;
    return a + d * t;
}

function applyInput(x: number, y: number, p: PendingInput): { x: number; y: number } {
    let dx = (p.right ? 1 : 0) - (p.left ? 1 : 0);
    let dy = (p.down ? 1 : 0) - (p.up ? 1 : 0);
    const len = Math.hypot(dx, dy);
    if (len === 0) return { x, y };
    dx /= len;
    dy /= len;
    const speed = SPEED * (p.run ? SPRINT : 1);
    return { x: x + dx * speed * p.dt, y: y + dy * speed * p.dt };
}
