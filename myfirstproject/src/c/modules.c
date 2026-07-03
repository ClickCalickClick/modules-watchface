#include <pebble.h>
#include <ctype.h>
#include <string.h>
#include "config.h"

// Module types
typedef enum {
  MODULE_EMPTY = 0,
  MODULE_DATE = 1,
  MODULE_WEATHER = 2,
  MODULE_TIME = 3,
  MODULE_STATS = 4,
  MODULE_TZ = 5,         // Second time zone
  MODULE_WEEK = 6,       // ISO week number
  MODULE_COUNTDOWN = 7,  // Days until a target date
  MODULE_DISTANCE = 8,   // Distance walked today
  MODULE_CALENDAR = 9,   // Next calendar event (from phone)
  // Phase 3c on-watch wave (data sourced entirely on the watch, no phone needed)
  MODULE_BLUETOOTH = 10, // Bluetooth connection status
  MODULE_HEARTRATE = 11, // Current heart rate (BPM)
  MODULE_GOAL_RING = 12, // Step-goal progress ring (custom-draw)
  MODULE_SLEEP = 13,     // Sleep duration today
  MODULE_CALORIES = 14,  // Active + resting calories today
  MODULE_ACTIVE = 15,    // Active minutes today
  MODULE_MOON = 16,      // Moon phase
  MODULE_DAY_OF_YEAR = 17, // Day-of-year (1..366)
  MODULE_COUNTUP = 18,   // Days since a target date
  MODULE_CUSTOM_TEXT = 19, // User-supplied custom text
  MODULE_QUIET_TIME = 20,  // Quiet Time on/off
  MODULE_MINI_CLOCK = 21,  // Analog mini-clock (custom-draw)
  // Phase 3d phone-data wave (fed from the companion pkjs over AppMessage)
  MODULE_SUN = 22,         // Sunrise / sunset times
  MODULE_WEATHER_HL = 23,  // Today's forecast high / low
  MODULE_HUMIDITY = 24,    // Relative humidity
  MODULE_WIND = 25,        // Wind speed
  MODULE_UV = 26,          // UV index
  MODULE_AQI = 27,         // Air quality (US EPA index)
  MODULE_CRYPTO = 28,      // Crypto price ticker (one symbol per cell)
  NUM_MODULE_TYPES
} ModuleType;

// Cell layout per platform:
//  - Round platforms (chalk 180x180, gabbro 260x260) use a radial layout: one
//    center pod (cell 0) plus a ring of RING_PODS pods. No grid.
//  - emery (200x228) uses a 3x3 grid; the 144x168 rect platforms stay 2x2.
// MAX_CELLS sizes the per-cell arrays.
#if defined(PBL_ROUND)
  #define ROUND_LAYOUT 1
  // Radial layout tuning as percentages of screen size. The smaller 180x180
  // chalk fits 4 legible pods; gabbro's 260x260 fits 8. CENTER/RING/POD percents
  // are chosen so ring pods clear the center pod and the round edge.
  #if defined(PBL_PLATFORM_CHALK)
    #define RING_PODS 4
    #define CENTER_W_PCT 38
    #define CENTER_H_PCT 48
    #define RING_PCT   36
    #define POD_W_PCT  32
    #define POD_H_PCT  22
  #else
    #define RING_PODS 8   // gabbro 260x260
    #define CENTER_W_PCT 40
    #define CENTER_H_PCT 44
    #define RING_PCT   34
    #define POD_W_PCT  28
    #define POD_H_PCT  20
  #endif
  #define NUM_CELLS (1 + RING_PODS)
#elif defined(PBL_PLATFORM_EMERY)
  #define GRID_COLS 3
  #define GRID_ROWS 3
  #define NUM_CELLS (GRID_COLS * GRID_ROWS)
#else
  #define GRID_COLS 2
  #define GRID_ROWS 2
  #define NUM_CELLS (GRID_COLS * GRID_ROWS)
#endif
#define MAX_CELLS 9

// Base coordinate space the module layout templates are authored in (one 144x168 quadrant).
#define BASE_QUAD_W 72
#define BASE_QUAD_H 84

// Persistence keys. Per-cell settings use a base + cell-index scheme (cells 0..8).
#define PERSIST_MODULE_BASE     100  // 100..108
#define PERSIST_BG_BASE         110  // 110..118
#define PERSIST_COLOR_BASE      120  // 120..128
#define PERSIST_AUTOTEXT_BASE   130  // 130..138
#define PERSIST_TEXTCOLOR_BASE  140  // 140..148
// New module settings (scalar)
#define PERSIST_TZ_OFFSET       150  // minutes offset from local time
#define PERSIST_TZ_LABEL        151  // short city label string
#define PERSIST_COUNTDOWN_DATE  152  // target date as YYYYMMDD int
#define PERSIST_COUNTDOWN_LABEL 153  // short label string
#define PERSIST_DIST_UNITS      154  // 0 = miles, 1 = km
#define PERSIST_BT_VIBE         155  // 1 = vibrate on Bluetooth disconnect
#define PERSIST_COUNTUP_DATE    156  // start date as YYYYMMDD int
#define PERSIST_COUNTUP_LABEL   157  // short label string
#define PERSIST_CUSTOM_TEXT     158  // custom text string
#define PERSIST_STEP_GOAL       159  // daily step goal for the progress ring
#define PERSIST_CRYPTO_PRICE    160  // last crypto ticker string (from phone)

// UI Elements
static Window *s_main_window;
static Layer *s_window_layer;
static Layer *s_background_layer;

// Shared bitmaps (referenced by any cell showing the owning module)
static GBitmap *s_weather_icon;
static GBitmap *s_battery_icon;

// Data buffers. Module descriptors point text parts at these, so every cell
// showing a given module renders from the same source of truth.
static char s_day_name_buffer[4];
static char s_day_number_buffer[3];
static char s_month_name_buffer[4];
static char s_temperature_buffer[8] = "--\xC2\xB0";
static char s_hour_buffer[3];
static char s_minute_buffer[3];
static char s_battery_buffer[6];
static char s_steps_buffer[12];
static char s_weather_condition_buffer[16] = "--";
static char s_tz_time_buffer[8];
static char s_week_buffer[4];
static char s_cd_buffer[16];
static char s_dist_buffer[20];
static char s_cal_title_buffer[24] = "--";
static char s_cal_time_buffer[12];
// Phase 3c on-watch buffers
static char s_bt_buffer[6] = "--";
static char s_hr_buffer[6] = "--";
static char s_sleep_buffer[8] = "--";
static char s_calories_buffer[8] = "--";
static char s_active_buffer[8] = "--";
static char s_moon_buffer[8] = "--";
static char s_doy_buffer[6] = "--";
static char s_countup_buffer[16] = "--";
static char s_quiet_buffer[6] = "--";
static char s_custom_text[24] = "TEXT";
// Phase 3d phone-data buffers
static char s_sunrise_buffer[8] = "--";
static char s_sunset_buffer[8] = "--";
static char s_hl_buffer[12] = "--";
static char s_humidity_buffer[6] = "--";
static char s_wind_buffer[8] = "--";
static char s_uv_buffer[6] = "--";
static char s_aqi_buffer[8] = "--";
static char s_crypto_buffer[16] = "--";
static int s_sunrise_min = -1;  // minutes since midnight (-1 = unset)
static int s_sunset_min = -1;

// Settings
static bool s_use_celsius = false;
static int s_current_temperature = 0;
static bool s_has_temperature = false;

// Module assignments for each cell (cells 0..MAX_CELLS-1; only NUM_CELLS used).
#ifdef ROUND_LAYOUT
// Round: cell 0 is the center pod (Time); cells 1.. are the ring, clockwise from top.
static ModuleType s_quadrant_modules[MAX_CELLS] = {
  MODULE_TIME,      // cell 0 (center)
  MODULE_DATE,      // ring...
  MODULE_WEATHER,
  MODULE_WEEK,
  MODULE_DISTANCE,
  MODULE_STATS,
  MODULE_COUNTDOWN,
  MODULE_CALENDAR,
  MODULE_TZ
};
#else
// Grid: cells 5-9 default to the new module types so emery's 3x3 is fully populated.
static ModuleType s_quadrant_modules[MAX_CELLS] = {
  MODULE_DATE,      // cell 0
  MODULE_WEATHER,   // cell 1
  MODULE_TIME,      // cell 2
  MODULE_STATS,     // cell 3
  MODULE_TZ,        // cell 4 (emery)
  MODULE_WEEK,      // cell 5 (emery)
  MODULE_COUNTDOWN, // cell 6 (emery)
  MODULE_DISTANCE,  // cell 7 (emery)
  MODULE_CALENDAR   // cell 8 (emery)
};
#endif

// Background state for each cell (true = light gray/color, false = white)
static bool s_quadrant_backgrounds[MAX_CELLS] = {
  false, true, true, false,        // cells 0-3 (original defaults)
  true, false, false, true, false  // cells 4-8
};

#ifdef PBL_COLOR
// Color values for each cell (stored as 32-bit integer in ARGB8 format)
static GColor s_quadrant_colors[MAX_CELLS];
#endif

// Auto text color enabled for each cell (true = auto, false = manual)
static bool s_auto_text_color[MAX_CELLS] = {
  true, true, true, true, true, true, true, true, true
};

// Custom text color for each cell (when auto is disabled)
static GColor s_custom_text_color[MAX_CELLS];

// New-module settings
static int s_tz_offset_minutes = 0;            // second time zone offset from local
static char s_tz_label[8] = "TZ2";
static int s_countdown_date = 0;               // target as YYYYMMDD (0 = unset)
static char s_countdown_label[10] = "EVENT";
static bool s_dist_use_km = true;              // true = km, false = miles
static bool s_bt_vibe = false;                 // vibrate on Bluetooth disconnect
static int s_countup_date = 0;                 // start date as YYYYMMDD (0 = unset)
static char s_countup_label[10] = "SINCE";
static int s_step_goal = 10000;                // daily step goal for the ring
static int s_step_count = 0;                   // today's steps (for the ring arc)

// Runtime screen geometry, computed once from the actual display bounds so the
// layout adapts to any platform (e.g. emery 200x228) instead of static 144x168.
static int16_t s_screen_w, s_screen_h, s_quad_w, s_quad_h;
// Absolute on-screen rect for each cell. Grid platforms fill these as a uniform
// grid; round platforms fill them as a center pod + a ring of smaller pods.
// All layout math scales the BASE_QUAD templates into these rects, so the same
// module descriptors render on both grid and round.
static GRect s_cell_frame[MAX_CELLS];

