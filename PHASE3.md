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
- [x] **3b — chalk + gabbro:** radial renderer behind `ROUND_LAYOUT` (center pod + ring of
      pods). Per-cell geometry now flows through `s_cell_frame[]`, so `scale_layout`/`sysfont`
      work for grid cells and round pods alike. Compact `MODULE_PODS[]` descriptors give
      Date/Weather/Stats/Calendar a two-line, icon-free form on ring pods; the center pod uses
      the full descriptor (Time). Pod count/sizing tuned per platform: chalk = center + 4,
      gabbro = center + 8. Both added to `targetPlatforms`; all 7 platforms build; screenshots
      confirm chalk (4 pods) and gabbro (8 pods, matches mockup C) plus basalt/emery regression.
      Round uses a plain white field (no per-cell background colors yet — a follow-up).
- [x] **3c — On-watch modules:** 12 new modules (enum 10–21) slotted into the registry —
      BT status (+ disconnect vibe), HR, step-goal ring, sleep, calories, active minutes,
      moon phase, day-of-year, count-up, custom text, Quiet Time, analog mini-clock.
  - [x] Registry extended for **custom-draw** modules: `ModuleDef.custom_draw` (+ optional
        `custom_frame`) generalizes the old Stats-divider pattern. A dedicated `Layer` is
        created via `layer_create_with_data` storing the cell index, so the proc recovers
        geometry/color/data. The goal ring (gray track + fg progress arc, step count
        centered) and analog mini-clock (face, ticks, hour/minute hands) use it; the other
        10 modules are pure TextLayers. Text-only modules leave `custom_draw` NULL — zero
        change to the existing 9.
  - [x] Data producers: connection service (BT), Health (HR/sleep/calories/active), and
        tick-driven moon/day-of-year/count-up/Quiet Time. New settings: BluetoothVibe,
        CountupDate/Label, CustomText, StepGoal (persist 155–159; message keys + Clay inputs).
  - [x] Compact `MODULE_PODS` variants for all 12; Clay dropdown grew to 21 options.
  - [x] Build all 7 platforms; screenshots basalt 2×2, emery 3×3, chalk (center+4),
        gabbro (center+8) for both the text wave and the ring/mini-clock wave.
  - [x] **Follow-up (from 3b): round per-cell background colors** — the round background
        proc now paints a rounded "chip" behind each pod whose background is enabled
        (center pod radius 10, ring pods radius 6); confirmed on chalk + gabbro.
  - [x] **Follow-up (from 3a): live module reassignment** — verified headlessly via
        `pebble send-app-message` (Quadrant*Module keys): cells swap module live through the
        `cell_build_ui` rebuild path with no restart.
- [x] **3d — Phone-data modules:** 7 new modules (enum 22–28) fed from the companion pkjs.
  - [x] **Weather family:** pkjs fetch switched from `current.json` to `forecast.json?days=1&aqi=yes`
        — one request now yields hi/lo, humidity, wind, UV, US-EPA AQI, and sunrise/sunset.
        New modules Sunrise/Sunset, Weather Hi/Lo, Humidity, Wind, UV, AQI. Hi/lo + wind travel
        in the unit matching the existing Celsius toggle (C labels via `s_use_celsius`);
        sunrise/sunset sent as minutes-since-midnight so the watch formats them with
        `clock_is_24h_style()`; AQI sent as the 1–6 index, mapped to a word on-watch.
  - [x] **ICS calendar:** new pkjs fetch + parser (line-unfolding, DTSTART/SUMMARY scan, TZID/Z
        handling, escape unfolding) picks the soonest upcoming `VEVENT` and feeds the existing
        `CalendarTitle`/`CalendarTime` keys. Clay gains a Calendar ICS URL input. Parser
        unit-tested in node; AppMessage→render confirmed in the emulator.
  - [x] **IANA TZ city picker:** the raw-offset dropdown is replaced by a 20-city IANA list;
        pkjs resolves the DST-correct offset with `Intl.DateTimeFormat` and pushes the existing
        `TZOffset` (recomputed on launch / save / 30-min tick). Offset math unit-tested in node
        across DST (incl. inverted southern-hemisphere). Note: the emulator's pypkjs may lack
        `Intl`; the code catches and skips gracefully, and real-phone JS engines support it.
  - [x] **Crypto ticker:** `MODULE_CRYPTO` fed by a keyless CoinGecko `simple/price` fetch;
        Clay inputs for CoinGecko id + short label; pkjs sends a preformatted "SYM 62.2k"
        string (persisted on-watch so it survives restart). Render confirmed via injection.
  - [x] New message keys: WeatherHigh/Low, Humidity, Wind, UV, AQI, SunriseMin/SunsetMin,
        CalendarUrl, TZCity, CryptoId/Label/Price (persist 160 for the last price).
        All 7 platforms build (aplite ~7.9 KB free heap).
