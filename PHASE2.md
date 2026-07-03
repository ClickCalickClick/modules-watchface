# Phase 2 — Multi-module grids, new module types, new platforms

## Context
Phase 1 made the watchface responsive (runtime-derived geometry, scalable icons) and
added full emery support on a 2×2 grid. Phase 2 expands the watchface from 4 fixed
modules to **9 module types** laid out on **per-platform grids**, and brings in the two
new Core Devices platforms (flint, gabbro). Decisions below are locked with the user.

## Locked decisions
- **Grids:** 144×168 platforms (aplite/basalt/diorite/flint) = **2×2** (4 cells);
  emery (200×228) = **3×3** (9 cells); gabbro (260×260 round) = **bespoke radial**
  (center time + ring of 8) — see mockups in `design-mockups/round_C_bespoke.png`.
- **New module types (5):** Second time zone, Week number, Countdown, Distance,
  Next calendar event. Added to existing Date, Weather, Time, Stats = **9 total**.
- **Phasing:** **2a = flint + emery 3×3 + the 5 new modules + settings/persistence.**
  **2b = gabbro round (bespoke renderer).**

## Platform facts (from SDK `pebble_sdk_platform.py`)
- flint: 144×168, **PBL_BW**, rect, health+compass. Geometrically == diorite → nearly free.
- gabbro: 260×260, **PBL_COLOR**, **PBL_ROUND**, health+compass+touch. Round — needs new renderer.

## Module data sources
| Module | Source | Notes |
|---|---|---|
| Date / Weather / Time / Stats | existing | unchanged logic, re-fit to cells |
| Second time zone | on-watch | needs a UTC-offset + label setting |
| Week number | on-watch | `strftime("%V")` ISO week |
| Countdown | on-watch | needs a target-date setting; show days remaining |
| Distance | Health API | `health_service_sum_today(HealthMetricWalkedDistanceMeters)`; km/mi |
| Next calendar event | **phone (PebbleKit JS)** | JS reads calendar, sends title + start via AppMessage |

## Key engineering work (Phase 2a, all in `src/c/modules.c` unless noted)
1. **Per-platform grid config.** Replace the fixed `GRID_COLS/GRID_ROWS/NUM_CELLS`
   with a per-platform choice via `PBL_PLATFORM_SWITCH`/`PBL_PLATFORM_TYPE_CURRENT`
   (emery → 3×3, else → 2×2). `compute_screen_geometry` already builds
   `s_quadrant_origins[]` from cols/rows; widen the array to `MAX_CELLS = 9`.
2. **Cell model 4 → up to 9.** `s_quadrant_modules[]`, backgrounds/colors/text arrays,
   and the Q-loops grow to `NUM_CELLS`. Persistence keys Q5–Q9 + new settings keys.
3. **New `ModuleType` values + render/layout/update** for the 5 new modules, each with a
   layout template (authored for the cell base) and a draw/update path mirroring existing
   modules. Reuse `scale_layout`/`sysfont`.
4. **Smaller-cell fonts (RISK).** emery 3×3 cells are ~66×76 vs 2×2's 100×114. The Time
   module's stacked 42px hour+minute (~84px) **won't fit** 76px tall. Add a cell-size-aware
   font tier (e.g. when cell height < ~90px, drop hero to GOTHIC_28, or render HH:MM on one
   line). Generalize `sysfont` to take available cell size, not just screen width.
5. **Settings (Clay).** `src/pkjs/config.js`: show 9 cell dropdowns on emery
   (`config.platform`), 4 elsewhere; add the 9 module options; add inputs for TZ offset/label,
   countdown date, distance units. `src/pkjs/index.js`: calendar fetch + AppMessage send.
   `package.json`: new `messageKeys` (Quadrant5–9Module, TZOffset, TZLabel, CountdownDate,
   DistanceUnits, CalendarTitle, CalendarTime); add `flint` (and later `gabbro`) to
   `targetPlatforms`; ensure `capabilities: ["health"]`.
6. **Icons.** New modules are mostly text; if any need glyphs (e.g. calendar), generate via
   the existing pipeline (see `resources/images/CREDITS.txt`) with `~emery` variants.

## Phase 2b (gabbro round, later)
Bespoke radial renderer: large centered Time, 8 module "pods" on a ring (mockup C). Guarded
by `PBL_ROUND`. Separate from the grid path; reuses module data/draw code but its own
positioning. Add `gabbro` to `targetPlatforms` and handle `PBL_COLOR` (it is color).

## Build / verification
- `pebble build` must pass for aplite/basalt/diorite/emery/flint (+gabbro in 2b).
- emery emulator: 3×3, all 9 modules legible, no clipping; settings pick per-cell modules.
- basalt/flint emulator: unchanged 2×2 regression.
- Calendar/TZ/countdown exercised via Clay settings + emulator.

## BLOCKER — clay vs flint/gabbro (discovered during 2a.1)
`pebble-clay` 1.0.4 only declares aplite/basalt/chalk/diorite/emery and ships a `dist.zip`
that the build **re-extracts every time**, so adding flint/gabbro platform stubs to
`node_modules` does not stick. Clay's C side is just an empty `libpebble-clay.a` + stub
header, so the fix is mechanical — but it must survive `npm install` + the zip extraction.
Options: (a) **vendor a patched clay** in-repo (`file:` dependency) — fully durable, recommended;
(b) `patch-package` postinstall (binary `.a`/zip is awkward); (c) wait for / use an updated
clay fork. Until resolved, flint stays OUT of `targetPlatforms`. gabbro (2b) needs the same fix.
**Decision: bundle flint enablement with the clay fix; not on the emery-3×3 critical path.**

## Calendar data-source caveat
PebbleKit JS cannot read the phone's calendar natively. The watch-side Calendar module is
complete (renders `CalendarTitle`/`CalendarTime` sent via AppMessage), but a real data source
(e.g. Google Calendar OAuth or a companion web service) is a **follow-up**. Until then the
module shows "--".

## Status
- [x] 2a.1 flint enabled + clay swapped to @rebble/clay 1.0.10 (supports flint/gabbro)
- [x] 2a.2 per-platform grid (emery 3×3, others 2×2) + cell model 4→9 + persistence loops
- [x] 2a.3 five new module types (TZ, Week, Countdown, Distance, Calendar*)
- [x] 2a.4 cell-size-aware fonts
- [x] 2a.5 Clay settings (9 cells + module options + TZ/countdown/distance)
- [x] 2a.6 emery 3×3 + basalt 2×2 verified in emulator; all 5 platforms build
- [ ] Calendar real data source → moved to Phase 3d (ICS URL approach, see PHASE3.md)
- [ ] 2b gabbro bespoke round renderer → moved to Phase 3b (shared with chalk, see PHASE3.md)
