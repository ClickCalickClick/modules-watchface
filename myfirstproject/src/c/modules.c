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
  MODULE_STATS = 4
} ModuleType;

// Persistence keys
#define PERSIST_KEY_Q1_MODULE 100
#define PERSIST_KEY_Q2_MODULE 101
#define PERSIST_KEY_Q3_MODULE 102
#define PERSIST_KEY_Q4_MODULE 103
#define PERSIST_KEY_Q1_BACKGROUND 104
#define PERSIST_KEY_Q2_BACKGROUND 105
#define PERSIST_KEY_Q3_BACKGROUND 106
#define PERSIST_KEY_Q4_BACKGROUND 107
#define PERSIST_KEY_Q1_COLOR 108
#define PERSIST_KEY_Q2_COLOR 109
#define PERSIST_KEY_Q3_COLOR 110
#define PERSIST_KEY_Q4_COLOR 111
#define PERSIST_KEY_Q1_AUTO_TEXT 112
#define PERSIST_KEY_Q2_AUTO_TEXT 113
#define PERSIST_KEY_Q3_AUTO_TEXT 114
#define PERSIST_KEY_Q4_AUTO_TEXT 115
#define PERSIST_KEY_Q1_TEXT_COLOR 116
#define PERSIST_KEY_Q2_TEXT_COLOR 117
#define PERSIST_KEY_Q3_TEXT_COLOR 118
#define PERSIST_KEY_Q4_TEXT_COLOR 119
#define PERSIST_KEY_ALT_WEATHER_LAYOUT 120

// UI Elements
static Window *s_main_window;
static Layer *s_background_layer;

// Quadrant 1 - Date (Top Left)
static TextLayer *s_day_name_layer;
static TextLayer *s_day_number_layer;
static TextLayer *s_month_name_layer;

// Quadrant 2 - Weather (Top Right)
static BitmapLayer *s_weather_icon_layer;
static GBitmap *s_weather_icon;
static TextLayer *s_temperature_layer;
static TextLayer *s_weather_condition_layer;

// Quadrant 3 - Time (Bottom Left)
static TextLayer *s_hour_layer;
static TextLayer *s_minute_layer;

// Quadrant 4 - Stats (Bottom Right)
static BitmapLayer *s_battery_icon_layer;
static GBitmap *s_battery_icon;
static TextLayer *s_battery_text_layer;
static Layer *s_divider_layer;
static TextLayer *s_steps_count_layer;
static TextLayer *s_steps_label_layer;

// Data buffers
static char s_day_name_buffer[4];
static char s_day_number_buffer[3];
static char s_month_name_buffer[4];
static char s_temperature_buffer[8];
static char s_hour_buffer[3];
static char s_minute_buffer[3];
static char s_battery_buffer[6];
static char s_steps_buffer[12];
static char s_weather_condition_buffer[16];

// Settings
static bool s_use_celsius = false;
static bool s_alt_weather_layout = false;
static int s_current_temperature = 0;
static bool s_has_temperature = false;
static int s_current_icon_id = -1;

// Module assignments for each quadrant
static ModuleType s_quadrant_modules[4] = {
  MODULE_DATE,    // Q1 default
  MODULE_WEATHER, // Q2 default
  MODULE_TIME,    // Q3 default
  MODULE_STATS    // Q4 default
};

// Background state for each quadrant (true = light gray, false = white)
static bool s_quadrant_backgrounds[4] = {
  false,  // Q1 default: white
  true,   // Q2 default: light gray
  true,   // Q3 default: light gray
  false   // Q4 default: white
};

// Color values for each quadrant (stored as 32-bit integer in ARGB8 format)
static GColor s_quadrant_colors[4];

// Auto text color enabled for each quadrant (true = auto, false = manual)
static bool s_auto_text_color[4] = {
  true,  // Q1 default: auto
  true,  // Q2 default: auto
  true,  // Q3 default: auto
  true   // Q4 default: auto
};

// Custom text color for each quadrant (when auto is disabled)
static GColor s_custom_text_color[4];

// Quadrant origins (x, y)
static const GPoint QUADRANT_ORIGINS[4] = {
  {0, 0},     // Q1 - Top Left
  {72, 0},    // Q2 - Top Right
  {0, 84},    // Q3 - Bottom Left
  {72, 84}    // Q4 - Bottom Right
};

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

// Determine text color for a quadrant based on background and settings
static GColor get_text_color_for_quadrant(int quadrant) {
  GColor result = GColorBlack;
  bool auto_mode = s_auto_text_color[quadrant];
  
  if (!auto_mode) {
    result = s_custom_text_color[quadrant];
    APP_LOG(APP_LOG_LEVEL_DEBUG, "TextColor q%d manual -> 0x%02X", quadrant + 1, result.argb);
    return result;
  }
  
#ifdef PBL_COLOR
  if (s_quadrant_backgrounds[quadrant]) {
    GColor bg_color = s_quadrant_colors[quadrant];
    uint8_t brightness = prv_calculate_brightness(bg_color);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "TextColor q%d auto bg=0x%02X brightness=%d", quadrant + 1, bg_color.argb, brightness);
    if (brightness < 128) {
      result = GColorWhite;  // Dark background, use white text
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "TextColor q%d auto bg disabled", quadrant + 1);
  }
#else
  APP_LOG(APP_LOG_LEVEL_DEBUG, "TextColor q%d auto (B&W) -> black", quadrant + 1);
#endif
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "TextColor q%d auto result 0x%02X", quadrant + 1, result.argb);
  return result;
}

// Relative positions for DATE module (relative to quadrant origin)
static const GRect DATE_LAYOUTS[3] = {
  {{0, 0}, {72, 20}},   // day name (increased height for GOTHIC_18_BOLD)
  {{0, 15}, {72, 66}},  // day number
  {{0, 63}, {72, 77}}   // month name (moved down 1px)
};

// Relative positions for WEATHER module (relative to quadrant origin)
static const GRect WEATHER_LAYOUTS_DEFAULT[3] = {
  {{22, 2}, {28, 30}},  // icon (centered: 72/2 - 28/2 = 22)
  {{0, 30}, {72, 58}},  // temperature
  {{0, 55}, {72, 79}}   // condition
};