- [x] **3e — Touch (emery + gabbro):** tap-driven detail overlays + tap-to-act.
  - [x] **Capability spike (done first):** verified `TouchService` is real — declared in
        `pebble.h` (`touch_service_subscribe`/`_unsubscribe`/`_is_enabled`, `TouchEvent` with
        type + **x/y**), `PBL_TOUCH` defined only for emery+gabbro, and the symbols are real
        `T` entries in those two `libpebble.a` (0 in basalt/aplite/chalk). Runtime: a
        watchface subscription returns `touch_service_is_enabled()==1` on both emery and
        gabbro emulators. (API gives coordinates, so hit-testing is exact — richer than the
        "tap-only" originally planned.)
  - [x] **Foundation:** subscribe under `#ifdef PBL_TOUCH`; a tap = Touchdown→Liftoff with
        little travel; `point_to_cell()` hit-tests (x,y) against `s_cell_frame[]`; `handle_tap()`
        is the shared entry point.
  - [x] **Detail overlay:** full-screen scrim + centered panel showing the tapped module's
        name (`MODULE_NAMES[]`) + primary value, auto-dismiss after 3 s, tap-again to close.
        Works on grid + round.
  - [x] **Tap-to-act:** phone-data modules (weather family, calendar, crypto, TZ) also fire
        `request_phone_refresh()` and the open overlay updates live when fresh data lands;
        Stats cycles Steps→Distance→Calories→Active on repeated taps.
  - [x] The whole subsystem is `#ifdef PBL_TOUCH`, so non-touch platforms carry none of it
        (aplite heap unchanged at ~7.9 KB). All 7 build.
  - [ ] **Pending real-tap check:** no CLI touchscreen injector exists (`emu-tap` is the
        accelerometer; `emu-control` needs a browser/VNC page), so an actual tap→`Touchdown`
        with coordinates was validated only via an injected `handle_tap`. Confirm a physical
        tap on the emulator GUI (VNC/local display) or emery/gabbro hardware before shipping.
- [x] **3f — Clay live preview + grouped options** (pkjs only; no C change).
  - [x] **Categorized dropdowns:** the 28 module options are grouped into Core / Clock &
        Calendar / Health / Weather & Sky / Utility / Finance. Clay 1.0.10's `select.tpl`
        renders an `<optgroup>` whenever an option's `value` is an array, so it's a pure
        data-shape change (Empty stays ungrouped at top).
  - [x] **Live preview** (`src/pkjs/preview.js`): a `clay.registerComponent` custom component
        (`watchPreview`) draws a canvas mock of the layout for the active platform — grid
        (2×2 / emery 3×3) or round (chalk center+4 / gabbro center+8) — simulating per-cell
        background colors, brightness-based auto text color, manual text color, and B&W
        dithering on the mono platforms (aplite/diorite/flint). Registered + wired via a
        `customFn` on `AFTER_BUILD`; redraws on any module/colour change.
  - [x] Verified headlessly as far as possible: pure layout/colour helpers unit-tested in
        node; `render()` smoke-tested against a mock 2-D context; `registerComponent` API
        conformance confirmed against the vendored Clay; and the drawing logic visually
        validated by SVG mocks (emery grid, gabbro round, flint B&W) generated from the same
        functions. All 7 platforms build; merged JS bundle valid.
  - [ ] **Pending browser check:** the actual canvas rendering + Clay DOM wiring
        (`item.$element.select`, change-event redraw) run only in the config webview, which
        can't be driven headlessly — confirm by opening the settings page on device/emulator.

**Phase 3 complete** (3a–3f). 29 module types across 7 platforms, touch on emery/gabbro,
Clay live preview. Remaining open item is the browser-side confirmation of 3e's real-tap
path and 3f's live-preview rendering.

## Verification (every phase)
- `pebble build` green for every platform in `targetPlatforms`.
- Emulator screenshots: each new module in at least one cell; regression shots for
  basalt (2×2) and emery (3×3); chalk/gabbro round shots from 3b onward.