static void compute_screen_geometry(Window *window) {
  GRect b = layer_get_bounds(window_get_root_layer(window));
  s_screen_w = b.size.w;
  s_screen_h = b.size.h;

#ifdef ROUND_LAYOUT
  int16_t cx = s_screen_w / 2, cy = s_screen_h / 2;
  // Center pod (cell 0): a square-ish box centered on screen.
  int16_t cw = (s_screen_w * CENTER_W_PCT) / 100;
  int16_t ch = (s_screen_h * CENTER_H_PCT) / 100;
  s_cell_frame[0] = GRect(cx - cw / 2, cy - ch / 2, cw, ch);
  // Ring pods: evenly spaced on a circle, starting at the top (12 o'clock).
  int16_t ring_r = (s_screen_w * RING_PCT) / 100;
  int16_t pw = (s_screen_w * POD_W_PCT) / 100;
  int16_t ph = (s_screen_h * POD_H_PCT) / 100;
  for (int i = 0; i < RING_PODS; i++) {
    int32_t angle = (TRIG_MAX_ANGLE * i) / RING_PODS - TRIG_MAX_ANGLE / 4;
    int16_t px = cx + (ring_r * cos_lookup(angle)) / TRIG_MAX_RATIO;
    int16_t py = cy + (ring_r * sin_lookup(angle)) / TRIG_MAX_RATIO;
    s_cell_frame[i + 1] = GRect(px - pw / 2, py - ph / 2, pw, ph);
  }
  s_quad_w = cw;
  s_quad_h = ch;
#else
  s_quad_w = s_screen_w / GRID_COLS;
  s_quad_h = s_screen_h / GRID_ROWS;
  for (int i = 0; i < NUM_CELLS; i++) {
    s_cell_frame[i] = GRect((i % GRID_COLS) * s_quad_w, (i / GRID_COLS) * s_quad_h,
                            s_quad_w, s_quad_h);
  }
#endif
}

// Scale a base-coordinate GRect (authored for BASE_QUAD_W x BASE_QUAD_H) into the
// given cell's on-screen frame. Integer multiply-before-divide keeps precision
// without floats.
static GRect scale_layout(GRect base, int cell) {
  GRect f = s_cell_frame[cell];
  return GRect(
    f.origin.x + (base.origin.x * f.size.w) / BASE_QUAD_W,
    f.origin.y + (base.origin.y * f.size.h) / BASE_QUAD_H,
    (base.size.w * f.size.w) / BASE_QUAD_W,
    (base.size.h * f.size.h) / BASE_QUAD_H);
}

// Position a bitmap icon within the cell's scaled frame. bitmap_layer does not
// upscale image data, so the layer is sized to the loaded bitmap's native pixels
// (the SDK selects a larger ~emery asset automatically); only the position is scaled.
static GRect place_icon(GRect base, int cell, GBitmap *bmp, bool center_x) {
  GRect scaled = scale_layout(base, cell);
  if (bmp) {
    GSize sz = gbitmap_get_bounds(bmp).size;
    scaled.size = sz;
    if (center_x) {
      scaled.origin.x = s_cell_frame[cell].origin.x + (s_cell_frame[cell].size.w - sz.w) / 2;
    }
  }
  return scaled;
}

// Font roles, resolved to system fonts per cell size.
typedef enum {
  ROLE_LABEL_SM,
  ROLE_LABEL_MD,
  ROLE_VALUE_MD,
  ROLE_VALUE_LG,
  ROLE_HERO,
} FontRole;

static GFont sysfont(FontRole role, int16_t h) {
  // Pick a font tier from the available CELL height, not the screen size: emery's
  // 3x3 cells (~76px) are smaller than the 2x2 cells (84px / 114px), and round
  // ring pods (~43px) are smaller still, so big screens can still need small
  // fonts. Buckets: LARGE (>=100), MED (>=84), SMALL (<84).
  if (h >= 100) {            // large cell (e.g. emery 2x2)
    switch (role) {
      case ROLE_LABEL_SM: return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
      case ROLE_LABEL_MD: return fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
      case ROLE_VALUE_MD: return fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
      case ROLE_VALUE_LG: return fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
      case ROLE_HERO:     return fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
    }
  } else if (h >= 84) {      // medium cell (144x168 2x2) — original sizes
    switch (role) {
      case ROLE_LABEL_SM: return fonts_get_system_font(FONT_KEY_GOTHIC_14);
      case ROLE_LABEL_MD: return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
      case ROLE_VALUE_MD: return fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
      case ROLE_VALUE_LG: return fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
      case ROLE_HERO:     return fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
    }
  } else {                   // small cell (emery 3x3 ~76px)
    switch (role) {
      case ROLE_LABEL_SM: return fonts_get_system_font(FONT_KEY_GOTHIC_14);
      case ROLE_LABEL_MD: return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
      case ROLE_VALUE_MD: return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
      case ROLE_VALUE_LG: return fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
      case ROLE_HERO:     return fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    }
  }
  return fonts_get_system_font(FONT_KEY_GOTHIC_14);
}

#ifdef PBL_COLOR
static inline uint8_t prv_expand_component(uint8_t value) {
  return value * 85; // Map 2-bit component (0-3) to 0-255 range
}

static uint8_t prv_calculate_brightness(GColor color) {
  uint8_t red = prv_expand_component((color.argb >> 4) & 0x3);
  uint8_t green = prv_expand_component((color.argb >> 2) & 0x3);
  uint8_t blue = prv_expand_component(color.argb & 0x3);
  return (uint8_t)((red * 299 + green * 587 + blue * 114) / 1000);
}
#endif

// Determine text color for a cell based on background and settings
static GColor get_text_color_for_quadrant(int quadrant) {
  if (!s_auto_text_color[quadrant]) {
    return s_custom_text_color[quadrant];
  }
#ifdef PBL_COLOR
  if (s_quadrant_backgrounds[quadrant] &&
      prv_calculate_brightness(s_quadrant_colors[quadrant]) < 128) {
    return GColorWhite;  // Dark background, use white text
  }
#endif
  return GColorBlack;
}

// ---------------------------------------------------------------------------
// Custom-draw update procs. Modules that can't be expressed with TextLayers
// point ModuleDef.custom_draw at one of these. The layer stores its cell index
// as layer data (see cell_build_ui), so the proc can recover cell geometry,
// per-cell text color, and read the shared data globals.
// ---------------------------------------------------------------------------

// Step-goal progress ring: a gray track with a foreground arc for progress.
static void goal_ring_update_proc(Layer *layer, GContext *ctx) {
  int cell = *(int *)layer_get_data(layer);
  GRect b = layer_get_bounds(layer);
  GPoint center = GPoint(b.size.w / 2, b.size.h / 2);
  int16_t outer = (b.size.w < b.size.h ? b.size.w : b.size.h) / 2 - 2;
  int16_t thick = outer / 5;
  if (thick < 3) thick = 3;
  GRect box = GRect(center.x - outer, center.y - outer, outer * 2, outer * 2);

  // Track in dark gray so it contrasts on white and light-gray cell backgrounds.
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thick, 0, TRIG_MAX_ANGLE);

  int pct = s_step_goal > 0 ? (s_step_count * 100 / s_step_goal) : 0;
  if (pct > 100) pct = 100;
  int32_t end = (TRIG_MAX_ANGLE * pct) / 100;
  graphics_context_set_fill_color(ctx, get_text_color_for_quadrant(cell));
  graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thick, 0, end);
}

// Analog mini-clock: face outline, hour ticks, and hour/minute hands.
static void mini_clock_update_proc(Layer *layer, GContext *ctx) {
  int cell = *(int *)layer_get_data(layer);
  GRect b = layer_get_bounds(layer);
  GColor fg = get_text_color_for_quadrant(cell);
  GPoint center = GPoint(b.size.w / 2, b.size.h / 2);
  int16_t r = (b.size.w < b.size.h ? b.size.w : b.size.h) / 2 - 2;

  graphics_context_set_stroke_color(ctx, fg);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, center, r);

  // Hour ticks (12 o'clock is straight up; angle grows clockwise).
  for (int i = 0; i < 12; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 12;
    int16_t sx = center.x + (int16_t)((r - 3) * sin_lookup(a) / TRIG_MAX_RATIO);
    int16_t sy = center.y - (int16_t)((r - 3) * cos_lookup(a) / TRIG_MAX_RATIO);
    int16_t ex = center.x + (int16_t)(r * sin_lookup(a) / TRIG_MAX_RATIO);
    int16_t ey = center.y - (int16_t)(r * cos_lookup(a) / TRIG_MAX_RATIO);
    graphics_draw_line(ctx, GPoint(sx, sy), GPoint(ex, ey));
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t min_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  int32_t hour_angle = TRIG_MAX_ANGLE * ((t->tm_hour % 12) * 60 + t->tm_min) / 720;

  int16_t hr_len = r / 2;
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, center, GPoint(
    center.x + (int16_t)(hr_len * sin_lookup(hour_angle) / TRIG_MAX_RATIO),
    center.y - (int16_t)(hr_len * cos_lookup(hour_angle) / TRIG_MAX_RATIO)));

  int16_t min_len = r - 4;
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, center, GPoint(
    center.x + (int16_t)(min_len * sin_lookup(min_angle) / TRIG_MAX_RATIO),
    center.y - (int16_t)(min_len * cos_lookup(min_angle) / TRIG_MAX_RATIO)));
}

// ---------------------------------------------------------------------------
// Module registry: one descriptor per module type. Each cell displaying a
// module gets its own lazily-created layers, positioned from these templates
// (authored in BASE_QUAD space) and fed from the shared data buffers.
// Adding a module type = add an enum value, a descriptor, and a data producer.
// ---------------------------------------------------------------------------

#define MAX_TEXT_PARTS 3

typedef struct {
  GRect frame;            // BASE_QUAD space
  FontRole role;          // normal font role
  FontRole role_bw_bg;    // role when a B&W platform has a background enabled
  GTextAlignment align;
  const char *src;        // static label or shared data buffer
} TextPart;

typedef struct {
  uint8_t num_texts;
  TextPart texts[MAX_TEXT_PARTS];
  GBitmap **icon;         // shared bitmap slot, or NULL
  GRect icon_frame;       // BASE_QUAD space
  bool icon_center_x;
  bool has_divider;       // horizontal rule (Stats)
  // Optional custom drawing: modules whose visuals can't be expressed as
  // TextLayers (progress ring, analog clock) set custom_draw. A dedicated Layer
  // is created at custom_frame (BASE_QUAD space; {0,0} size falls back to the
  // whole cell) and this proc is invoked to paint it. The proc recovers its cell
  // index from the layer's data (see cell_build_ui) to read geometry + data.
  void (*custom_draw)(Layer *layer, GContext *ctx);
  GRect custom_frame;     // BASE_QUAD space; zero size => full cell
} ModuleDef;

