# Performance

**This document is the contract for keeping Oxide fast. Anything added to the
render or update path should follow these rules, and any new technique worth
keeping should be written down here.**

Much of this is adapted from the `lumber-wave` project, which solved the same
problems for a networked canvas game. Where a pattern comes from there it is
noted.

## Budget

One frame at 60fps is 16.7ms. The whole game, simulation, world render and HUD

- must fit inside a fraction of that so the browser has room for everything
  else. Current steady-state on a full island (397 NPCs, 41k resource nodes):

| Phase           | Cost                           |
| --------------- | ------------------------------ |
| `game.update`   | 0.38 ms                        |
| `renderer.draw` | 0.30 ms                        |
| `hud.draw`      | 0.43 ms                        |
| **Total**       | **1.10 ms** (15.6 ms headroom) |

At 1920x1080 with the island fully explored: update 0.12ms, world 0.22ms, HUD
0.30ms, **0.65ms total**. If the observed frame rate is well below the cap, the
limit is the display or the browser, not this.

If a change pushes the total past ~4ms, it needs justifying.

## Rules

### 1. Bake anything static, but watch the memory

Terrain is a flat colour per biome tile, so the **whole island** bakes into one
canvas at **one pixel per tile** (216x216, about 190KB) and is blitted scaled
with `imageSmoothingEnabled = false`. Mottling and water shimmer are drawn live
over visible tiles only, which is bounded by the viewport.

_(lumber-wave prerenders each map layer to an offscreen canvas, one blit per
layer per frame. Same idea, adapted to a map far too large to bake 1:1.)_

**This replaced a per-chunk cache of 1536x1536 canvases that was never evicted.**
Each chunk was 9.4MB; walking the island accumulated close to 2GB, at which
point the browser started dropping canvases and the world rendered as black
bands across the screen. A cache without a bound is a leak with extra steps -
if a bake cannot be small enough to keep forever, it needs an eviction policy.

Caches are keyed on the world object and cleared when it changes, so a new
island never shows a stale bake.

### 2. Simulate only what is near the player

NPCs beyond `NPC_ACTIVE_RADIUS` are skipped entirely, no AI, no pathing, no
separation. On a 20736² island most of the population is nowhere near you, and
the cost of the simulation stays flat no matter how big the map gets.

Distant clans still progress through a **coarse offscreen tick** instead: their
stockpile accrues and walls occasionally go up. Cheap approximations beat
freezing the world.

### 3. Never allocate in a hot path

Per-frame `filter`, `map`, `slice` and inline arrow comparators all allocate.
Draw loops use **reusable module- or instance-level buffers** (`nodeBuf`,
`npcBuf`) and a **shared `byY` comparator**. Spatial queries take an `out`
array to fill rather than returning a new one.

### 4. Index instead of scanning

Anything looked up by id gets a `Map`: `world.nodeById_`, the building grid,
the edge and foundation maps. A linear scan over 41k nodes per gather swing is
the kind of thing that only shows up under load.

_(lumber-wave uses a shared `SpatialGrid` and a `QuadTree` for the same reason.)_

### 5. Recompute derived state only when its inputs change

Room enclosure is a flood fill over the build grid. It runs when the layout
actually changes, a piece placed or destroyed, a door swung, via
`markLayoutChanged()`, not every tick. It used to be marked dirty in `update()`,
which meant paying for the flood fill on every single frame.

Any expensive derived value should follow this shape: a dirty flag, mutations
that set it, and a getter that rebuilds lazily.

### 6. Cull before you draw

Every draw pass takes the camera's `viewBounds` and skips anything outside it.
Spatial queries are rectangle queries against the hash, not full scans.

### 7. Fixed simulation step, variable render

`main.ts` accumulates real time and steps the simulation at a fixed 1/60, capped
at 5 catch-up steps so a stall cannot spiral. Rendering happens once per frame
regardless. This keeps physics and AI identical at any refresh rate.

_(lumber-wave separates these harder still: a 30Hz authoritative server tick and
a 60fps client frame. See docs/systems/networking.md.)_

### 8. Poll edge-triggered input once per frame, not per simulation step

`game.pollInput()` runs from the frame loop, before any simulation steps.

The simulation is a fixed 60Hz while the display refreshes at whatever rate it
likes, so plenty of frames run _zero_ simulation steps. A key pressed on one of
those was cleared at the end of the frame having never been read, which is why
changing belt slot while moving "did not always work". The same bug existed in
the sister project and was fixed the same way.

Held state (movement, mouse held to swing) is safe to read inside the step,
because it is still true next frame. Edges, key presses, placement clicks -
are not.

### 9. Measure, do not guess

The FPS counter is in the top-left corner. For a breakdown, `window.oxide`
exposes the live objects; time `update`, `renderer.draw` and `hud.draw`
separately rather than assuming which one is expensive.

_(lumber-wave ships a `tick-performance-tracker` that attributes time per method
and per entity type. Worth building here if the frame budget ever gets tight.)_

## Known trade-offs

- **Enclosure flood fill** is bounded to the neighbourhood of built structures
  and bails past 4000 cells. A single enormous base could still be slow; if that
  becomes real, cache per-region rather than rebuilding all of them.
- **NPC separation** is O(n²) over _active_ NPCs only. Fine at ~40 active, would
  need the spatial grid if the active radius grew.
