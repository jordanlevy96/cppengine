# World Map System Implementation (EU/Factorio-Style)
**Status:** Planned (design + implementation notes)  
**Last Updated:** 2026-01-28  
**Version:** 0.1.0

---

## Scope

This document defines the target architecture for an **EU-style world map** (provinces, borders, political/strategic overlays) combined with **Factorio-style automation** (large simulation, local activity windows, dynamic world changes).

The previous clipmap terrain prototype has been removed; this design is explicitly **world-centric** (stable IDs and coordinates), not camera-centric.

---

## Design goals (non-negotiable)

- **Stable addressing**: all persistent state is keyed by stable world coordinates and IDs (not camera-relative structures).
- **Incremental updates**: edits (terraforming, river diversion, construction) trigger bounded recompute and bounded GPU uploads.
- **Scalable simulation**: simulation runs by chunk scheduling + LOD (detailed near, aggregated far).
- **Determinism-friendly**: serialization, replay, and networking should be possible later (no hidden camera-driven state).
- **Fast picking/selection**: province picking and entity picking are first-class (ID buffers / ID textures).

---

## Core concepts

### 1) World coordinates (authoritative) vs render coordinates (ephemeral)

- **Authoritative world space** is integer-addressed and stable.
- **Render space** is camera-relative floats (floating origin) derived from authoritative coordinates.

Recommended coordinate types:

- `ChunkCoord` = `(int32 chunkX, int32 chunkY)`
- `CellCoord` = `(int32 x, int32 y)` (world cell coordinates in a chosen “simulation resolution”)
- `CellInChunk` = `(uint16 localX, uint16 localY)` with fixed chunk dimensions
- `WorldCell` = `(ChunkCoord, CellInChunk)` (or `(int64 x, int64 y)`)

### 2) Chunking (the unit of persistence and scheduling)

Use fixed-size chunks (example: `256×256` simulation cells per chunk):

- persistence: one file (or block) per chunk per layer
- simulation scheduling: activate/deactivate by chunk
- dirty propagation: edits mark chunks dirty
- streaming: cache by chunk keys

### 3) Render tiles (the unit of LOD and GPU cache)

Rendering uses **stable tiles** selected by a quadtree/LOD scheme:

- key: `(lod, tileX, tileY)`
- each tile covers a stable world rectangle
- tiles map to textures (arrays or atlas) stored in a GPU cache

This is distinct from simulation chunks:
- simulation chunk size is chosen for CPU scheduling and locality
- render tile size is chosen for GPU cache efficiency and visual resolution

---

## Data model

### Base layers (authoritative CPU)

At minimum:

- `elevation` (height field or “terrain class”)
- `water` (water depth / water mask)
- `soil` / `biome` (for visuals and gameplay rules)
- `provinceId` (integer ID per cell)

Optional but likely:
- `riverMask` / `flowDir` / `flowAccum` derived layers
- `roadMask` / `railMask` / `navCost`
- `resourceMask` (ore, fertility, etc.)

### Derived layers (recomputed)

Derived layers are produced by deterministic transforms on base layers:

- hydrology: flow direction, accumulation, river network extraction
- borders: province edge mask, coastlines
- rendering: color layers, shading layers, normal/gradient layers

### Edits (operations, not full rewrites)

Edits should be expressed as operations on world coordinates:

- raise/lower terrain in a brush region
- carve a channel / build a dam
- paint province IDs / split provinces (later)
- place/remove automation entities

Edits should:
- record as an operation log (for persistence and undo/redo later), and/or
- apply into a delta layer per chunk, with periodic compaction into base layers.

---

## Persistence and streaming

### Chunk storage layout

One workable layout:

```
res/world/
  meta.yaml
  chunks/
    cx_0000_cy_0000/
      elevation.bin
      water.bin
      biome.bin
      provinceId.bin
      edits.log
    cx_0000_cy_0001/
      ...
```

Notes:
- Keep formats simple first (raw or lightly compressed).
- Version chunk files (magic + version) to support migrations.

### Streaming pipeline (CPU)

Goal: load/unload chunk data based on camera + simulation activity window.

Recommended stages:
1. Determine needed chunks (camera view + simulation window + prefetch margin).
2. Background I/O thread loads chunk blobs into CPU memory.
3. Main thread (or worker threads) build derived layers / render tiles.
4. Main thread uploads render tiles to GPU cache with a per-frame budget.