static const ModuleDef MODULE_DEFS[NUM_MODULE_TYPES] = {
  [MODULE_EMPTY] = { .num_texts = 0 },
  [MODULE_DATE] = {
    .num_texts = 3,
    .texts = {
      {{{0, 0},  {72, 20}}, ROLE_LABEL_SM, ROLE_LABEL_MD, GTextAlignmentCenter, s_day_name_buffer},
      {{{0, 15}, {72, 66}}, ROLE_HERO,     ROLE_HERO,     GTextAlignmentCenter, s_day_number_buffer},
      {{{0, 63}, {72, 77}}, ROLE_LABEL_SM, ROLE_LABEL_MD, GTextAlignmentCenter, s_month_name_buffer},
    },
  },
  [MODULE_WEATHER] = {
    .num_texts = 2,
    .texts = {
      {{{0, 30}, {72, 58}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_temperature_buffer},
      {{{0, 55}, {72, 79}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_weather_condition_buffer},
    },
    .icon = &s_weather_icon,
    .icon_frame = {{22, 2}, {28, 30}},
    .icon_center_x = true,
  },
  [MODULE_TIME] = {
    .num_texts = 2,
    .texts = {
      {{{0, 0},  {72, 42}}, ROLE_HERO, ROLE_HERO, GTextAlignmentCenter, s_hour_buffer},
      {{{0, 34}, {72, 70}}, ROLE_HERO, ROLE_HERO, GTextAlignmentCenter, s_minute_buffer},
    },
  },
  [MODULE_STATS] = {
    .num_texts = 3,
    .texts = {
      {{{32, 7}, {40, 29}}, ROLE_LABEL_MD, ROLE_LABEL_MD, GTextAlignmentLeft,   s_battery_buffer},
      {{{0, 34}, {72, 61}}, ROLE_VALUE_MD, ROLE_VALUE_LG, GTextAlignmentCenter, s_steps_buffer},
      {{{0, 60}, {72, 76}}, ROLE_LABEL_SM, ROLE_LABEL_MD, GTextAlignmentCenter, "STEPS"},
    },
    .icon = &s_battery_icon,
    .icon_frame = {{14, 8}, {16, 24}},
    .has_divider = true,
  },
  [MODULE_TZ] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 24}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_tz_label},
      {{{0, 26}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_tz_time_buffer},
    },
  },
  [MODULE_WEEK] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "WEEK"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_week_buffer},
    },
  },
  [MODULE_COUNTDOWN] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_countdown_label},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_cd_buffer},
    },
  },
  [MODULE_DISTANCE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "DIST"},
      {{{0, 30}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_dist_buffer},
    },
  },
  [MODULE_CALENDAR] = {
    .num_texts = 3,
    .texts = {
      {{{0, 4},  {72, 22}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "NEXT"},
      {{{0, 24}, {72, 54}}, ROLE_LABEL_MD, ROLE_LABEL_MD, GTextAlignmentCenter, s_cal_title_buffer},
      {{{0, 54}, {72, 80}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_cal_time_buffer},
    },
  },
  // --- Phase 3c on-watch modules ---
  [MODULE_BLUETOOTH] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "BLUETOOTH"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_bt_buffer},
    },
  },
  [MODULE_HEARTRATE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "HEART"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_hr_buffer},
    },
  },
  [MODULE_SLEEP] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "SLEEP"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_sleep_buffer},
    },
  },
  [MODULE_CALORIES] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "CAL"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_calories_buffer},
    },
  },
  [MODULE_ACTIVE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "ACTIVE"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_active_buffer},
    },
  },
  [MODULE_MOON] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "MOON"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_moon_buffer},
    },
  },
  [MODULE_DAY_OF_YEAR] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "DAY"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_doy_buffer},
    },
  },
  [MODULE_COUNTUP] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_countup_label},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_countup_buffer},
    },
  },
  [MODULE_CUSTOM_TEXT] = {
    .num_texts = 1,
    .texts = {
      {{{2, 24}, {68, 78}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_custom_text},
    },
  },
  [MODULE_QUIET_TIME] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "QUIET"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_quiet_buffer},
    },
  },
  [MODULE_GOAL_RING] = {
    // Progress ring (custom-draw) with the step count centered inside it.
    .num_texts = 1,
    .texts = {
      {{{0, 32}, {72, 56}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_steps_buffer},
    },
    .custom_draw = goal_ring_update_proc,
  },
  [MODULE_MINI_CLOCK] = {
    .num_texts = 0,
    .custom_draw = mini_clock_update_proc,
  },
  // --- Phase 3d phone-data modules ---
  [MODULE_SUN] = {
    // Two value lines; each buffer carries an R/S prefix built on the watch.
    .num_texts = 2,
    .texts = {
      {{{0, 12}, {72, 44}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_sunrise_buffer},
      {{{0, 44}, {72, 76}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_sunset_buffer},
    },
  },
  [MODULE_WEATHER_HL] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "HI / LO"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_hl_buffer},
    },
  },
  [MODULE_HUMIDITY] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "HUMIDITY"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_humidity_buffer},
    },
  },
  [MODULE_WIND] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "WIND"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_wind_buffer},
    },
  },
  [MODULE_UV] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "UV INDEX"},
      {{{0, 28}, {72, 80}}, ROLE_HERO, ROLE_HERO, GTextAlignmentCenter, s_uv_buffer},
    },
  },
  [MODULE_AQI] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "AIR"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_aqi_buffer},
    },
  },
  [MODULE_CRYPTO] = {
    .num_texts = 1,
    .texts = {
      {{{2, 24}, {68, 78}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_crypto_buffer},
    },
  },
};

#ifdef PBL_TOUCH
// Human-readable module names, used by the tap detail overlay (Phase 3e).
static const char *const MODULE_NAMES[NUM_MODULE_TYPES] = {
  [MODULE_EMPTY] = "EMPTY",
  [MODULE_DATE] = "DATE",
  [MODULE_WEATHER] = "WEATHER",
  [MODULE_TIME] = "TIME",
  [MODULE_STATS] = "STATS",
  [MODULE_TZ] = "TIME ZONE",
  [MODULE_WEEK] = "WEEK",
  [MODULE_COUNTDOWN] = "COUNTDOWN",
  [MODULE_DISTANCE] = "DISTANCE",
  [MODULE_CALENDAR] = "NEXT EVENT",
  [MODULE_BLUETOOTH] = "BLUETOOTH",
  [MODULE_HEARTRATE] = "HEART RATE",
  [MODULE_GOAL_RING] = "STEP GOAL",
  [MODULE_SLEEP] = "SLEEP",
  [MODULE_CALORIES] = "CALORIES",
  [MODULE_ACTIVE] = "ACTIVE",
  [MODULE_MOON] = "MOON PHASE",
  [MODULE_DAY_OF_YEAR] = "DAY OF YEAR",
  [MODULE_COUNTUP] = "COUNT-UP",
  [MODULE_CUSTOM_TEXT] = "TEXT",
  [MODULE_QUIET_TIME] = "QUIET TIME",
  [MODULE_MINI_CLOCK] = "CLOCK",
  [MODULE_SUN] = "SUN",
  [MODULE_WEATHER_HL] = "HIGH / LOW",
  [MODULE_HUMIDITY] = "HUMIDITY",
  [MODULE_WIND] = "WIND",
  [MODULE_UV] = "UV INDEX",
  [MODULE_AQI] = "AIR QUALITY",
  [MODULE_CRYPTO] = "CRYPTO",
};
#endif  // PBL_TOUCH

#ifdef ROUND_LAYOUT
// Compact descriptors for round ring pods, which are much smaller than grid
// cells. Multi-part modules drop to a two-line label+value form and shed icons;
// the already-compact modules (TZ/Week/Countdown/Distance) reuse their grid
// layout. The center pod (cell 0) uses the full MODULE_DEFS entry, not these.
static const ModuleDef MODULE_PODS[NUM_MODULE_TYPES] = {
  [MODULE_EMPTY] = { .num_texts = 0 },
  [MODULE_DATE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 4},  {72, 30}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_day_name_buffer},
      {{{0, 28}, {72, 82}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_day_number_buffer},
    },
  },
  [MODULE_WEATHER] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 44}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_temperature_buffer},
      {{{0, 46}, {72, 80}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_weather_condition_buffer},
    },
  },
  [MODULE_TIME] = {
    .num_texts = 2,
    .texts = {
      {{{0, 2},  {72, 42}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_hour_buffer},
      {{{0, 40}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_minute_buffer},
    },
  },
  [MODULE_STATS] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 44}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_battery_buffer},
      {{{0, 46}, {72, 80}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_steps_buffer},
    },
  },
  [MODULE_TZ] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 24}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_tz_label},
      {{{0, 26}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_tz_time_buffer},
    },
  },
  [MODULE_WEEK] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "WEEK"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_week_buffer},
    },
  },
  [MODULE_COUNTDOWN] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_countdown_label},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_cd_buffer},
    },
  },
  [MODULE_DISTANCE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 28}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "DIST"},
      {{{0, 30}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_dist_buffer},
    },
  },
  [MODULE_CALENDAR] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 44}}, ROLE_LABEL_MD, ROLE_LABEL_MD, GTextAlignmentCenter, s_cal_title_buffer},
      {{{0, 46}, {72, 80}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_cal_time_buffer},
    },
  },
  // --- Phase 3c on-watch modules (compact ring-pod form) ---
  [MODULE_BLUETOOTH] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "BT"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_bt_buffer},
    },
  },
  [MODULE_HEARTRATE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "HR"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_hr_buffer},
    },
  },
  [MODULE_SLEEP] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "SLEEP"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_sleep_buffer},
    },
  },
  [MODULE_CALORIES] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "CAL"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_calories_buffer},
    },
  },
  [MODULE_ACTIVE] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "ACTIVE"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_active_buffer},
    },
  },
  [MODULE_MOON] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "MOON"},
      {{{0, 28}, {72, 80}}, ROLE_LABEL_MD, ROLE_LABEL_MD, GTextAlignmentCenter, s_moon_buffer},
    },
  },
  [MODULE_DAY_OF_YEAR] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "DAY"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_doy_buffer},
    },
  },
  [MODULE_COUNTUP] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_countup_label},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_countup_buffer},
    },
  },
  [MODULE_CUSTOM_TEXT] = {
    .num_texts = 1,
    .texts = {
      {{{2, 20}, {68, 78}}, ROLE_LABEL_MD, ROLE_LABEL_MD, GTextAlignmentCenter, s_custom_text},
    },
  },
  [MODULE_QUIET_TIME] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "QUIET"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_quiet_buffer},
    },
  },
  [MODULE_GOAL_RING] = {
    // Compact ring keeps the arc; step count shrinks to a small centered label.
    .num_texts = 1,
    .texts = {
      {{{0, 32}, {72, 52}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, s_steps_buffer},
    },
    .custom_draw = goal_ring_update_proc,
  },
  [MODULE_MINI_CLOCK] = {
    .num_texts = 0,
    .custom_draw = mini_clock_update_proc,
  },
  // --- Phase 3d phone-data modules (compact ring-pod form) ---
  [MODULE_SUN] = {
    .num_texts = 2,
    .texts = {
      {{{0, 8},  {72, 44}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_sunrise_buffer},
      {{{0, 44}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_sunset_buffer},
    },
  },
  [MODULE_WEATHER_HL] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "HI/LO"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_hl_buffer},
    },
  },
  [MODULE_HUMIDITY] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "HUM"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_humidity_buffer},
    },
  },
  [MODULE_WIND] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "WIND"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_wind_buffer},
    },
  },
  [MODULE_UV] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "UV"},
      {{{0, 26}, {72, 80}}, ROLE_VALUE_LG, ROLE_VALUE_LG, GTextAlignmentCenter, s_uv_buffer},
    },
  },
  [MODULE_AQI] = {
    .num_texts = 2,
    .texts = {
      {{{0, 6},  {72, 26}}, ROLE_LABEL_SM, ROLE_LABEL_SM, GTextAlignmentCenter, "AIR"},
      {{{0, 28}, {72, 80}}, ROLE_VALUE_MD, ROLE_VALUE_MD, GTextAlignmentCenter, s_aqi_buffer},
    },
  },
  [MODULE_CRYPTO] = {
    .num_texts = 1,
    .texts = {
      {{{2, 20}, {68, 78}}, ROLE_LABEL_MD, ROLE_LABEL_MD, GTextAlignmentCenter, s_crypto_buffer},
    },
  },
};
#endif