static const GRect WEATHER_LAYOUTS_ALT[3] = {
  {{9, 0}, {54, 54}},   // icon (centered: 72/2 - 54/2 = 9)
  {{0, 45}, {72, 30}},  // temperature (moved up to 45)
  {{0, 84}, {72, 0}}    // condition (hidden)
};

// Relative positions for TIME module (relative to quadrant origin)
// Relative positioning templates for each module type
static const GRect TIME_LAYOUTS[] = {
  {{0, 0}, {72, 42}},   // Hour label
  {{0, 34}, {72, 70}}   // Minute label (5px gap from hour)
};

// Relative positions for STATS module (relative to quadrant origin)
static const GRect STATS_LAYOUTS[5] = {
  {{14, 8}, {16, 24}},   // battery icon (moved up 2px to align with text)
  {{32, 7}, {40, 29}},   // battery text (offset from icon)
  {{0, 34}, {72, 61}},   // steps count
  {{0, 60}, {72, 76}}    // steps label
};

// Background layer update procedure
static void background_layer_update_proc(Layer *layer, GContext *ctx) {
  // Fill quadrant backgrounds based on settings
  for (int q = 0; q < 4; q++) {
    GPoint origin = QUADRANT_ORIGINS[q];
    GColor color;
    
#ifdef PBL_COLOR
    // On color platforms, use custom color if background is enabled
    color = s_quadrant_backgrounds[q] ? s_quadrant_colors[q] : GColorWhite;
#else
    // On B&W platforms, use light gray or white
    color = s_quadrant_backgrounds[q] ? GColorLightGray : GColorWhite;
#endif
    
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(origin.x, origin.y, 72, 84), 0, GCornerNone);
  }
  
  // Draw grid lines
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  
  // Vertical line at x=72
  graphics_draw_line(ctx, GPoint(72, 0), GPoint(72, 168));
  
  // Horizontal line at y=84
  graphics_draw_line(ctx, GPoint(0, 84), GPoint(144, 84));
}

// Divider layer for Q4
static void divider_layer_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(6, 32), GPoint(66, 32)); // Relative to layer position
}

