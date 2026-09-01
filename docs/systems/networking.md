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

- **Server owns**: player positions, health, structures, doors.
- **Client owns**: nothing that matters. It predicts, it does not decide.

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

All in `src/net/protocol.ts`. Client sends `hello`, `rooms`, `create`, `join`,
`leave`, `input`, `build`, `door`, `damage`, `chat`, `ping`. Server sends
`welcome`, `rooms`, `joined`, `snapshot`, `state`, `built`, `destroyed`,
`door`, `chat`, `pong`, `error`.

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

1. **Inventory and crafting** on the server, so loot cannot be fabricated.
2. **Resource nodes** as server state, so two players cannot harvest the same tree.
3. **Combat**, projectiles resolved server-side with lag compensation.
4. **Wildlife and clans**, simulated on the server and broadcast like players.
5. **Persistence**, rooms that survive a restart, and a wipe schedule.

Until then, online is a shared island where people move, build and raid each
other's structures. Anything not in the message list is local-only and **will
differ between clients**, do not build features on top of local state and
assume other players see it.

## Rules

- New synced state goes in the protocol first, then both ends.
- Never trust a client message: the server validates or ignores it.
- Prefer sending on change over sending on a timer.
- Keep the movement rule in exactly two places, and keep them identical.