// Descriptor to use for a cell. On round, ring pods (cell > 0) use the compact
// MODULE_PODS table; the center pod and all grid cells use the full MODULE_DEFS.
static const ModuleDef *def_for_cell(int cell) {
#ifdef ROUND_LAYOUT
  if (cell > 0) {
    return &MODULE_PODS[s_quadrant_modules[cell]];
  }
#endif
  return &MODULE_DEFS[s_quadrant_modules[cell]];
}

// Per-cell UI: layers exist only while a module needing them is assigned.
typedef struct {
  TextLayer *text[MAX_TEXT_PARTS];
  BitmapLayer *icon;
  Layer *divider;
  Layer *custom;   // optional custom-draw layer (progress ring, mini-clock)
} CellUI;
static CellUI s_cell_ui[MAX_CELLS];

static ModuleType sanitize_module(int value) {
  return (value > MODULE_EMPTY && value < NUM_MODULE_TYPES) ? (ModuleType)value : MODULE_EMPTY;
}

// Background layer update procedure
static void background_layer_update_proc(Layer *layer, GContext *ctx) {
#ifdef ROUND_LAYOUT
  // Round: white field, then a rounded background "chip" behind each pod whose
  // background is enabled (center pod gets a larger corner radius than the ring
  // pods). Pods without a background stay on the white field.
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  for (int q = 0; q < NUM_CELLS; q++) {
    if (!s_quadrant_backgrounds[q]) continue;
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, s_quadrant_colors[q]);
#else
    graphics_context_set_fill_color(ctx, GColorLightGray);
#endif
    graphics_fill_rect(ctx, s_cell_frame[q], (q == 0) ? 10 : 6, GCornersAll);
  }

  // Subtle outline around the center pod for a consistent focal point.
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, s_cell_frame[0], 10);
#else
  // Grid: fill each cell's background, then draw the separating grid lines.
  for (int q = 0; q < NUM_CELLS; q++) {
    GRect f = s_cell_frame[q];
    GColor color;

#ifdef PBL_COLOR
    // On color platforms, use custom color if background is enabled
    color = s_quadrant_backgrounds[q] ? s_quadrant_colors[q] : GColorWhite;
#else
    // On B&W platforms, use light gray or white
    color = s_quadrant_backgrounds[q] ? GColorLightGray : GColorWhite;
#endif

    // Fill the right column / bottom row to the screen edge to avoid seams on
    // displays whose dimensions don't divide evenly by the grid.
    int16_t w = (f.origin.x + s_quad_w >= s_screen_w) ? (s_screen_w - f.origin.x) : s_quad_w;
    int16_t h = (f.origin.y + s_quad_h >= s_screen_h) ? (s_screen_h - f.origin.y) : s_quad_h;
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(f.origin.x, f.origin.y, w, h), 0, GCornerNone);
  }

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  for (int c = 1; c < GRID_COLS; c++) {
    graphics_draw_line(ctx, GPoint(c * s_quad_w, 0), GPoint(c * s_quad_w, s_screen_h));
  }
  for (int r = 1; r < GRID_ROWS; r++) {
    graphics_draw_line(ctx, GPoint(0, r * s_quad_h), GPoint(s_screen_w, r * s_quad_h));
  }
#endif
}

// Divider line for the Stats module, scaled within the layer's own bounds.
static void divider_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  // Scale the base (6,32)->(66,32) line (authored in BASE_QUAD space) to the cell.
  int16_t x0 = (6 * b.size.w) / BASE_QUAD_W;
  int16_t x1 = (66 * b.size.w) / BASE_QUAD_W;
  int16_t y = (32 * b.size.h) / BASE_QUAD_H;
  graphics_draw_line(ctx, GPoint(x0, y), GPoint(x1, y));
}

// Font for a text part: B&W platforms bump certain roles when the cell has a
// (dithered gray) background, for legibility.
static GFont part_font(const TextPart *part, int cell) {
  int16_t h = s_cell_frame[cell].size.h;
#ifndef PBL_COLOR
  if (s_quadrant_backgrounds[cell]) {
    return sysfont(part->role_bw_bg, h);
  }
#endif
  return sysfont(part->role, h);
}

// Push current data into an existing cell's layers: text pointers, icon bitmap
// (which may have been recreated since the layer was built), and text colors.
static void cell_render(int cell) {
  const ModuleDef *def = def_for_cell(cell);
  CellUI *ui = &s_cell_ui[cell];
  GColor text_color = get_text_color_for_quadrant(cell);

  for (int i = 0; i < def->num_texts; i++) {
    if (ui->text[i]) {
      text_layer_set_text(ui->text[i], def->texts[i].src);
      text_layer_set_text_color(ui->text[i], text_color);
    }
  }
  if (def->icon && ui->icon) {
    bitmap_layer_set_bitmap(ui->icon, *def->icon);
    layer_set_frame(bitmap_layer_get_layer(ui->icon),
      place_icon(def->icon_frame, cell, *def->icon, def->icon_center_x));
  }
  // Custom-draw modules repaint on every data push (steps/goal, time).
  if (ui->custom) {
    layer_mark_dirty(ui->custom);
  }
}

// Destroy a cell's layers (safe on an already-empty cell).
static void cell_destroy_ui(int cell) {
  CellUI *ui = &s_cell_ui[cell];
  for (int i = 0; i < MAX_TEXT_PARTS; i++) {
    if (ui->text[i]) {
      text_layer_destroy(ui->text[i]);
      ui->text[i] = NULL;
    }
  }
  if (ui->icon) {
    bitmap_layer_destroy(ui->icon);
    ui->icon = NULL;
  }
  if (ui->divider) {
    layer_destroy(ui->divider);
    ui->divider = NULL;
  }
  if (ui->custom) {
    layer_destroy(ui->custom);
    ui->custom = NULL;
  }
}

// (Re)build a cell's layers for its currently assigned module, then render it.
static void cell_build_ui(int cell) {
  cell_destroy_ui(cell);

  const ModuleDef *def = def_for_cell(cell);
  CellUI *ui = &s_cell_ui[cell];

  // Custom-draw layer (added first so TextLayers render on top of it). The layer
  // stores its cell index as data so the update proc can recover geometry/color.
  if (def->custom_draw) {
    GRect frame = (def->custom_frame.size.w == 0 && def->custom_frame.size.h == 0)
      ? s_cell_frame[cell] : scale_layout(def->custom_frame, cell);
    Layer *cl = layer_create_with_data(frame, sizeof(int));
    *(int *)layer_get_data(cl) = cell;
    layer_set_update_proc(cl, def->custom_draw);
    layer_add_child(s_window_layer, cl);
    ui->custom = cl;
  }

  for (int i = 0; i < def->num_texts; i++) {
    const TextPart *part = &def->texts[i];
    TextLayer *tl = text_layer_create(scale_layout(part->frame, cell));
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_font(tl, part_font(part, cell));
    text_layer_set_text_alignment(tl, part->align);
    layer_add_child(s_window_layer, text_layer_get_layer(tl));
    ui->text[i] = tl;
  }

  if (def->icon) {
    BitmapLayer *bl = bitmap_layer_create(
      place_icon(def->icon_frame, cell, *def->icon, def->icon_center_x));
    bitmap_layer_set_background_color(bl, GColorClear);
    bitmap_layer_set_compositing_mode(bl, GCompOpSet);
    layer_add_child(s_window_layer, bitmap_layer_get_layer(bl));
    ui->icon = bl;
  }

  if (def->has_divider) {
    // Divider spans the STATS cell; draws a rule scaled within its own bounds.
    ui->divider = layer_create(s_cell_frame[cell]);
    layer_set_update_proc(ui->divider, divider_layer_update_proc);
    layer_add_child(s_window_layer, ui->divider);
  }

  cell_render(cell);
}

// Re-render every cell currently showing the given module (no-op if unassigned).
#ifdef PBL_TOUCH
// Refresh the detail overlay in place if it is currently showing this module
// (defined with the touch/overlay code below).
static void overlay_note_render(ModuleType module);
#endif

static void render_module(ModuleType module) {
  if (!s_window_layer) {
    return;  // window not loaded yet; cell_build_ui will render initial state
  }
  for (int q = 0; q < NUM_CELLS; q++) {
    if (s_quadrant_modules[q] == module) {
      cell_render(q);
    }
  }
#ifdef PBL_TOUCH
  overlay_note_render(module);
#endif
}

// Ask the phone to refetch everything (weather, calendar, crypto, TZ). The pkjs
// 'appmessage' handler keys off the Temperature request, same as the 30-min tick.
// Used by the tick handler and (on touch platforms) by tap-to-act.
static void request_phone_refresh() {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_Temperature, 1);
    app_message_outbox_send();
  }
}

// ---------------------------------------------------------------------------
// Tap interaction (Phase 3e): a TouchService tap maps to a cell, which pops a
// temporary detail overlay for that cell's module. The whole subsystem is
// PBL_TOUCH-only (emery + gabbro) — non-touch platforms carry none of it, and
// don't pay the full-screen overlay layer's RAM.
// ---------------------------------------------------------------------------
#ifdef PBL_TOUCH

static Layer *s_overlay_layer;      // full-screen; draws only while shown
static int s_overlay_cell = -1;     // cell whose module is shown, -1 = hidden
static AppTimer *s_overlay_timer;   // auto-dismiss timer
static char s_ov_title[16];
static char s_ov_value[24];

// Stats tap-to-act: repeated taps cycle the overlay through these metrics.
#define STATS_CYCLE_COUNT 4
static int s_stats_cycle = 0;       // 0 steps, 1 distance, 2 calories, 3 active

// Modules whose data comes from the phone; tapping one requests a fresh pull.
static bool is_phone_data_module(ModuleType m) {
  switch (m) {
    case MODULE_WEATHER: case MODULE_SUN: case MODULE_WEATHER_HL:
    case MODULE_HUMIDITY: case MODULE_WIND: case MODULE_UV: case MODULE_AQI:
    case MODULE_CALENDAR: case MODULE_CRYPTO: case MODULE_TZ:
      return true;
    default:
      return false;
  }
}