// Update fonts based on platform and background state
static void update_fonts_for_background() {
#ifndef PBL_COLOR
  // On B&W Pebbles, increase font sizes when backgrounds are enabled
  // Find which quadrant has DATE module
  int date_quadrant = -1;
  int stats_quadrant = -1;
  
  for (int q = 0; q < 4; q++) {
    if (s_quadrant_modules[q] == MODULE_DATE) date_quadrant = q;
    if (s_quadrant_modules[q] == MODULE_STATS) stats_quadrant = q;
  }
  
  // Update DATE fonts if background is enabled for that quadrant
  if (date_quadrant >= 0 && s_quadrant_backgrounds[date_quadrant]) {
    text_layer_set_font(s_day_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_font(s_month_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  } else {
    text_layer_set_font(s_day_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_font(s_month_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  }
  
  // Update STATS fonts if background is enabled for that quadrant
  if (stats_quadrant >= 0 && s_quadrant_backgrounds[stats_quadrant]) {
    text_layer_set_font(s_steps_count_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_font(s_steps_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  } else {
    text_layer_set_font(s_steps_count_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_font(s_steps_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  }
#endif
}

// Update text colors for all modules based on quadrant assignments
static void update_text_colors() {
  for (int q = 0; q < 4; q++) {
    GColor text_color = get_text_color_for_quadrant(q);
    ModuleType module = s_quadrant_modules[q];
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Applying text color 0x%02X to quadrant %d module %d", text_color.argb, q + 1, module);
    
    switch (module) {
      case MODULE_DATE:
        text_layer_set_text_color(s_day_name_layer, text_color);
        text_layer_set_text_color(s_day_number_layer, text_color);
        text_layer_set_text_color(s_month_name_layer, text_color);
        break;
        
      case MODULE_WEATHER:
        text_layer_set_text_color(s_temperature_layer, text_color);
        text_layer_set_text_color(s_weather_condition_layer, text_color);
        break;
        
      case MODULE_TIME:
        text_layer_set_text_color(s_hour_layer, text_color);
        text_layer_set_text_color(s_minute_layer, text_color);
        break;
        
      case MODULE_STATS:
        text_layer_set_text_color(s_battery_text_layer, text_color);
        text_layer_set_text_color(s_steps_count_layer, text_color);
        text_layer_set_text_color(s_steps_label_layer, text_color);
        break;
        
      case MODULE_EMPTY:
      default:
        break;
    }
  }
}

// Reposition layers based on module assignments
static void reposition_layers() {
  for (int q = 0; q < 4; q++) {
    GPoint origin = QUADRANT_ORIGINS[q];
    ModuleType module = s_quadrant_modules[q];
    
    switch (module) {
      case MODULE_DATE:
        layer_set_frame(text_layer_get_layer(s_day_name_layer), 
          GRect(origin.x + DATE_LAYOUTS[0].origin.x, origin.y + DATE_LAYOUTS[0].origin.y,
                DATE_LAYOUTS[0].size.w, DATE_LAYOUTS[0].size.h));
        layer_set_frame(text_layer_get_layer(s_day_number_layer),
          GRect(origin.x + DATE_LAYOUTS[1].origin.x, origin.y + DATE_LAYOUTS[1].origin.y,
                DATE_LAYOUTS[1].size.w, DATE_LAYOUTS[1].size.h));
        layer_set_frame(text_layer_get_layer(s_month_name_layer),
          GRect(origin.x + DATE_LAYOUTS[2].origin.x, origin.y + DATE_LAYOUTS[2].origin.y,
                DATE_LAYOUTS[2].size.w, DATE_LAYOUTS[2].size.h));
        layer_set_hidden(text_layer_get_layer(s_day_name_layer), false);
        layer_set_hidden(text_layer_get_layer(s_day_number_layer), false);
        layer_set_hidden(text_layer_get_layer(s_month_name_layer), false);
        break;
        
      case MODULE_WEATHER: {
        const GRect *layout = s_alt_weather_layout ? WEATHER_LAYOUTS_ALT : WEATHER_LAYOUTS_DEFAULT;
        layer_set_frame(bitmap_layer_get_layer(s_weather_icon_layer),
          GRect(origin.x + layout[0].origin.x, origin.y + layout[0].origin.y,
                layout[0].size.w, layout[0].size.h));
        layer_set_frame(text_layer_get_layer(s_temperature_layer),
          GRect(origin.x + layout[1].origin.x, origin.y + layout[1].origin.y,
                layout[1].size.w, layout[1].size.h));
        layer_set_frame(text_layer_get_layer(s_weather_condition_layer),
          GRect(origin.x + layout[2].origin.x, origin.y + layout[2].origin.y,
                layout[2].size.w, layout[2].size.h));
        layer_set_hidden(bitmap_layer_get_layer(s_weather_icon_layer), false);
        layer_set_hidden(text_layer_get_layer(s_temperature_layer), false);
        layer_set_hidden(text_layer_get_layer(s_weather_condition_layer), false);
        break;
      }
        
      case MODULE_TIME:
        layer_set_frame(text_layer_get_layer(s_hour_layer),
          GRect(origin.x + TIME_LAYOUTS[0].origin.x, origin.y + TIME_LAYOUTS[0].origin.y,
                TIME_LAYOUTS[0].size.w, TIME_LAYOUTS[0].size.h));
        layer_set_frame(text_layer_get_layer(s_minute_layer),
          GRect(origin.x + TIME_LAYOUTS[1].origin.x, origin.y + TIME_LAYOUTS[1].origin.y,
                TIME_LAYOUTS[1].size.w, TIME_LAYOUTS[1].size.h));
        layer_set_hidden(text_layer_get_layer(s_hour_layer), false);
        layer_set_hidden(text_layer_get_layer(s_minute_layer), false);
        break;
        
      case MODULE_STATS:
        layer_set_frame(bitmap_layer_get_layer(s_battery_icon_layer),
          GRect(origin.x + STATS_LAYOUTS[0].origin.x, origin.y + STATS_LAYOUTS[0].origin.y,
                STATS_LAYOUTS[0].size.w, STATS_LAYOUTS[0].size.h));
        layer_set_frame(text_layer_get_layer(s_battery_text_layer),
          GRect(origin.x + STATS_LAYOUTS[1].origin.x, origin.y + STATS_LAYOUTS[1].origin.y,
                STATS_LAYOUTS[1].size.w, STATS_LAYOUTS[1].size.h));
        layer_set_frame(text_layer_get_layer(s_steps_count_layer),
          GRect(origin.x + STATS_LAYOUTS[2].origin.x, origin.y + STATS_LAYOUTS[2].origin.y,
                STATS_LAYOUTS[2].size.w, STATS_LAYOUTS[2].size.h));
        layer_set_frame(text_layer_get_layer(s_steps_label_layer),
          GRect(origin.x + STATS_LAYOUTS[3].origin.x, origin.y + STATS_LAYOUTS[3].origin.y,
                STATS_LAYOUTS[3].size.w, STATS_LAYOUTS[3].size.h));
        layer_set_frame(s_divider_layer,
          GRect(origin.x, origin.y, 72, 84));
        layer_set_hidden(bitmap_layer_get_layer(s_battery_icon_layer), false);
        layer_set_hidden(text_layer_get_layer(s_battery_text_layer), false);
        layer_set_hidden(text_layer_get_layer(s_steps_count_layer), false);
        layer_set_hidden(text_layer_get_layer(s_steps_label_layer), false);
        layer_set_hidden(s_divider_layer, false);
        break;
        
      case MODULE_EMPTY:
      default:
        // Hide all layers for empty quadrant
        break;
    }
  }
  
  // Hide layers not assigned to any quadrant
  bool date_assigned = false, weather_assigned = false, time_assigned = false, stats_assigned = false;
  for (int q = 0; q < 4; q++) {
    if (s_quadrant_modules[q] == MODULE_DATE) date_assigned = true;
    if (s_quadrant_modules[q] == MODULE_WEATHER) weather_assigned = true;
    if (s_quadrant_modules[q] == MODULE_TIME) time_assigned = true;
    if (s_quadrant_modules[q] == MODULE_STATS) stats_assigned = true;
  }
  
  if (!date_assigned) {
    layer_set_hidden(text_layer_get_layer(s_day_name_layer), true);
    layer_set_hidden(text_layer_get_layer(s_day_number_layer), true);
    layer_set_hidden(text_layer_get_layer(s_month_name_layer), true);
  }
  if (!weather_assigned) {
    layer_set_hidden(bitmap_layer_get_layer(s_weather_icon_layer), true);
    layer_set_hidden(text_layer_get_layer(s_temperature_layer), true);
    layer_set_hidden(text_layer_get_layer(s_weather_condition_layer), true);
  }
  if (!time_assigned) {
    layer_set_hidden(text_layer_get_layer(s_hour_layer), true);
    layer_set_hidden(text_layer_get_layer(s_minute_layer), true);
  }
  if (!stats_assigned) {
    layer_set_hidden(bitmap_layer_get_layer(s_battery_icon_layer), true);
    layer_set_hidden(text_layer_get_layer(s_battery_text_layer), true);
    layer_set_hidden(text_layer_get_layer(s_steps_count_layer), true);
    layer_set_hidden(text_layer_get_layer(s_steps_label_layer), true);
    layer_set_hidden(s_divider_layer, true);
  }
  
  // Update fonts based on background state (B&W only)
  update_fonts_for_background();
  
  // Update text colors based on background colors
  update_text_colors();
}

// Update time
static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Update time
  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", tick_time);
  
  // Remove leading zero for 12-hour format
  if (!clock_is_24h_style() && s_hour_buffer[0] == '0') {
    memmove(s_hour_buffer, s_hour_buffer + 1, sizeof(s_hour_buffer) - 1);
  }
  
  text_layer_set_text(s_hour_layer, s_hour_buffer);
  text_layer_set_text(s_minute_layer, s_minute_buffer);
  
  // Update date
  strftime(s_day_name_buffer, sizeof(s_day_name_buffer), "%a", tick_time);
  strftime(s_day_number_buffer, sizeof(s_day_number_buffer), "%e", tick_time);
  strftime(s_month_name_buffer, sizeof(s_month_name_buffer), "%b", tick_time);
  
  // Convert to uppercase
  for (int i = 0; s_day_name_buffer[i]; i++) {
    s_day_name_buffer[i] = toupper((unsigned char)s_day_name_buffer[i]);
  }
  for (int i = 0; s_month_name_buffer[i]; i++) {
    s_month_name_buffer[i] = toupper((unsigned char)s_month_name_buffer[i]);
  }
  
  text_layer_set_text(s_day_name_layer, s_day_name_buffer);
  text_layer_set_text(s_day_number_layer, s_day_number_buffer);
  text_layer_set_text(s_month_name_layer, s_month_name_buffer);
}

// Update battery
static void update_battery() {
  BatteryChargeState battery_state = battery_state_service_peek();
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", battery_state.charge_percent);
  text_layer_set_text(s_battery_text_layer, s_battery_buffer);
  
  // Update battery icon based on charge level and charging state
  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
  }
  
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
  
  s_battery_icon = gbitmap_create_with_resource(resource_id);
  bitmap_layer_set_bitmap(s_battery_icon_layer, s_battery_icon);
}

// Update steps using Health API
static void update_steps() {
  // Check if Health service is available
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  // Check if the metric is available
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    // Data is available
    int steps = (int)health_service_sum_today(metric);
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", steps);
  } else {
    // Data not available
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "--");
  }
  
  text_layer_set_text(s_steps_count_layer, s_steps_buffer);
}

// Health event handler
static void health_handler(HealthEventType event, void *context) {
  // Update step count when health data changes
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
}

// Update weather - will be updated via AppMessage
// Update weather - will be updated via AppMessage
static void update_weather() {
  // Placeholder - will be updated from phone
  snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "--°");
  text_layer_set_text(s_temperature_layer, s_temperature_buffer);
  text_layer_set_text(s_weather_condition_layer, "--");
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
    // Find the position after "partly"
    char* partly_pos = strstr(lower_condition, "partly");
    char* after_partly = partly_pos + 6; // "partly" is 6 characters
    
    // Skip whitespace
    while (*after_partly == ' ' && *after_partly != '\0') {
      after_partly++;
    }
    
    // Extract the next word
    if (*after_partly != '\0') {
      // Check what the second word is
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
  
  // Check for other weather patterns and return single words
  if (strstr(lower_condition, "sunny") || strstr(lower_condition, "clear")) {
    return (hour >= 17) ? "Clear" : "Sunny";
  }
  if (strstr(lower_condition, "cloudy") || strstr(lower_condition, "overcast")) return "Cloudy";
  if (strstr(lower_condition, "mist")) return "Mist";
  if (strstr(lower_condition, "fog")) return "Fog";
  if (strstr(lower_condition, "rain") || strstr(lower_condition, "drizzle") || strstr(lower_condition, "shower")) return "Rain";
  if (strstr(lower_condition, "snow") || strstr(lower_condition, "sleet") || strstr(lower_condition, "blizzard") || strstr(lower_condition, "ice")) return "Snow";
  if (strstr(lower_condition, "thunder") || strstr(lower_condition, "storm")) return "Storm";
  
  // Default fallback
  return "Unknown";
}

// Tick handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "=== TICK HANDLER CALLED ===");
  update_time();
  
  // Update weather every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_Temperature, 1);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather update request sent");
  }
}

// Battery callback
static void battery_callback(BatteryChargeState charge_state) {
  update_battery();
}

static uint32_t get_resource_id_for_weather_icon(int icon_id) {
  if (s_alt_weather_layout) {
    switch(icon_id) {
      case 0: return RESOURCE_ID_ICON_WEATHER_SUNNY_ALT;
      case 1: return RESOURCE_ID_ICON_WEATHER_PARTLY_CLOUDY_ALT;
      case 2: return RESOURCE_ID_ICON_WEATHER_CLOUDY_ALT;
      case 3: return RESOURCE_ID_ICON_WEATHER_LIGHT_RAIN_ALT;
      case 4: return RESOURCE_ID_ICON_WEATHER_HEAVY_RAIN_ALT;
      case 5: return RESOURCE_ID_ICON_WEATHER_LIGHT_SNOW_ALT;
      case 6: return RESOURCE_ID_ICON_WEATHER_HEAVY_SNOW_ALT;
      case 7: return RESOURCE_ID_ICON_WEATHER_RAIN_SNOW_ALT;
      case 8: return RESOURCE_ID_ICON_WEATHER_CLEAR_NIGHT_ALT;
      case 9: return RESOURCE_ID_ICON_WEATHER_NIGHT_PARTLY_CLOUDY_ALT;
      case 10: return RESOURCE_ID_ICON_WEATHER_NIGHT_CLOUDY_ALT;
      case 11: return RESOURCE_ID_ICON_WEATHER_NIGHT_LIGHT_RAIN_ALT;
      case 12: return RESOURCE_ID_ICON_WEATHER_NIGHT_HEAVY_RAIN_ALT;
      case 13: return RESOURCE_ID_ICON_WEATHER_NIGHT_LIGHT_SNOW_ALT;
      case 14: return RESOURCE_ID_ICON_WEATHER_NIGHT_HEAVY_SNOW_ALT;
      case 15: return RESOURCE_ID_ICON_WEATHER_NIGHT_RAIN_SNOW_ALT;
      default: return RESOURCE_ID_ICON_WEATHER_GENERIC_ALT;
    }
  } else {
    switch(icon_id) {
      case 0: return RESOURCE_ID_ICON_WEATHER_SUNNY;
      case 1: return RESOURCE_ID_ICON_WEATHER_PARTLY_CLOUDY;
      case 2: return RESOURCE_ID_ICON_WEATHER_CLOUDY;
      case 3: return RESOURCE_ID_ICON_WEATHER_LIGHT_RAIN;
      case 4: return RESOURCE_ID_ICON_WEATHER_HEAVY_RAIN;
      case 5: return RESOURCE_ID_ICON_WEATHER_LIGHT_SNOW;
      case 6: return RESOURCE_ID_ICON_WEATHER_HEAVY_SNOW;
      case 7: return RESOURCE_ID_ICON_WEATHER_RAIN_SNOW;
      case 8: return RESOURCE_ID_ICON_WEATHER_CLEAR_NIGHT;
      case 9: return RESOURCE_ID_ICON_WEATHER_NIGHT_PARTLY_CLOUDY;
      case 10: return RESOURCE_ID_ICON_WEATHER_NIGHT_CLOUDY;
      case 11: return RESOURCE_ID_ICON_WEATHER_NIGHT_LIGHT_RAIN;
      case 12: return RESOURCE_ID_ICON_WEATHER_NIGHT_HEAVY_RAIN;
      case 13: return RESOURCE_ID_ICON_WEATHER_NIGHT_LIGHT_SNOW;
      case 14: return RESOURCE_ID_ICON_WEATHER_NIGHT_HEAVY_SNOW;
      case 15: return RESOURCE_ID_ICON_WEATHER_NIGHT_RAIN_SNOW;
      default: return RESOURCE_ID_ICON_WEATHER_GENERIC;
    }
  }
}

// Inbox received callback
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "=== inbox_received_callback START ===");
  
  // Read weather data from phone
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_Temperature);
  Tuple *icon_tuple = dict_find(iterator, MESSAGE_KEY_WeatherIcon);
  Tuple *condition_tuple = dict_find(iterator, MESSAGE_KEY_Condition);
  
  if (temp_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature (Fahrenheit): %d", (int)temp_tuple->value->int32);
    s_current_temperature = (int)temp_tuple->value->int32;
    s_has_temperature = true;
    
    // Display temperature with unit conversion if needed
    int display_temp = s_current_temperature;
    char unit = 'F';
    if (s_use_celsius) {
    
    // Display temperature immediately with current s_use_celsius setting
    int display_temp = s_current_temperature;
    char unit = 'F';
    if (s_use_celsius) {
      display_temp = (s_current_temperature - 32) * 5 / 9;
      unit = 'C';
    }
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d°%c", display_temp, unit);
    text_layer_set_text(s_temperature_layer, s_temperature_buffer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature updated: %d°%c (use_celsius=%d)", display_temp, unit, s_use_celsius);
      // Convert Fahrenheit to Celsius: (F - 32) * 5 / 9
      display_temp = (s_current_temperature - 32) * 5 / 9;
      unit = 'C';
    }
    
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d°%c", display_temp, unit);
    text_layer_set_text(s_temperature_layer, s_temperature_buffer);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature tuple not found");
  }
  
  if (condition_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Condition: %s", condition_tuple->value->cstring);
    const char* single_word = get_single_word_condition(condition_tuple->value->cstring);
    snprintf(s_weather_condition_buffer, sizeof(s_weather_condition_buffer), "%s", single_word);
    text_layer_set_text(s_weather_condition_layer, s_weather_condition_buffer);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Condition tuple not found");
  }
  
  if (icon_tuple) {
    // Update weather icon
    if (s_weather_icon) {
      gbitmap_destroy(s_weather_icon);
    }
    
    int icon_id = (int)icon_tuple->value->int32;
    s_current_icon_id = icon_id;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather icon ID: %d", icon_id);
    uint32_t resource_id = get_resource_id_for_weather_icon(icon_id);
    
    s_weather_icon = gbitmap_create_with_resource(resource_id);
    bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Icon tuple not found");
  }
  
  // Read settings
  Tuple *temp_unit_tuple = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_tuple) {
    s_use_celsius = temp_unit_tuple->value->int32 == 1;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received TemperatureUnit: %d, setting s_use_celsius to: %d", (int)temp_unit_tuple->value->int32, s_use_celsius);
    // Persist this setting so it survives app restarts
    persist_write_bool(1, s_use_celsius);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Persisted s_use_celsius: %d", s_use_celsius);
    
    // If we have temperature data, update display with conversion
    if (s_has_temperature) {
      int display_temp = s_current_temperature;
      char unit = 'F';
      if (s_use_celsius) {
        // Convert Fahrenheit to Celsius: (F - 32) * 5 / 9
        display_temp = (s_current_temperature - 32) * 5 / 9;
        unit = 'C';
      }
      snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d°%c", display_temp, unit);
      text_layer_set_text(s_temperature_layer, s_temperature_buffer);
    }
  }
  
  // Read quadrant module assignments
  Tuple *q1_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant1Module);
  Tuple *q2_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant2Module);
  Tuple *q3_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant3Module);
  Tuple *q4_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant4Module);
  
  bool layout_changed = false;

  // Read Alt Weather Layout setting
  Tuple *alt_layout_tuple = dict_find(iterator, MESSAGE_KEY_AltWeatherLayout);
  if (alt_layout_tuple) {
    bool new_layout = alt_layout_tuple->value->int32 == 1;
    if (s_alt_weather_layout != new_layout) {
      s_alt_weather_layout = new_layout;
      persist_write_bool(PERSIST_KEY_ALT_WEATHER_LAYOUT, s_alt_weather_layout);
      layout_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Alt Weather Layout changed to: %d", s_alt_weather_layout);
      
      // Force reload of weather icon with new layout
      if (s_weather_icon) {
        gbitmap_destroy(s_weather_icon);
        s_weather_icon = NULL;
      }
      
      uint32_t resource_id;
      if (s_current_icon_id >= 0) {
        resource_id = get_resource_id_for_weather_icon(s_current_icon_id);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading icon %d with resource %d", s_current_icon_id, (int)resource_id);
      } else {
        resource_id = s_alt_weather_layout ? RESOURCE_ID_ICON_WEATHER_GENERIC_ALT : RESOURCE_ID_ICON_WEATHER_GENERIC;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Reloading generic icon with resource %d", (int)resource_id);
      }
      
      s_weather_icon = gbitmap_create_with_resource(resource_id);
      bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon);
    }
  }
  
  if (q1_tuple) {
    ModuleType new_module = (ModuleType)q1_tuple->value->int32;
    if (s_quadrant_modules[0] != new_module) {
      s_quadrant_modules[0] = new_module;
      persist_write_int(PERSIST_KEY_Q1_MODULE, new_module);
      layout_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q1 Module: %d", new_module);
    }
  }
  
  if (q2_tuple) {
    ModuleType new_module = (ModuleType)q2_tuple->value->int32;
    if (s_quadrant_modules[1] != new_module) {
      s_quadrant_modules[1] = new_module;
      persist_write_int(PERSIST_KEY_Q2_MODULE, new_module);
      layout_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q2 Module: %d", new_module);
    }
  }
  
  if (q3_tuple) {
    ModuleType new_module = (ModuleType)q3_tuple->value->int32;
    if (s_quadrant_modules[2] != new_module) {
      s_quadrant_modules[2] = new_module;
      persist_write_int(PERSIST_KEY_Q3_MODULE, new_module);
      layout_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q3 Module: %d", new_module);
    }
  }
  
  if (q4_tuple) {
    ModuleType new_module = (ModuleType)q4_tuple->value->int32;
    if (s_quadrant_modules[3] != new_module) {
      s_quadrant_modules[3] = new_module;
      persist_write_int(PERSIST_KEY_Q4_MODULE, new_module);
      layout_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q4 Module: %d", new_module);
    }
  }
  
  // Reposition layers if layout changed
  if (layout_changed) {
    reposition_layers();
  }
  
  // Read quadrant background settings
  Tuple *q1_bg_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant1Background);
  Tuple *q2_bg_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant2Background);
  Tuple *q3_bg_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant3Background);
  Tuple *q4_bg_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant4Background);
  
  bool background_changed = false;
  
  if (q1_bg_tuple) {
    bool new_bg = q1_bg_tuple->value->int32 == 1;
    if (s_quadrant_backgrounds[0] != new_bg) {
      s_quadrant_backgrounds[0] = new_bg;
      persist_write_bool(PERSIST_KEY_Q1_BACKGROUND, new_bg);
      background_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q1 Background: %d", new_bg);
    }
  }
  
  if (q2_bg_tuple) {
    bool new_bg = q2_bg_tuple->value->int32 == 1;
    if (s_quadrant_backgrounds[1] != new_bg) {
      s_quadrant_backgrounds[1] = new_bg;
      persist_write_bool(PERSIST_KEY_Q2_BACKGROUND, new_bg);
      background_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q2 Background: %d", new_bg);
    }
  }
  
  if (q3_bg_tuple) {
    bool new_bg = q3_bg_tuple->value->int32 == 1;
    if (s_quadrant_backgrounds[2] != new_bg) {
      s_quadrant_backgrounds[2] = new_bg;
      persist_write_bool(PERSIST_KEY_Q3_BACKGROUND, new_bg);
      background_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q3 Background: %d", new_bg);
    }
  }
  
  if (q4_bg_tuple) {
    bool new_bg = q4_bg_tuple->value->int32 == 1;
    if (s_quadrant_backgrounds[3] != new_bg) {
      s_quadrant_backgrounds[3] = new_bg;
      persist_write_bool(PERSIST_KEY_Q4_BACKGROUND, new_bg);
      background_changed = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Q4 Background: %d", new_bg);
    }
  }
  