---

## Rendering architecture

### 1) Tile selection (quadtree LOD)

Use quadtree traversal to select a set of tiles that:
- cover the camera view
- meet a screen-space error threshold (or zoom-level threshold)

This yields a list of stable render tiles `(lod, x, y)`.

### 2) GPU cache (texture array or atlas)

Maintain an LRU cache of tile textures on GPU:
- key → slot index
- eviction policy: LRU by last used frame
- upload policy: budgeted per frame (bytes or tiles)

Typical tile texture layers:
- `baseColor` (RGBA8)
- `provinceId` (R16UI or R32UI)
- `water` (R8/R16)
- `elevation` (R16/R16F) if needed for shading

### 3) Borders and picking

Borders:
- store `provinceId` per cell in a texture
- in shader, sample neighbors and draw border where IDs differ

Picking:
- option A: render an offscreen ID buffer (provinceId/entityId)
- option B: sample `provinceId` texture at mouse position (requires mapping from screen → world)

### 4) Projection

Choose early:
- **flat wrapped map** (e.g., equirectangular with horizontal wrap) for EU-style
- **spherical/cube-sphere** if you need true globe behavior

Flat wrapped maps are simpler and align with “EU-style” expectations.

---

## Simulation architecture (Factorio-style automation)

### 1) Simulation cells vs entities

Keep two grids:
- **cell grid**: terrain/water/province per cell (chunked)
- **entity graph**: automation entities (belts, machines, pipes) anchored to cells

### 2) Chunk scheduling (activity windows)

Define:
- `ActiveChunks`: near camera or within user-defined regions
- `WarmChunks`: cached but simulated at reduced rate
- `ColdChunks`: persisted only (no active sim)

### 3) Simulation LOD

To scale:
- near: detailed per-entity simulation (tick-by-tick)
- far: aggregated throughput simulation per chunk/region
- transitions must conserve resources and remain deterministic

---

## Dynamic world changes (terraforming + rivers)

### Dirty region pipeline

When an edit occurs:
1. Convert edit region → set of affected chunks.
2. Apply edit ops to chunk base/delta layers.
3. Mark derived layers dirty for a bounded influence region.
4. Recompute derived layers incrementally.
5. Invalidate and rebuild intersecting render tiles.
6. Budgeted GPU uploads update only those tiles.

### River diversion specifics (incremental hydrology)

Hydrology recompute should be region-bounded:
- local terrain change updates flow directions locally
- changes propagate downstream; cap propagation by:
  - watershed boundaries, and/or
  - a max influence distance per tick, and/or
  - queued incremental propagation over multiple frames

Keep the recompute deterministic (same inputs → same outputs).

---

## Engine integration points (Imhotep)

### Prefer C++ “services” for heavy world systems

Large systems (map streaming, hydrology, automation) should be C++ services exposed to Lua via narrow APIs:
- use `ServiceRegistry` (see `include/systems/ServiceRegistry.h`) to register `WorldMapService`, `HydrologyService`, etc.
- Lua scripts remain orchestration/UI and high-level gameplay rules

### Render loop ownership

Rendering remains main-thread (OpenGL context):
- CPU work can be multithreaded (I/O, tile builds, derived recompute)
- GPU uploads must be budgeted and done on the main thread

---

## Implementation roadmap (phased)

### Phase 0 — Foundations

- Define coordinate types and conversions (cell/chunk/tile).
- Implement chunk storage for `provinceId` and a dummy `baseColor` layer.
- Implement quadtree tile selection for a flat wrapped map.
- Implement GPU tile cache with LRU + budgeted uploads.

### Phase 1 — EU map visuals + picking

- Province borders via `provinceId` neighbor compare in shader.
- Province picking (ID buffer or texture sampling path).
- UI overlay wiring for selected province.

### Phase 2 — Terrain + water edits

- Add `elevation` + `water` layers, edits, dirty-region propagation.
- Add incremental derived recompute (flow + rivers) with bounded propagation.
- Update render tiles incrementally on changes.

### Phase 3 — Automation simulation

- Implement entity placement, chunk scheduling, and near/far simulation LOD.
- Integrate with persistence and deterministic stepping.

---

## Open questions (answer early)

- Projection: flat wrapped vs globe.
- Cell resolution: smallest meaningful simulation cell size.
- Province representation: authoritative raster IDs vs vector polygons with raster cache.
- Serialization format and compression strategy.

