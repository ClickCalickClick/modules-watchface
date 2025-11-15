#include <pebble.h>
#include <ctype.h>
#include <string.h>
#include "config.h"

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
static int s_current_temperature = 0;
static bool s_has_temperature = false;

// Background layer update procedure
static void background_layer_update_proc(Layer *layer, GContext *ctx) {
  // GRect bounds = layer_get_bounds(layer);
  
  // Fill quadrant backgrounds
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, 72, 84), 0, GCornerNone); // Q1 - white
  
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(72, 0, 72, 84), 0, GCornerNone); // Q2 - light gray
  graphics_fill_rect(ctx, GRect(0, 84, 72, 84), 0, GCornerNone); // Q3 - light gray
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(72, 84, 72, 84), 0, GCornerNone); // Q4 - white
  
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
  update_time();
  
  // Update weather every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_Temperature, 1);
    app_message_outbox_send();
  }
}

// Battery callback
static void battery_callback(BatteryChargeState charge_state) {
  update_battery();
}

// Inbox received callback
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "=== inbox_received_callback START ===");
  
  // Read weather data from phone
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_Temperature);
  Tuple *icon_tuple = dict_find(iterator, MESSAGE_KEY_WeatherIcon);
  Tuple *condition_tuple = dict_find(iterator, MESSAGE_KEY_Condition);
  
  if (temp_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature: %d", (int)temp_tuple->value->int32);
    s_current_temperature = (int)temp_tuple->value->int32;
    s_has_temperature = true;
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d°%c", 
             s_current_temperature, s_use_celsius ? 'C' : 'F');
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
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather icon ID: %d", icon_id);
    uint32_t resource_id = RESOURCE_ID_ICON_WEATHER_GENERIC; // Default
    
    // Map icon IDs to new weather icon resources
    switch(icon_id) {
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
    
    s_weather_icon = gbitmap_create_with_resource(resource_id);
    bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Icon tuple not found");
  }
  
  // Read settings
  Tuple *temp_unit_tuple = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_tuple) {
    s_use_celsius = temp_unit_tuple->value->int32 == 1;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Use Celsius: %d", s_use_celsius);
    
    // If we have temperature data, just update the display format
    if (s_has_temperature) {
      snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%d°%c", 
               s_current_temperature, s_use_celsius ? 'C' : 'F');
      text_layer_set_text(s_temperature_layer, s_temperature_buffer);
    }
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
  s_weather_icon_layer = bitmap_layer_create(GRect(94, 2, 28, 28));
  bitmap_layer_set_background_color(s_weather_icon_layer, GColorClear);
  bitmap_layer_set_compositing_mode(s_weather_icon_layer, GCompOpSet);
  s_weather_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_WEATHER_GENERIC);
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_icon_layer));
  
  s_temperature_layer = text_layer_create(GRect(72, 30, 72, 54));
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_text_color(s_temperature_layer, GColorBlack);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));
  
  s_weather_condition_layer = text_layer_create(GRect(72, 55, 72, 68));
  text_layer_set_background_color(s_weather_condition_layer, GColorClear);
  text_layer_set_text_color(s_weather_condition_layer, GColorBlack);
  text_layer_set_font(s_weather_condition_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_weather_condition_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_weather_condition_layer));
  
  // QUADRANT 3 - TIME (Bottom Left) - centered vertically
  s_hour_layer = text_layer_create(GRect(0, 78, 72, 120));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_text_color(s_hour_layer, GColorBlack);
  text_layer_set_font(s_hour_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
  
  s_minute_layer = text_layer_create(GRect(0, 120, 72, 162));
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
  text_layer_set_text_color(s_steps_label_layer, GColorDarkGray);
  text_layer_set_font(s_steps_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_steps_label_layer, GTextAlignmentCenter);
  text_layer_set_text(s_steps_label_layer, "STEPS");
  layer_add_child(window_layer, text_layer_get_layer(s_steps_label_layer));
  
  // Initialize displays
  update_time();
  update_battery();
  update_weather();
  update_steps();
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