// Shared buffer holding a module's primary value (NULL => composed in build).
static const char *module_primary_value(ModuleType m) {
  switch (m) {
    case MODULE_WEATHER:     return s_temperature_buffer;
    case MODULE_STATS:
    case MODULE_GOAL_RING:   return s_steps_buffer;
    case MODULE_TZ:          return s_tz_time_buffer;
    case MODULE_WEEK:        return s_week_buffer;
    case MODULE_COUNTDOWN:   return s_cd_buffer;
    case MODULE_DISTANCE:    return s_dist_buffer;
    case MODULE_CALENDAR:    return s_cal_title_buffer;
    case MODULE_BLUETOOTH:   return s_bt_buffer;
    case MODULE_HEARTRATE:   return s_hr_buffer;
    case MODULE_SLEEP:       return s_sleep_buffer;
    case MODULE_CALORIES:    return s_calories_buffer;
    case MODULE_ACTIVE:      return s_active_buffer;
    case MODULE_MOON:        return s_moon_buffer;
    case MODULE_DAY_OF_YEAR: return s_doy_buffer;
    case MODULE_COUNTUP:     return s_countup_buffer;
    case MODULE_CUSTOM_TEXT: return s_custom_text;
    case MODULE_QUIET_TIME:  return s_quiet_buffer;
    case MODULE_WEATHER_HL:  return s_hl_buffer;
    case MODULE_HUMIDITY:    return s_humidity_buffer;
    case MODULE_WIND:        return s_wind_buffer;
    case MODULE_UV:          return s_uv_buffer;
    case MODULE_AQI:         return s_aqi_buffer;
    case MODULE_CRYPTO:      return s_crypto_buffer;
    default:                 return NULL;  // Time/Date/Sun composed below
  }
}

static void overlay_build_text(int cell) {
  ModuleType m = s_quadrant_modules[cell];
  snprintf(s_ov_title, sizeof(s_ov_title), "%s", MODULE_NAMES[m]);
  switch (m) {
    case MODULE_STATS: {
      // Cycled by repeated taps; title becomes the metric name.
      static const char *const names[STATS_CYCLE_COUNT] = {"STEPS", "DISTANCE", "CALORIES", "ACTIVE"};
      const char *const vals[STATS_CYCLE_COUNT] = {s_steps_buffer, s_dist_buffer, s_calories_buffer, s_active_buffer};
      snprintf(s_ov_title, sizeof(s_ov_title), "%s", names[s_stats_cycle]);
      snprintf(s_ov_value, sizeof(s_ov_value), "%s", vals[s_stats_cycle]);
      break;
    }
    case MODULE_TIME:
    case MODULE_MINI_CLOCK:
      snprintf(s_ov_value, sizeof(s_ov_value), "%s:%s", s_hour_buffer, s_minute_buffer);
      break;
    case MODULE_DATE:
      snprintf(s_ov_value, sizeof(s_ov_value), "%s %s", s_day_name_buffer, s_day_number_buffer);
      break;
    case MODULE_SUN:
      snprintf(s_ov_value, sizeof(s_ov_value), "%s %s", s_sunrise_buffer, s_sunset_buffer);
      break;
    default: {
      const char *v = module_primary_value(m);
      snprintf(s_ov_value, sizeof(s_ov_value), "%s", v ? v : "--");
      break;
    }
  }
}

static void overlay_update_proc(Layer *layer, GContext *ctx) {
  if (s_overlay_cell < 0) return;
  GRect b = layer_get_bounds(layer);

  // Scrim over the whole face, then a centered white panel with a black border.
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  GRect panel = grect_inset(b, GEdgeInsets(b.size.h / 4, b.size.w / 8));
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, panel, 8, GCornersAll);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, panel, 8);

  graphics_context_set_text_color(ctx, GColorBlack);
  GRect tr = GRect(panel.origin.x, panel.origin.y + 8, panel.size.w, 24);
  graphics_draw_text(ctx, s_ov_title, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     tr, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  GRect vr = GRect(panel.origin.x, panel.origin.y + panel.size.h / 2 - 20, panel.size.w, 40);
  graphics_draw_text(ctx, s_ov_value, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                     vr, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void overlay_hide() {
  if (s_overlay_timer) { app_timer_cancel(s_overlay_timer); s_overlay_timer = NULL; }
  s_overlay_cell = -1;
  if (s_overlay_layer) layer_mark_dirty(s_overlay_layer);
}

static void overlay_timeout(void *data) {
  s_overlay_timer = NULL;
  overlay_hide();
}

static void overlay_show(int cell) {
  s_overlay_cell = cell;
  overlay_build_text(cell);
  if (s_overlay_layer) layer_mark_dirty(s_overlay_layer);
  if (s_overlay_timer) app_timer_cancel(s_overlay_timer);
  s_overlay_timer = app_timer_register(3000, overlay_timeout, NULL);
}

// If the overlay is open on a cell showing this module, rebuild its text so a
// tap-triggered refresh (or any data update) shows through live.
static void overlay_note_render(ModuleType module) {
  if (s_overlay_cell >= 0 && s_overlay_layer &&
      s_quadrant_modules[s_overlay_cell] == module) {
    overlay_build_text(s_overlay_cell);
    layer_mark_dirty(s_overlay_layer);
  }
}

// Shared tap entry point (real TouchService taps and the debug injector).
static void handle_tap(int cell) {
  if (cell < 0 || cell >= NUM_CELLS) return;
  ModuleType m = s_quadrant_modules[cell];
  if (m == MODULE_EMPTY) { overlay_hide(); return; }

  // Stats tap-to-act: repeated taps on the open overlay cycle the metric.
  if (m == MODULE_STATS && s_overlay_cell == cell) {
    s_stats_cycle = (s_stats_cycle + 1) % STATS_CYCLE_COUNT;
    overlay_show(cell);   // rebuilds text with the new metric, resets the timer
    return;
  }
  // Every other module toggles closed on a repeat tap.
  if (m != MODULE_STATS && s_overlay_cell == cell) { overlay_hide(); return; }

  // Tap-to-act: phone-data modules also trigger a refresh; the overlay updates
  // live (see overlay_note_render) when the fresh values arrive.
  if (is_phone_data_module(m)) {
    request_phone_refresh();
  }
  overlay_show(cell);
}

// Map a screen point to a cell index (grid tiles fully; round pods may miss).
static int point_to_cell(GPoint p) {
  for (int i = 0; i < NUM_CELLS; i++) {
    if (grect_contains_point(&s_cell_frame[i], &p)) return i;
  }
  return -1;
}

static GPoint s_touch_down;
// A tap = touchdown then liftoff without much travel. Drags are ignored.
static void touch_handler(const TouchEvent *e, void *context) {
  if (e->type == TouchEvent_Touchdown) {
    s_touch_down = GPoint(e->x, e->y);
  } else if (e->type == TouchEvent_Liftoff) {
    int dx = e->x - s_touch_down.x, dy = e->y - s_touch_down.y;
    if (dx * dx + dy * dy <= 20 * 20) {
      handle_tap(point_to_cell(GPoint(e->x, e->y)));
    }
  }
}

#endif  // PBL_TOUCH

// ---------------------------------------------------------------------------
// Data producers: format into the shared buffers, then re-render the cells
// showing the affected module(s).
// ---------------------------------------------------------------------------

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", tick_time);

  // Remove leading zero for 12-hour format
  if (!clock_is_24h_style() && s_hour_buffer[0] == '0') {
    memmove(s_hour_buffer, s_hour_buffer + 1, sizeof(s_hour_buffer) - 1);
  }

  strftime(s_day_name_buffer, sizeof(s_day_name_buffer), "%a", tick_time);
  strftime(s_day_number_buffer, sizeof(s_day_number_buffer), "%e", tick_time);
  strftime(s_month_name_buffer, sizeof(s_month_name_buffer), "%b", tick_time);

  for (int i = 0; s_day_name_buffer[i]; i++) {
    s_day_name_buffer[i] = toupper((unsigned char)s_day_name_buffer[i]);
  }
  for (int i = 0; s_month_name_buffer[i]; i++) {
    s_month_name_buffer[i] = toupper((unsigned char)s_month_name_buffer[i]);
  }

  render_module(MODULE_TIME);
  render_module(MODULE_DATE);
  render_module(MODULE_MINI_CLOCK);
}

// Second time zone (offset from local wall-clock time)
static void update_tz() {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  int total = lt->tm_hour * 60 + lt->tm_min + s_tz_offset_minutes;
  total = ((total % 1440) + 1440) % 1440;  // wrap into [0,1440)
  int hh = total / 60, mm = total % 60;
  if (clock_is_24h_style()) {
    snprintf(s_tz_time_buffer, sizeof(s_tz_time_buffer), "%02d:%02d", hh, mm);
  } else {
    int h12 = hh % 12; if (h12 == 0) h12 = 12;
    snprintf(s_tz_time_buffer, sizeof(s_tz_time_buffer), "%d:%02d", h12, mm);
  }
  render_module(MODULE_TZ);
}

// ISO week number
static void update_week() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(s_week_buffer, sizeof(s_week_buffer), "%V", t);
  render_module(MODULE_WEEK);
}

// Countdown to the target date (YYYYMMDD)
static void update_countdown() {
  if (s_countdown_date <= 0) {
    snprintf(s_cd_buffer, sizeof(s_cd_buffer), "--");
  } else {
    struct tm tt = {0};
    tt.tm_year = (s_countdown_date / 10000) - 1900;
    tt.tm_mon  = ((s_countdown_date / 100) % 100) - 1;
    tt.tm_mday = s_countdown_date % 100;
    tt.tm_hour = 12;
    time_t target = mktime(&tt);
    time_t now = time(NULL);
    int days = (int)((target - now) / 86400);
    if (days == 0) snprintf(s_cd_buffer, sizeof(s_cd_buffer), "TODAY");
    else snprintf(s_cd_buffer, sizeof(s_cd_buffer), "%dd", days);
  }
  render_module(MODULE_COUNTDOWN);
}

// Distance walked today (Health API)
static void update_distance() {
  HealthMetric metric = HealthMetricWalkedDistanceMeters;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  if (health_service_metric_accessible(metric, start, end) & HealthServiceAccessibilityMaskAvailable) {
    int meters = (int)health_service_sum_today(metric);
    // tenths of the display unit, using integer math (no reliable float printf)
    int tenths = s_dist_use_km ? (meters / 100) : (meters * 10 / 1609);
    snprintf(s_dist_buffer, sizeof(s_dist_buffer), "%d.%d%s",
             tenths / 10, tenths % 10, s_dist_use_km ? "km" : "mi");
  } else {
    snprintf(s_dist_buffer, sizeof(s_dist_buffer), "--");
  }
  render_module(MODULE_DISTANCE);
}