#ifdef PBL_COLOR
  // Read quadrant color settings (color platforms only)
  Tuple *q1_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant1Color);
  Tuple *q2_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant2Color);
  Tuple *q3_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant3Color);
  Tuple *q4_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant4Color);
  
  if (q1_color_tuple) {
    s_quadrant_colors[0] = GColorFromHEX(q1_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q1_COLOR, q1_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q1 Color: 0x%x", (unsigned int)q1_color_tuple->value->int32);
  }
  
  if (q2_color_tuple) {
    s_quadrant_colors[1] = GColorFromHEX(q2_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q2_COLOR, q2_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q2 Color: 0x%x", (unsigned int)q2_color_tuple->value->int32);
  }
  
  if (q3_color_tuple) {
    s_quadrant_colors[2] = GColorFromHEX(q3_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q3_COLOR, q3_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q3 Color: 0x%x", (unsigned int)q3_color_tuple->value->int32);
  }
  
  if (q4_color_tuple) {
    s_quadrant_colors[3] = GColorFromHEX(q4_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q4_COLOR, q4_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q4 Color: 0x%x", (unsigned int)q4_color_tuple->value->int32);
  }
  
  // Read auto text color settings
  Tuple *q1_auto_text_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant1AutoTextColor);
  Tuple *q2_auto_text_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant2AutoTextColor);
  Tuple *q3_auto_text_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant3AutoTextColor);
  Tuple *q4_auto_text_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant4AutoTextColor);
  
  if (q1_auto_text_tuple) {
    s_auto_text_color[0] = (q1_auto_text_tuple->value->int32 == 1);
    persist_write_bool(PERSIST_KEY_Q1_AUTO_TEXT, s_auto_text_color[0]);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q1 Auto Text: %d", s_auto_text_color[0]);
  }
  
  if (q2_auto_text_tuple) {
    s_auto_text_color[1] = (q2_auto_text_tuple->value->int32 == 1);
    persist_write_bool(PERSIST_KEY_Q2_AUTO_TEXT, s_auto_text_color[1]);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q2 Auto Text: %d", s_auto_text_color[1]);
  }
  
  if (q3_auto_text_tuple) {
    s_auto_text_color[2] = (q3_auto_text_tuple->value->int32 == 1);
    persist_write_bool(PERSIST_KEY_Q3_AUTO_TEXT, s_auto_text_color[2]);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q3 Auto Text: %d", s_auto_text_color[2]);
  }
  
  if (q4_auto_text_tuple) {
    s_auto_text_color[3] = (q4_auto_text_tuple->value->int32 == 1);
    persist_write_bool(PERSIST_KEY_Q4_AUTO_TEXT, s_auto_text_color[3]);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q4 Auto Text: %d", s_auto_text_color[3]);
  }
  
  // Read custom text color settings
  Tuple *q1_text_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant1TextColor);
  Tuple *q2_text_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant2TextColor);
  Tuple *q3_text_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant3TextColor);
  Tuple *q4_text_color_tuple = dict_find(iterator, MESSAGE_KEY_Quadrant4TextColor);
  
  if (q1_text_color_tuple) {
    s_custom_text_color[0] = GColorFromHEX(q1_text_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q1_TEXT_COLOR, q1_text_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q1 Text Color: 0x%x", (unsigned int)q1_text_color_tuple->value->int32);
  }
  
  if (q2_text_color_tuple) {
    s_custom_text_color[1] = GColorFromHEX(q2_text_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q2_TEXT_COLOR, q2_text_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q2 Text Color: 0x%x", (unsigned int)q2_text_color_tuple->value->int32);
  }
  
  if (q3_text_color_tuple) {
    s_custom_text_color[2] = GColorFromHEX(q3_text_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q3_TEXT_COLOR, q3_text_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q3 Text Color: 0x%x", (unsigned int)q3_text_color_tuple->value->int32);
  }
  
  if (q4_text_color_tuple) {
    s_custom_text_color[3] = GColorFromHEX(q4_text_color_tuple->value->int32);
    persist_write_int(PERSIST_KEY_Q4_TEXT_COLOR, q4_text_color_tuple->value->int32);
    background_changed = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Q4 Text Color: 0x%x", (unsigned int)q4_text_color_tuple->value->int32);
  }
#endif
  
  // Redraw background if changed
  if (background_changed) {
    layer_mark_dirty(s_background_layer);
    // Update fonts for B&W platforms when background changes
    update_fonts_for_background();
    // Update text colors when background changes
    update_text_colors();
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "=== inbox_received_callback END ===");
}

// Inbox dropped callback
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", (int)reason);
}

// Outbox failed callback
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason: %d", (int)reason);
}

