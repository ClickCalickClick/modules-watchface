# Phase 3 — Module registry, all 7 platforms, ~16 new modules, touch, Clay preview

## Context
Phase 2a delivered the emery 3×3 grid, 5 new module types (9 total + Empty), and flint.
Phase 3 (decisions locked with the user 2026-07-03) completes platform coverage, adds a
large second wave of modules, touch interaction, and a live settings preview.

## Locked decisions
- **Capacity:** 9 slots per screen is the maximum everywhere. No swipe pages, no denser grids.
- **Platforms:** all 7 SDK 4.17 targets. Add **chalk** (Pebble Time Round, 180×180 round)
  and **gabbro** (260×260 round, touch) to the existing 5.
- **Round renderer:** shared radial layout — center time + ring of pods.
  gabbro = 8 pods, chalk = 4–6 pods (whatever proves legible in the emulator).
  Mockup: `design-mockups/round_C_bespoke.png`.
- **Calendar data source:** user pastes an **ICS URL** into Clay; pkjs fetches + parses the
  next upcoming event. No OAuth, no companion service.
- **New modules — all four sets approved:**
  - *Health:* heart rate, sleep, calories, active minutes, step-goal progress ring
  - *Sky:* sunrise/sunset, moon phase, weather high/low + humidity/wind/UV, AQI
  - *Utility:* Bluetooth status (+ optional disconnect vibe), Quiet Time, day-of-year,
    count-up (days since), custom text, analog mini-clock
  - *Finance:* stock/crypto ticker (one symbol per cell)
- **Touch (emery + gabbro, both PBL_TOUCH in SDK 4.17):** tap-based only.
  Tap a cell → temporary detail overlay; tap-to-act (weather refresh, stats cycling).
  Guard with `PBL_TOUCH` + `touch_service_is_enabled()`. Spike first: confirm TouchService
  events are delivered to watchfaces (not just apps) in the emulator.
- **Clay preview:** live canvas preview via `clay.registerComponent` custom component —
  platform-aware (2×2 / 3×3 / round), simulates colors, auto-text-color, and B&W dithering.
  Approximate fonts are acceptable; layout + colors must be faithful.

## Plan / status
- [x] **3a — Foundation refactor** (branch `phase3a-module-registry`)
  - [x] Module registry: per-type descriptor (layout template authored in BASE_QUAD space,
        font roles, alignment, data-source pointer); generic per-cell build/render loops
        replace the six per-module switch statements in `modules.c`.
  - [x] Lazy layer creation: only modules assigned to a cell get layers (aplite has a
        24 KB app-memory cap; ~25 eager module types would not fit). Duplicate module
        assignments across cells are now supported.
  - [x] Delete the duplicate-module rule and the custom `webviewclosed` handler in
        `src/pkjs/index.js` (Clay auto-handles settings; weather refetch now runs 2 s
        after save). Fixes the double-send race and the cells-5–9 gap.
  - [x] Weather over HTTPS.
  - [x] Strip per-minute APP_LOG debug spam.
  - [x] README refresh (9 modules, 3×3, platform matrix; Pebble Time 2 is shipping hardware).
  - [x] Build all 5 current platforms + emulator regression (basalt 2×2, emery 3×3, aplite B&W).
        Note: module *reassignment* via the config page still needs a hands-on test
        (exercises the new cell rebuild path).
- [ ] **3b — chalk + gabbro:** radial renderer behind `PBL_ROUND`, condensed pod variants of
      Date/Weather/Stats, add both to `targetPlatforms`, all-platform builds + screenshots.
- [ ] **3c — On-watch modules:** BT status, HR, goal ring, sleep, calories, active minutes,
      moon phase, day-of-year, count-up, custom text, Quiet Time, analog mini-clock.
- [ ] **3d — Phone-data modules:** sunrise/sunset, weather hi/lo/humidity/wind/UV, AQI,
      ICS calendar, stock/crypto ticker; replace raw TZ offset with an IANA city picker
      (pkjs computes DST-correct offset via `Intl` and pushes updates).
- [ ] **3e — Touch:** emulator delivery spike, then tap overlays + tap actions.
- [ ] **3f — Clay live preview** custom component; group the (by then ~25) module options
      into categorized option groups in the dropdowns.

## Verification (every phase)
- `pebble build` green for every platform in `targetPlatforms`.
- Emulator screenshots: each new module in at least one cell; regression shots for
  basalt (2×2) and emery (3×3); chalk/gabbro round shots from 3b onward.