// Sum a cumulative Health metric over today, or -1 if it isn't available.
static int health_sum_today(HealthMetric metric) {
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  if (health_service_metric_accessible(metric, start, end) & HealthServiceAccessibilityMaskAvailable) {
    return (int)health_service_sum_today(metric);
  }
  return -1;
}

// Bluetooth connection status
static void update_bluetooth() {
  bool connected = connection_service_peek_pebble_app_connection();
  snprintf(s_bt_buffer, sizeof(s_bt_buffer), "%s", connected ? "OK" : "OFF");
  render_module(MODULE_BLUETOOTH);
}

// Heart rate (Health API); current BPM, not a daily sum
static void update_heartrate() {
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  if (health_service_metric_accessible(HealthMetricHeartRateBPM, start, end)
        & HealthServiceAccessibilityMaskAvailable) {
    HealthValue v = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if (v > 0) snprintf(s_hr_buffer, sizeof(s_hr_buffer), "%d", (int)v);
    else snprintf(s_hr_buffer, sizeof(s_hr_buffer), "--");
  } else {
    snprintf(s_hr_buffer, sizeof(s_hr_buffer), "--");
  }
  render_module(MODULE_HEARTRATE);
}

// Sleep duration today (hours, one decimal)
static void update_sleep() {
  int secs = health_sum_today(HealthMetricSleepSeconds);
  if (secs < 0) {
    snprintf(s_sleep_buffer, sizeof(s_sleep_buffer), "--");
  } else {
    int tenths = secs * 10 / 3600;  // integer math, no float printf
    snprintf(s_sleep_buffer, sizeof(s_sleep_buffer), "%d.%dh", tenths / 10, tenths % 10);
  }
  render_module(MODULE_SLEEP);
}

// Calories today (active + resting kcal)
static void update_calories() {
  int active = health_sum_today(HealthMetricActiveKCalories);
  int resting = health_sum_today(HealthMetricRestingKCalories);
  if (active < 0 && resting < 0) {
    snprintf(s_calories_buffer, sizeof(s_calories_buffer), "--");
  } else {
    int total = (active < 0 ? 0 : active) + (resting < 0 ? 0 : resting);
    snprintf(s_calories_buffer, sizeof(s_calories_buffer), "%d", total);
  }
  render_module(MODULE_CALORIES);
}

// Active minutes today
static void update_active() {
  int secs = health_sum_today(HealthMetricActiveSeconds);
  if (secs < 0) {
    snprintf(s_active_buffer, sizeof(s_active_buffer), "--");
  } else {
    snprintf(s_active_buffer, sizeof(s_active_buffer), "%dm", secs / 60);
  }
  render_module(MODULE_ACTIVE);
}

// Moon phase, computed from the synodic month since a known new moon.
static void update_moon() {
  const long PERIOD = 2551443;   // synodic month in seconds (29.530589 days)
  const long REF    = 947182440; // 2000-01-06 18:14 UTC new moon (unix seconds)
  long age = ((long)time(NULL) - REF) % PERIOD;
  if (age < 0) age += PERIOD;
  int idx = (int)(((long long)age * 8 + PERIOD / 2) / PERIOD) % 8;
  static const char *const names[8] = {
    "NEW", "WAX", "1QTR", "WAX+", "FULL", "WAN+", "3QTR", "WAN"
  };
  snprintf(s_moon_buffer, sizeof(s_moon_buffer), "%s", names[idx]);
  render_module(MODULE_MOON);
}

// Day of year (1..366)
static void update_day_of_year() {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(s_doy_buffer, sizeof(s_doy_buffer), "%j", t);
  // Strip leading zeros (%j is zero-padded to 3 digits)
  while (s_doy_buffer[0] == '0' && s_doy_buffer[1] != '\0') {
    memmove(s_doy_buffer, s_doy_buffer + 1, strlen(s_doy_buffer));
  }
  render_module(MODULE_DAY_OF_YEAR);
}

// Count-up: days elapsed since the target date (YYYYMMDD)
static void update_countup() {
  if (s_countup_date <= 0) {
    snprintf(s_countup_buffer, sizeof(s_countup_buffer), "--");
  } else {
    struct tm tt = {0};
    tt.tm_year = (s_countup_date / 10000) - 1900;
    tt.tm_mon  = ((s_countup_date / 100) % 100) - 1;
    tt.tm_mday = s_countup_date % 100;
    tt.tm_hour = 12;
    time_t target = mktime(&tt);
    int days = (int)((time(NULL) - target) / 86400);
    if (days < 0) days = 0;
    if (days == 0) snprintf(s_countup_buffer, sizeof(s_countup_buffer), "TODAY");
    else snprintf(s_countup_buffer, sizeof(s_countup_buffer), "%dd", days);
  }
  render_module(MODULE_COUNTUP);
}

// Quiet Time status
static void update_quiet_time() {
  snprintf(s_quiet_buffer, sizeof(s_quiet_buffer), "%s",
           quiet_time_is_active() ? "ON" : "OFF");
  render_module(MODULE_QUIET_TIME);
}

// Format a minutes-since-midnight time with a one-char prefix, honoring the
// watch's 12/24h setting (used by the sunrise/sunset module).
static void format_daytime(char *buf, size_t sz, char prefix, int minutes) {
  if (minutes < 0) {
    snprintf(buf, sz, "%c --", prefix);
    return;
  }
  int hh = (minutes / 60) % 24, mm = minutes % 60;
  if (!clock_is_24h_style()) {
    hh %= 12;
    if (hh == 0) hh = 12;
  }
  snprintf(buf, sz, "%c%d:%02d", prefix, hh, mm);
}

// Reformat the sunrise/sunset buffers from the stored minute values (called on
// receipt and whenever the 12/24h format may have changed).
static void update_sun() {
  format_daytime(s_sunrise_buffer, sizeof(s_sunrise_buffer), 'R', s_sunrise_min);
  format_daytime(s_sunset_buffer, sizeof(s_sunset_buffer), 'S', s_sunset_min);
  render_module(MODULE_SUN);
}

// Map a US EPA air-quality index (1..6) to a short label.
static const char *aqi_word(int epa) {
  switch (epa) {
    case 1: return "GOOD";
    case 2: return "MODER";
    case 3: return "SENS";   // unhealthy for sensitive groups
    case 4: return "BAD";
    case 5: return "V.BAD";
    case 6: return "HAZ";
    default: return "--";
  }
}

static void update_battery() {
  BatteryChargeState battery_state = battery_state_service_peek();
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", battery_state.charge_percent);

  uint32_t resource_id;
  if (battery_state.is_charging) {
    resource_id = RESOURCE_ID_ICON_BATTERY_CHARGING;
  } else if (battery_state.charge_percent >= 75) {
    resource_id = RESOURCE_ID_ICON_BATTERY_FULL;
  } else if (battery_state.charge_percent >= 40) {
    resource_id = RESOURCE_ID_ICON_BATTERY_MEDIUM;
  } else if (battery_state.charge_percent >= 15) {
    resource_id = RESOURCE_ID_ICON_BATTERY_LOW;
  } else {
    resource_id = RESOURCE_ID_ICON_BATTERY_EMPTY;
  }

  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
  }
  s_battery_icon = gbitmap_create_with_resource(resource_id);
  render_module(MODULE_STATS);
}

// Steps (Health API); shares the Stats module with battery
static void update_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int steps = (int)health_service_sum_today(metric);
    s_step_count = steps;
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", steps);
  } else {
    s_step_count = 0;
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "--");
  }
  render_module(MODULE_STATS);
  render_module(MODULE_GOAL_RING);
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
    update_distance();
    update_sleep();
    update_calories();
    update_active();
  }
  if (event == HealthEventHeartRateUpdate || event == HealthEventSignificantUpdate) {
    update_heartrate();
  }
}

// Bluetooth connection changes; optionally buzz on disconnect.
static void bluetooth_handler(bool connected) {
  update_bluetooth();
  if (!connected && s_bt_vibe) {
    vibes_double_pulse();
  }
}

// Convert multi-word weather conditions to single words for display
static const char* get_single_word_condition(const char* condition) {
  if (!condition) return "Unknown";

  // Convert to lowercase for easier matching
  char lower_condition[32];
  int i = 0;
  for (; condition[i] && i < 31; i++) {
    lower_condition[i] = tolower((unsigned char)condition[i]);
  }
  lower_condition[i] = '\0';

  // Get current hour to check if it's after 5 PM
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int hour = tick_time->tm_hour;

  // Special handling for "partly" conditions - use the word after "partly"
  if (strstr(lower_condition, "partly")) {
    char* partly_pos = strstr(lower_condition, "partly");
    char* after_partly = partly_pos + 6; // "partly" is 6 characters

    while (*after_partly == ' ' && *after_partly != '\0') {
      after_partly++;
    }

    if (*after_partly != '\0') {
      if (strstr(after_partly, "sunny") || strstr(after_partly, "clear")) {
        return (hour >= 17) ? "Clear" : "Sunny";
      }
      if (strstr(after_partly, "cloudy") || strstr(after_partly, "overcast")) return "Cloudy";
      if (strstr(after_partly, "rain") || strstr(after_partly, "drizzle") || strstr(after_partly, "shower")) return "Rain";
      if (strstr(after_partly, "snow") || strstr(after_partly, "sleet") || strstr(after_partly, "blizzard") || strstr(after_partly, "ice")) return "Snow";
      if (strstr(after_partly, "thunder") || strstr(after_partly, "storm")) return "Storm";
      if (strstr(after_partly, "mist")) return "Mist";
      if (strstr(after_partly, "fog")) return "Fog";
    }
    // If we can't parse the second word, fall back to "Cloudy" as default for partly conditions
    return "Cloudy";
  }

  if (strstr(lower_condition, "sunny") || strstr(lower_condition, "clear")) {
    return (hour >= 17) ? "Clear" : "Sunny";
  }
  if (strstr(lower_condition, "cloudy") || strstr(lower_condition, "overcast")) return "Cloudy";
  if (strstr(lower_condition, "mist")) return "Mist";
  if (strstr(lower_condition, "fog")) return "Fog";
  if (strstr(lower_condition, "rain") || strstr(lower_condition, "drizzle") || strstr(lower_condition, "shower")) return "Rain";
  if (strstr(lower_condition, "snow") || strstr(lower_condition, "sleet") || strstr(lower_condition, "blizzard") || strstr(lower_condition, "ice")) return "Snow";
  if (strstr(lower_condition, "thunder") || strstr(lower_condition, "storm")) return "Storm";

  return "Unknown";
}

// Tick handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  update_tz();
  update_week();
  update_countdown();
  update_countup();
  update_day_of_year();
  update_moon();
  update_quiet_time();

  // Request a phone data refresh every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    request_phone_refresh();
  }
}

static void battery_callback(BatteryChargeState charge_state) {
  update_battery();
}