// Outbox sent callback
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success!");
}

// Main window load
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  // GRect bounds = layer_get_bounds(window_layer);
  
  // Create background layer
  s_background_layer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(s_background_layer, background_layer_update_proc);
  layer_add_child(window_layer, s_background_layer);
  
  // QUADRANT 1 - DATE (Top Left) - centered vertically in quadrant
  s_day_name_layer = text_layer_create(GRect(0, 3, 72, 14));
  text_layer_set_background_color(s_day_name_layer, GColorClear);
  text_layer_set_text_color(s_day_name_layer, GColorBlack);
  text_layer_set_font(s_day_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_day_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_day_name_layer));
  
  s_day_number_layer = text_layer_create(GRect(0, 15, 72, 51));
  text_layer_set_background_color(s_day_number_layer, GColorClear);
  text_layer_set_text_color(s_day_number_layer, GColorBlack);
  text_layer_set_font(s_day_number_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_day_number_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_day_number_layer));
  
  s_month_name_layer = text_layer_create(GRect(0, 62, 72, 71));
  text_layer_set_background_color(s_month_name_layer, GColorClear);
  text_layer_set_text_color(s_month_name_layer, GColorBlack);
  text_layer_set_font(s_month_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_month_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_month_name_layer));
  
  // QUADRANT 2 - WEATHER (Top Right) - moved up 10 pixels, removed condition text
  s_weather_icon_layer = bitmap_layer_create(GRect(81, 0, 54, 54));
  bitmap_layer_set_background_color(s_weather_icon_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_weather_icon_layer, GCompOpSet);
  s_weather_icon = gbitmap_create_with_resource(s_alt_weather_layout ? RESOURCE_ID_ICON_WEATHER_GENERIC_ALT : RESOURCE_ID_ICON_WEATHER_GENERIC);
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_icon_layer));
  
  s_temperature_layer = text_layer_create(GRect(72, 54, 72, 30));
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_text_color(s_temperature_layer, GColorBlack);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));
  
  s_weather_condition_layer = text_layer_create(GRect(72, 84, 72, 0));
  text_layer_set_background_color(s_weather_condition_layer, GColorClear);
  text_layer_set_text_color(s_weather_condition_layer, GColorBlack);
  text_layer_set_font(s_weather_condition_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_weather_condition_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_weather_condition_layer));
  
  // QUADRANT 3 - TIME (Bottom Left) - centered vertically within Q3 bounds
  s_hour_layer = text_layer_create(GRect(0, 90, 72, 126));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_text_color(s_hour_layer, GColorBlack);
  text_layer_set_font(s_hour_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
  
  s_minute_layer = text_layer_create(GRect(0, 128, 72, 164));
  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_text_color(s_minute_layer, GColorDarkGray);
  text_layer_set_font(s_minute_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
  
  // QUADRANT 4 - STATS (Bottom Right)
  s_battery_icon_layer = bitmap_layer_create(GRect(86, 94, 16, 16));
  bitmap_layer_set_background_color(s_battery_icon_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_battery_icon_layer, GCompOpSet);
  s_battery_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_BATTERY_FULL);
  bitmap_layer_set_bitmap(s_battery_icon_layer, s_battery_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_battery_icon_layer));
  
  s_battery_text_layer = text_layer_create(GRect(104, 91, 40, 105));
  text_layer_set_background_color(s_battery_text_layer, GColorClear);
  text_layer_set_text_color(s_battery_text_layer, GColorBlack);
  text_layer_set_font(s_battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_text_layer));
  
  // Divider line layer
  s_divider_layer = layer_create(GRect(72, 84, 72, 84));
  layer_set_update_proc(s_divider_layer, divider_layer_update_proc);
  layer_add_child(window_layer, s_divider_layer);
  
  s_steps_count_layer = text_layer_create(GRect(72, 118, 72, 145));
  text_layer_set_background_color(s_steps_count_layer, GColorClear);
  text_layer_set_text_color(s_steps_count_layer, GColorBlack);
  text_layer_set_font(s_steps_count_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_steps_count_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_steps_count_layer));
  
  s_steps_label_layer = text_layer_create(GRect(72, 144, 72, 160));
  text_layer_set_background_color(s_steps_label_layer, GColorClear);
  text_layer_set_text_color(s_steps_label_layer, GColorBlack);
  text_layer_set_font(s_steps_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_steps_label_layer, GTextAlignmentCenter);
  text_layer_set_text(s_steps_label_layer, "STEPS");
  layer_add_child(window_layer, text_layer_get_layer(s_steps_label_layer));
  
  // Initialize displays
  update_time();
  update_battery();
  update_weather();
  update_steps();
  
  // Apply initial layout
  reposition_layers();
}

