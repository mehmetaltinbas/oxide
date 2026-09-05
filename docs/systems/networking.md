# Networking

**This document is the contract for Oxide's multiplayer. Any new synced system
should follow the patterns here, and any new message belongs in
`src/net/protocol.ts`, which both halves import, so it cannot drift.**

The architecture is modelled on the `lumber-wave` project, which solved the same
problem for a canvas game: an authoritative server on a fixed tick, clients that
predict locally and reconcile against the server's answer.

## Two modes

| Mode              | What runs where                                                           |
| ----------------- | ------------------------------------------------------------------------- |
| **Single player** | Everything local. AI clans, wildlife, the lot. No socket.                 |
| **Online**        | Authoritative server owns players and building. Rooms, like Rust servers. |

Picked on the title screen. Online opens a **server browser** listing the rooms
the server is running, with a button to host a new one.

## Why the world is not sent

Every room has a **seed**. The island is generated deterministically from it, so
every client in a room produces a byte-identical map from a single integer.
Terrain, resource nodes, monuments and loot placement never touch the wire.

This is the single biggest bandwidth decision in the project. It only holds as
long as worldgen stays deterministic, **any randomness in worldgen must come
from the seeded RNG, never `Math.random()`**.

## Authority

- **Server owns**: player positions, health, structures, deployables, doors.
- **Client owns**: nothing that matters. It predicts, it does not decide.

### What the server checks about building

A placement is refused unless it is inside the map, on a free spot, and, for a
wall, standing on one of your own foundations. A door only goes into your own
doorway, and nothing goes up inside somebody else's tool cupboard.

It cannot yet check trees, rocks or monument ground, because those come from
worldgen and worldgen is client-only. Those stay client-side refusals, so a
modified client could build on a rock. `damage` is likewise still the client's
word, clamped so one message cannot flatten a base. Both are on the list below.

### Placement is optimistic

The client puts a piece down the moment you click, under an id it invented, and
tells the server. The server answers with `built`/`deployed` carrying the real
id, and the client swaps its own piece for that one. If the server refuses, it
sends `refused` naming the spot, and the client takes the piece back off the
map. Waiting for the round trip instead would put a visible delay on every wall.

`src/features/building/remote-build.ts` is the only place that turns wire
pieces into local ones, and the only place that maps a client id to the numeric
owner the build system uses.

The server re-runs the same movement rule the client does. `applyInput` in
`src/net/client.ts` and the movement block in `server/index.mjs` must stay
identical, if they drift, prediction fights the server on every step and
players rubber-band.

## Tick and frame

- Server: fixed **30 Hz**, `setInterval`, one broadcast per tick.
- Client: renders at its own refresh rate and predicts every frame.

Splitting these is what lets a 144 Hz client feel smooth against a 30 Hz server.

## Prediction and reconciliation

1. Every frame the client applies the local input immediately and **keeps it in
   a queue** with its sequence number.
2. Input is sent **only when it changes**, plus a keepalive every two seconds.
   Holding W does not generate 60 messages a second.
3. The server stamps each player's state with the last sequence number it
   applied (`ack`).
4. On receiving state, the client drops every queued input at or below `ack`,
   **replays the rest** on top of the server position, and eases onto the
   result. Past `SNAP_DISTANCE` it snaps instead, at that point the client is
   simply wrong and smoothing would only prolong it.

## Messages

The types are in `src/features/net/types/`, imported by both halves. Client
sends `hello`, `rooms`, `create`, `join`, `leave`, `input`, `build`, `door`,
`damage`, `chat`, `ping`. Server sends `welcome`, `rooms`, `joined`, `snapshot`,
`state`, `built`, `deployed`, `refused`, `destroyed`, `door`, `chat`, `pong`,
`error`.

`build` carries every placement, walls and deployables alike: the kind says
which, and only edge pieces carry a `side`.

A `snapshot` is the full picture, sent once on join. `state` is the per-tick
update and carries only players plus join/leave lists.

## Running it

```bash
npm run server
```

Then open the game and choose **Play online**. The client finds the server at
the same host on port 8787; override with `?server=ws://host:port`.

## What is not synced yet

Deliberately staged. Online rooms currently have **no AI**, no wildlife, no
clans, because those are the expensive things to make authoritative and they
are what single-player is for. Still to do, in order:

1. **Inventory and crafting** on the server, so loot cannot be fabricated. Until
   this lands, building costs nothing the server can see: it checks that a wall
   may stand somewhere, not that you had the wood for it.
2. **Resource nodes** as server state, so two players cannot harvest the same
   tree, and so placements can be refused for standing on one.
3. **Combat**, projectiles resolved server-side with lag compensation, which is
   what turns `damage` from a report into a request.
4. **Wildlife and clans**, simulated on the server and broadcast like players.
5. **Persistence**, rooms that survive a restart, and a wipe schedule.
6. **Upgrades and containers**: tiers and what is inside a box are still local.

Until then, online is a shared island where people move, build and raid each
other's structures. Anything not in the message list is local-only and **will
differ between clients**, do not build features on top of local state and
assume other players see it.

## Keeping the numbers in step

Anything both halves need lives in `shared/`, as JSON imported by the client
through the `shared` alias and by the server by relative path. Movement, the
build grid, wall thickness, the deployable box and the tool cupboard radius are
all there.

They used to be typed out again in `server/index.mjs`. The server's wall
thickness and the client's were the same only by luck, and its deployable size
did not exist at all, which is how players came to walk through every box on
the island.

## Rules

- New synced state goes in the protocol first, then both ends.
- Never trust a client message: the server validates or ignores it.
- Prefer sending on change over sending on a timer.
- Keep the movement rule in exactly two places, and keep them identical.