// Inbox received callback: weather data, per-cell settings, module options
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // --- Weather data ---
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_Temperature);
  Tuple *icon_tuple = dict_find(iterator, MESSAGE_KEY_WeatherIcon);
  Tuple *condition_tuple = dict_find(iterator, MESSAGE_KEY_Condition);

  if (temp_tuple) {
    s_current_temperature = (int)temp_tuple->value->int32;
    s_has_temperature = true;
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d\xC2\xB0%c",
             s_current_temperature, s_use_celsius ? 'C' : 'F');
  }

  if (condition_tuple) {
    const char* single_word = get_single_word_condition(condition_tuple->value->cstring);
    snprintf(s_weather_condition_buffer, sizeof(s_weather_condition_buffer), "%s", single_word);
  }

  if (icon_tuple) {
    int icon_id = (int)icon_tuple->value->int32;
    uint32_t resource_id;
    switch (icon_id) {
      case 0: resource_id = RESOURCE_ID_ICON_WEATHER_SUNNY; break;           // Clear/Sunny
      case 1: resource_id = RESOURCE_ID_ICON_WEATHER_PARTLY_CLOUDY; break;   // Partly Cloudy
      case 2: resource_id = RESOURCE_ID_ICON_WEATHER_CLOUDY; break;          // Cloudy
      case 3: resource_id = RESOURCE_ID_ICON_WEATHER_LIGHT_RAIN; break;      // Light Rain
      case 4: resource_id = RESOURCE_ID_ICON_WEATHER_HEAVY_RAIN; break;      // Heavy Rain/Rain
      case 5: resource_id = RESOURCE_ID_ICON_WEATHER_LIGHT_SNOW; break;      // Light Snow
      case 6: resource_id = RESOURCE_ID_ICON_WEATHER_HEAVY_SNOW; break;      // Heavy Snow/Snow
      case 7: resource_id = RESOURCE_ID_ICON_WEATHER_RAIN_SNOW; break;       // Rain and Snow
      default: resource_id = RESOURCE_ID_ICON_WEATHER_GENERIC; break;        // Generic/Unknown
    }
    if (s_weather_icon) {
      gbitmap_destroy(s_weather_icon);
    }
    s_weather_icon = gbitmap_create_with_resource(resource_id);
  }

  Tuple *temp_unit_tuple = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_tuple) {
    s_use_celsius = temp_unit_tuple->value->int32 == 1;
    if (s_has_temperature) {
      snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d\xC2\xB0%c",
               s_current_temperature, s_use_celsius ? 'C' : 'F');
    }
  }

  if (temp_tuple || condition_tuple || icon_tuple || temp_unit_tuple) {
    render_module(MODULE_WEATHER);
  }

  // --- Phase 3d weather-family fields (from forecast.json) ---
  // Hi/Lo arrive as ints already in the selected temperature unit.
  Tuple *hi_t = dict_find(iterator, MESSAGE_KEY_WeatherHigh);
  Tuple *lo_t = dict_find(iterator, MESSAGE_KEY_WeatherLow);
  if (hi_t || lo_t) {
    static int s_hi = 0, s_lo = 0;
    if (hi_t) s_hi = (int)hi_t->value->int32;
    if (lo_t) s_lo = (int)lo_t->value->int32;
    snprintf(s_hl_buffer, sizeof(s_hl_buffer), "H%d L%d", s_hi, s_lo);
    render_module(MODULE_WEATHER_HL);
  }
  Tuple *hum_t = dict_find(iterator, MESSAGE_KEY_Humidity);
  if (hum_t) {
    snprintf(s_humidity_buffer, sizeof(s_humidity_buffer), "%d%%", (int)hum_t->value->int32);
    render_module(MODULE_HUMIDITY);
  }
  Tuple *wind_t = dict_find(iterator, MESSAGE_KEY_Wind);
  if (wind_t) {
    // pkjs sends wind already in the unit matching the temperature setting.
    snprintf(s_wind_buffer, sizeof(s_wind_buffer), "%d%s",
             (int)wind_t->value->int32, s_use_celsius ? "kph" : "mph");
    render_module(MODULE_WIND);
  }
  Tuple *uv_t = dict_find(iterator, MESSAGE_KEY_UV);
  if (uv_t) {
    snprintf(s_uv_buffer, sizeof(s_uv_buffer), "%d", (int)uv_t->value->int32);
    render_module(MODULE_UV);
  }
  Tuple *aqi_t = dict_find(iterator, MESSAGE_KEY_AQI);
  if (aqi_t) {
    snprintf(s_aqi_buffer, sizeof(s_aqi_buffer), "%s", aqi_word((int)aqi_t->value->int32));
    render_module(MODULE_AQI);
  }
  Tuple *sr_t = dict_find(iterator, MESSAGE_KEY_SunriseMin);
  Tuple *ss_t = dict_find(iterator, MESSAGE_KEY_SunsetMin);
  if (sr_t || ss_t) {
    if (sr_t) s_sunrise_min = (int)sr_t->value->int32;
    if (ss_t) s_sunset_min = (int)ss_t->value->int32;
    update_sun();
  }
  // Crypto ticker: pkjs sends a preformatted "SYM 62.2k" string.
  Tuple *crypto_t = dict_find(iterator, MESSAGE_KEY_CryptoPrice);
  if (crypto_t) {
    strncpy(s_crypto_buffer, crypto_t->value->cstring, sizeof(s_crypto_buffer) - 1);
    s_crypto_buffer[sizeof(s_crypto_buffer) - 1] = '\0';
    persist_write_string(PERSIST_CRYPTO_PRICE, s_crypto_buffer);
    render_module(MODULE_CRYPTO);
  }

  // --- Per-cell settings ---
  // MESSAGE_KEY_* are runtime externs, so these are runtime-initialized local
  // arrays (not static/const-initialized).
  const uint32_t K_MODULE[MAX_CELLS] = {
    MESSAGE_KEY_Quadrant1Module, MESSAGE_KEY_Quadrant2Module, MESSAGE_KEY_Quadrant3Module,
    MESSAGE_KEY_Quadrant4Module, MESSAGE_KEY_Quadrant5Module, MESSAGE_KEY_Quadrant6Module,
    MESSAGE_KEY_Quadrant7Module, MESSAGE_KEY_Quadrant8Module, MESSAGE_KEY_Quadrant9Module };
  const uint32_t K_BG[MAX_CELLS] = {
    MESSAGE_KEY_Quadrant1Background, MESSAGE_KEY_Quadrant2Background, MESSAGE_KEY_Quadrant3Background,
    MESSAGE_KEY_Quadrant4Background, MESSAGE_KEY_Quadrant5Background, MESSAGE_KEY_Quadrant6Background,
    MESSAGE_KEY_Quadrant7Background, MESSAGE_KEY_Quadrant8Background, MESSAGE_KEY_Quadrant9Background };
#ifdef PBL_COLOR
  const uint32_t K_COLOR[MAX_CELLS] = {
    MESSAGE_KEY_Quadrant1Color, MESSAGE_KEY_Quadrant2Color, MESSAGE_KEY_Quadrant3Color,
    MESSAGE_KEY_Quadrant4Color, MESSAGE_KEY_Quadrant5Color, MESSAGE_KEY_Quadrant6Color,
    MESSAGE_KEY_Quadrant7Color, MESSAGE_KEY_Quadrant8Color, MESSAGE_KEY_Quadrant9Color };
  const uint32_t K_AUTOTEXT[MAX_CELLS] = {
    MESSAGE_KEY_Quadrant1AutoTextColor, MESSAGE_KEY_Quadrant2AutoTextColor, MESSAGE_KEY_Quadrant3AutoTextColor,
    MESSAGE_KEY_Quadrant4AutoTextColor, MESSAGE_KEY_Quadrant5AutoTextColor, MESSAGE_KEY_Quadrant6AutoTextColor,
    MESSAGE_KEY_Quadrant7AutoTextColor, MESSAGE_KEY_Quadrant8AutoTextColor, MESSAGE_KEY_Quadrant9AutoTextColor };
  const uint32_t K_TEXTCOLOR[MAX_CELLS] = {
    MESSAGE_KEY_Quadrant1TextColor, MESSAGE_KEY_Quadrant2TextColor, MESSAGE_KEY_Quadrant3TextColor,
    MESSAGE_KEY_Quadrant4TextColor, MESSAGE_KEY_Quadrant5TextColor, MESSAGE_KEY_Quadrant6TextColor,
    MESSAGE_KEY_Quadrant7TextColor, MESSAGE_KEY_Quadrant8TextColor, MESSAGE_KEY_Quadrant9TextColor };