// Main window unload
static void main_window_unload(Window *window) {
  layer_destroy(s_background_layer);
  text_layer_destroy(s_day_name_layer);
  text_layer_destroy(s_day_number_layer);
  text_layer_destroy(s_month_name_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_weather_condition_layer);
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_battery_text_layer);
  text_layer_destroy(s_steps_count_layer);
  text_layer_destroy(s_steps_label_layer);
  layer_destroy(s_divider_layer);
  
  bitmap_layer_destroy(s_weather_icon_layer);
  bitmap_layer_destroy(s_battery_icon_layer);
  
  if (s_weather_icon) {
    gbitmap_destroy(s_weather_icon);
  }
  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
  }
}

// Init
static void init() {
  // Load persisted module assignments
  if (persist_exists(PERSIST_KEY_Q1_MODULE)) {
    s_quadrant_modules[0] = (ModuleType)persist_read_int(PERSIST_KEY_Q1_MODULE);
  }
  if (persist_exists(PERSIST_KEY_Q2_MODULE)) {
    s_quadrant_modules[1] = (ModuleType)persist_read_int(PERSIST_KEY_Q2_MODULE);
  }
  if (persist_exists(PERSIST_KEY_Q3_MODULE)) {
    s_quadrant_modules[2] = (ModuleType)persist_read_int(PERSIST_KEY_Q3_MODULE);
  }
  if (persist_exists(PERSIST_KEY_Q4_MODULE)) {
    s_quadrant_modules[3] = (ModuleType)persist_read_int(PERSIST_KEY_Q4_MODULE);
  }
  
  // Load persisted background settings
  if (persist_exists(PERSIST_KEY_Q1_BACKGROUND)) {
    s_quadrant_backgrounds[0] = persist_read_bool(PERSIST_KEY_Q1_BACKGROUND);
  }
  if (persist_exists(PERSIST_KEY_Q2_BACKGROUND)) {
    s_quadrant_backgrounds[1] = persist_read_bool(PERSIST_KEY_Q2_BACKGROUND);
  }
  if (persist_exists(PERSIST_KEY_Q3_BACKGROUND)) {
    s_quadrant_backgrounds[2] = persist_read_bool(PERSIST_KEY_Q3_BACKGROUND);
  }
  if (persist_exists(PERSIST_KEY_Q4_BACKGROUND)) {
    s_quadrant_backgrounds[3] = persist_read_bool(PERSIST_KEY_Q4_BACKGROUND);
  }
  
#ifdef PBL_COLOR
  // Load persisted colors (default to light gray if not set)
  s_quadrant_colors[0] = persist_exists(PERSIST_KEY_Q1_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q1_COLOR)) : GColorLightGray;
  s_quadrant_colors[1] = persist_exists(PERSIST_KEY_Q2_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q2_COLOR)) : GColorLightGray;
  s_quadrant_colors[2] = persist_exists(PERSIST_KEY_Q3_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q3_COLOR)) : GColorLightGray;
  s_quadrant_colors[3] = persist_exists(PERSIST_KEY_Q4_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q4_COLOR)) : GColorLightGray;
  
  // Load persisted auto text color settings (default to true)
  if (persist_exists(PERSIST_KEY_Q1_AUTO_TEXT)) {
    s_auto_text_color[0] = persist_read_bool(PERSIST_KEY_Q1_AUTO_TEXT);
  }
  if (persist_exists(PERSIST_KEY_Q2_AUTO_TEXT)) {
    s_auto_text_color[1] = persist_read_bool(PERSIST_KEY_Q2_AUTO_TEXT);
  }
  if (persist_exists(PERSIST_KEY_Q3_AUTO_TEXT)) {
    s_auto_text_color[2] = persist_read_bool(PERSIST_KEY_Q3_AUTO_TEXT);
  }
  if (persist_exists(PERSIST_KEY_Q4_AUTO_TEXT)) {
    s_auto_text_color[3] = persist_read_bool(PERSIST_KEY_Q4_AUTO_TEXT);
  }
  
  // Load persisted custom text colors (default to black)
  s_custom_text_color[0] = persist_exists(PERSIST_KEY_Q1_TEXT_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q1_TEXT_COLOR)) : GColorBlack;
  s_custom_text_color[1] = persist_exists(PERSIST_KEY_Q2_TEXT_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q2_TEXT_COLOR)) : GColorBlack;
  s_custom_text_color[2] = persist_exists(PERSIST_KEY_Q3_TEXT_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q3_TEXT_COLOR)) : GColorBlack;
  s_custom_text_color[3] = persist_exists(PERSIST_KEY_Q4_TEXT_COLOR) ? 
    GColorFromHEX(persist_read_int(PERSIST_KEY_Q4_TEXT_COLOR)) : GColorBlack;
#endif
  
  // Load persisted settings from storage
  if (persist_exists(1)) {
    s_use_celsius = persist_read_bool(1);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded persisted s_use_celsius: %d", s_use_celsius);
  }
  
  if (persist_exists(PERSIST_KEY_ALT_WEATHER_LAYOUT)) {
    s_alt_weather_layout = persist_read_bool(PERSIST_KEY_ALT_WEATHER_LAYOUT);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded persisted s_alt_weather_layout: %d", s_alt_weather_layout);
  }
  
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
  
  // Subscribe to health service if available
  if (health_service_events_subscribe(health_handler, NULL)) {
    // Force initial update of steps
    update_steps();
  }
  
  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(512, 512);
}

// Deinit
static void deinit() {
  health_service_events_unsubscribe();
  window_destroy(s_main_window);
}

// Main
int main(void) {
  init();
  app_event_loop();
  deinit();
}