#endif

  bool cell_changed[MAX_CELLS] = {false};
  bool background_changed = false;

  for (int i = 0; i < NUM_CELLS; i++) {
    Tuple *t = dict_find(iterator, K_MODULE[i]);
    if (t) {
      ModuleType m = sanitize_module((int)t->value->int32);
      if (s_quadrant_modules[i] != m) {
        s_quadrant_modules[i] = m;
        persist_write_int(PERSIST_MODULE_BASE + i, m);
        cell_changed[i] = true;
      }
    }

    Tuple *bg = dict_find(iterator, K_BG[i]);
    if (bg) {
      bool new_bg = bg->value->int32 == 1;
      if (s_quadrant_backgrounds[i] != new_bg) {
        s_quadrant_backgrounds[i] = new_bg;
        persist_write_bool(PERSIST_BG_BASE + i, new_bg);
        // Fonts (B&W) and auto text color depend on the background
        cell_changed[i] = true;
        background_changed = true;
      }
    }

#ifdef PBL_COLOR
    Tuple *ct = dict_find(iterator, K_COLOR[i]);
    if (ct) {
      s_quadrant_colors[i] = GColorFromHEX(ct->value->int32);
      persist_write_int(PERSIST_COLOR_BASE + i, ct->value->int32);
      cell_changed[i] = true;
      background_changed = true;
    }
    Tuple *at = dict_find(iterator, K_AUTOTEXT[i]);
    if (at) {
      s_auto_text_color[i] = (at->value->int32 == 1);
      persist_write_bool(PERSIST_AUTOTEXT_BASE + i, s_auto_text_color[i]);
      cell_changed[i] = true;
    }
    Tuple *tc = dict_find(iterator, K_TEXTCOLOR[i]);
    if (tc) {
      s_custom_text_color[i] = GColorFromHEX(tc->value->int32);
      persist_write_int(PERSIST_TEXTCOLOR_BASE + i, tc->value->int32);
      cell_changed[i] = true;
    }
#endif
  }

  // --- New-module settings ---
  Tuple *tz_off = dict_find(iterator, MESSAGE_KEY_TZOffset);
  if (tz_off) {
    s_tz_offset_minutes = (int)tz_off->value->int32;
    persist_write_int(PERSIST_TZ_OFFSET, s_tz_offset_minutes);
    update_tz();
  }
  Tuple *tz_lbl = dict_find(iterator, MESSAGE_KEY_TZLabel);
  if (tz_lbl) {
    strncpy(s_tz_label, tz_lbl->value->cstring, sizeof(s_tz_label) - 1);
    s_tz_label[sizeof(s_tz_label) - 1] = '\0';
    persist_write_string(PERSIST_TZ_LABEL, s_tz_label);
    render_module(MODULE_TZ);
  }
  Tuple *cd_date = dict_find(iterator, MESSAGE_KEY_CountdownDate);
  if (cd_date) {
    // Clay text inputs arrive as strings; extract digits into a YYYYMMDD int.
    int v = 0;
    for (const char *p = cd_date->value->cstring; *p && v < 100000000; p++) {
      if (*p >= '0' && *p <= '9') v = v * 10 + (*p - '0');
    }
    s_countdown_date = v;
    persist_write_int(PERSIST_COUNTDOWN_DATE, s_countdown_date);
    update_countdown();
  }
  Tuple *cd_lbl = dict_find(iterator, MESSAGE_KEY_CountdownLabel);
  if (cd_lbl) {
    strncpy(s_countdown_label, cd_lbl->value->cstring, sizeof(s_countdown_label) - 1);
    s_countdown_label[sizeof(s_countdown_label) - 1] = '\0';
    persist_write_string(PERSIST_COUNTDOWN_LABEL, s_countdown_label);
    render_module(MODULE_COUNTDOWN);
  }
  Tuple *dist_u = dict_find(iterator, MESSAGE_KEY_DistanceUnits);
  if (dist_u) {
    s_dist_use_km = (dist_u->value->int32 == 1);
    persist_write_int(PERSIST_DIST_UNITS, s_dist_use_km ? 1 : 0);
    update_distance();
  }
  Tuple *bt_vibe = dict_find(iterator, MESSAGE_KEY_BluetoothVibe);
  if (bt_vibe) {
    s_bt_vibe = (bt_vibe->value->int32 == 1);
    persist_write_bool(PERSIST_BT_VIBE, s_bt_vibe);
  }
  Tuple *cu_date = dict_find(iterator, MESSAGE_KEY_CountupDate);
  if (cu_date) {
    int v = 0;
    for (const char *p = cu_date->value->cstring; *p && v < 100000000; p++) {
      if (*p >= '0' && *p <= '9') v = v * 10 + (*p - '0');
    }
    s_countup_date = v;
    persist_write_int(PERSIST_COUNTUP_DATE, s_countup_date);
    update_countup();
  }
  Tuple *cu_lbl = dict_find(iterator, MESSAGE_KEY_CountupLabel);
  if (cu_lbl) {
    strncpy(s_countup_label, cu_lbl->value->cstring, sizeof(s_countup_label) - 1);
    s_countup_label[sizeof(s_countup_label) - 1] = '\0';
    persist_write_string(PERSIST_COUNTUP_LABEL, s_countup_label);
    render_module(MODULE_COUNTUP);
  }
  Tuple *ctext = dict_find(iterator, MESSAGE_KEY_CustomText);
  if (ctext) {
    strncpy(s_custom_text, ctext->value->cstring, sizeof(s_custom_text) - 1);
    s_custom_text[sizeof(s_custom_text) - 1] = '\0';
    persist_write_string(PERSIST_CUSTOM_TEXT, s_custom_text);
    render_module(MODULE_CUSTOM_TEXT);
  }
  Tuple *sgoal = dict_find(iterator, MESSAGE_KEY_StepGoal);
  if (sgoal) {
    // Clay text inputs arrive as strings; extract digits into an int.
    int g = 0;
    for (const char *p = sgoal->value->cstring; *p && g < 1000000; p++) {
      if (*p >= '0' && *p <= '9') g = g * 10 + (*p - '0');
    }
    if (g > 0) {
      s_step_goal = g;
      persist_write_int(PERSIST_STEP_GOAL, s_step_goal);
      render_module(MODULE_GOAL_RING);
    }
  }
  Tuple *cal_title = dict_find(iterator, MESSAGE_KEY_CalendarTitle);
  if (cal_title) {
    strncpy(s_cal_title_buffer, cal_title->value->cstring, sizeof(s_cal_title_buffer) - 1);
    s_cal_title_buffer[sizeof(s_cal_title_buffer) - 1] = '\0';
    if (s_cal_title_buffer[0] == '\0') {
      // No upcoming event: show the placeholder and clear the time line
      s_cal_title_buffer[0] = '-'; s_cal_title_buffer[1] = '-'; s_cal_title_buffer[2] = '\0';
      s_cal_time_buffer[0] = '\0';
    }
    render_module(MODULE_CALENDAR);
  }
  Tuple *cal_time = dict_find(iterator, MESSAGE_KEY_CalendarTime);
  if (cal_time) {
    strncpy(s_cal_time_buffer, cal_time->value->cstring, sizeof(s_cal_time_buffer) - 1);
    s_cal_time_buffer[sizeof(s_cal_time_buffer) - 1] = '\0';
    render_module(MODULE_CALENDAR);
  }

  // Rebuild changed cells (module/fonts/colors) and redraw backgrounds
  if (s_window_layer) {
    for (int i = 0; i < NUM_CELLS; i++) {
      if (cell_changed[i]) {
        cell_build_ui(i);
      }
    }
  }
  if (background_changed && s_background_layer) {
    layer_mark_dirty(s_background_layer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason: %d", (int)reason);
}

// Main window load
static void main_window_load(Window *window) {
  // Compute screen geometry first: every layer size/position and font choice below
  // depends on it (must run before background layer and cell UI creation).
  compute_screen_geometry(window);

  // Background layer (full screen)
  s_background_layer = layer_create(GRect(0, 0, s_screen_w, s_screen_h));
  layer_set_update_proc(s_background_layer, background_layer_update_proc);
  layer_add_child(window_get_root_layer(window), s_background_layer);

  // Shared weather icon (the battery icon is created by update_battery)
  s_weather_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_WEATHER_GENERIC);

  // Populate the data buffers while s_window_layer is still NULL (render_module
  // no-ops), then build each cell's UI from the registry.
  update_time();
  update_battery();
  update_steps();
  update_tz();
  update_week();
  update_countdown();
  update_distance();
  update_bluetooth();
  update_heartrate();
  update_sleep();
  update_calories();
  update_active();
  update_moon();
  update_day_of_year();
  update_countup();
  update_quiet_time();

  s_window_layer = window_get_root_layer(window);
  for (int i = 0; i < NUM_CELLS; i++) {
    cell_build_ui(i);
  }

#ifdef PBL_TOUCH
  // Tap detail overlay, added last so it draws on top of every cell. It paints
  // nothing until a tap sets s_overlay_cell (see overlay_update_proc).
  s_overlay_layer = layer_create(GRect(0, 0, s_screen_w, s_screen_h));
  layer_set_update_proc(s_overlay_layer, overlay_update_proc);
  layer_add_child(s_window_layer, s_overlay_layer);
#endif
}

// Main window unload
static void main_window_unload(Window *window) {
#ifdef PBL_TOUCH
  overlay_hide();
  if (s_overlay_layer) {
    layer_destroy(s_overlay_layer);
    s_overlay_layer = NULL;
  }
#endif
  for (int i = 0; i < NUM_CELLS; i++) {
    cell_destroy_ui(i);
  }
  layer_destroy(s_background_layer);
  s_background_layer = NULL;
  s_window_layer = NULL;

  if (s_weather_icon) {
    gbitmap_destroy(s_weather_icon);
    s_weather_icon = NULL;
  }
  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
    s_battery_icon = NULL;
  }
}

// Init
static void init() {
  // Load persisted per-cell settings (cells 0..MAX_CELLS-1)
  for (int i = 0; i < MAX_CELLS; i++) {
    if (persist_exists(PERSIST_MODULE_BASE + i)) {
      s_quadrant_modules[i] = sanitize_module(persist_read_int(PERSIST_MODULE_BASE + i));
    }
    if (persist_exists(PERSIST_BG_BASE + i)) {
      s_quadrant_backgrounds[i] = persist_read_bool(PERSIST_BG_BASE + i);
    }
#ifdef PBL_COLOR
    s_quadrant_colors[i] = persist_exists(PERSIST_COLOR_BASE + i) ?
      GColorFromHEX(persist_read_int(PERSIST_COLOR_BASE + i)) : GColorLightGray;
    if (persist_exists(PERSIST_AUTOTEXT_BASE + i)) {
      s_auto_text_color[i] = persist_read_bool(PERSIST_AUTOTEXT_BASE + i);
    }
    s_custom_text_color[i] = persist_exists(PERSIST_TEXTCOLOR_BASE + i) ?
      GColorFromHEX(persist_read_int(PERSIST_TEXTCOLOR_BASE + i)) : GColorBlack;
#endif
  }

  // Load persisted new-module settings
  if (persist_exists(PERSIST_TZ_OFFSET)) s_tz_offset_minutes = persist_read_int(PERSIST_TZ_OFFSET);
  if (persist_exists(PERSIST_TZ_LABEL)) persist_read_string(PERSIST_TZ_LABEL, s_tz_label, sizeof(s_tz_label));
  if (persist_exists(PERSIST_COUNTDOWN_DATE)) s_countdown_date = persist_read_int(PERSIST_COUNTDOWN_DATE);
  if (persist_exists(PERSIST_COUNTDOWN_LABEL)) persist_read_string(PERSIST_COUNTDOWN_LABEL, s_countdown_label, sizeof(s_countdown_label));
  if (persist_exists(PERSIST_DIST_UNITS)) s_dist_use_km = (persist_read_int(PERSIST_DIST_UNITS) == 1);
  if (persist_exists(PERSIST_BT_VIBE)) s_bt_vibe = persist_read_bool(PERSIST_BT_VIBE);
  if (persist_exists(PERSIST_COUNTUP_DATE)) s_countup_date = persist_read_int(PERSIST_COUNTUP_DATE);
  if (persist_exists(PERSIST_COUNTUP_LABEL)) persist_read_string(PERSIST_COUNTUP_LABEL, s_countup_label, sizeof(s_countup_label));
  if (persist_exists(PERSIST_CUSTOM_TEXT)) persist_read_string(PERSIST_CUSTOM_TEXT, s_custom_text, sizeof(s_custom_text));
  if (persist_exists(PERSIST_STEP_GOAL)) s_step_goal = persist_read_int(PERSIST_STEP_GOAL);
  if (persist_exists(PERSIST_CRYPTO_PRICE)) persist_read_string(PERSIST_CRYPTO_PRICE, s_crypto_buffer, sizeof(s_crypto_buffer));

  // Create main window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  // Register services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  health_service_events_subscribe(health_handler, NULL);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_handler
  });
#ifdef PBL_TOUCH
  // Touch is available on emery + gabbro; taps drive the detail overlay.
  if (touch_service_is_enabled()) {
    touch_service_subscribe(touch_handler, NULL);
  }
#endif

  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(512, 512);
}

// Deinit
static void deinit() {
  health_service_events_unsubscribe();
  connection_service_unsubscribe();
#ifdef PBL_TOUCH
  touch_service_unsubscribe();
#endif
  window_destroy(s_main_window);
}

// Main
int main(void) {
  init();
  app_event_loop();
  deinit();
}
