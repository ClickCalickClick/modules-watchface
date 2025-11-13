{\rtf1\ansi\ansicpg1252\cocoartf2822
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;}
{\colortbl;\red255\green255\blue255;}
{\*\expandedcolortbl;;}
\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs24 \cf0 # Modules Watchface - Pebble 2 Build Guide for Claude LLM\
\
## Project Overview\
Build a watchface called "Modules" for Pebble 2 (monochrome, 144x168 pixels) using C and the Pebble SDK.\
This is a 2x2 grid layout with 4 quadrants, each displaying different data that can be configured via Clay Settings.\
\
---\
\
## Display Specifications\
- **Resolution**: 144px (width) \'d7 168px (height)\
- **Color Mode**: Black and White (monochrome)\
- **Grid Layout**: 2 columns \'d7 2 rows\
- **Quadrant Size**: 72px \'d7 84px each\
\
---\
\
## Quadrant Layout & Exact Coordinates\
\
### QUADRANT 1 - TOP LEFT (DATE)\
**Position**: (0, 0) to (72, 84)\
**Background**: GColorWhite\
**Border**: 1px black line on right edge (x=71) and bottom edge (y=83)\
\
**Elements**:\
1. **Day Name Text Layer**\
   - Position: (0, 8, 72, 22) - x, y, width, height\
   - Font: FONT_KEY_GOTHIC_14_BOLD (or fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD))\
   - Text: Day name in uppercase (e.g., "MON", "TUE", "WED")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
2. **Day Number Text Layer**\
   - Position: (0, 20, 72, 56)\
   - Font: FONT_KEY_BITHAM_42_BOLD\
   - Text: Day of month (e.g., "1", "15", "28")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
3. **Month Name Text Layer**\
   - Position: (0, 64, 72, 78)\
   - Font: FONT_KEY_GOTHIC_14_BOLD\
   - Text: Month in uppercase (e.g., "JAN", "FEB", "MAR")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
---\
\
### QUADRANT 2 - TOP RIGHT (WEATHER)\
**Position**: (72, 0) to (144, 84)\
**Background**: GColorLightGray\
**Border**: 1px black line on bottom edge (y=83)\
\
**Elements**:\
1. **Weather Icon Bitmap Layer**\
   - Position: (94, 12, 28, 28) - centered horizontally in quadrant\
   - Size: 28px \'d7 28px\
   - Format: 1-bit BMP (black and white)\
   - Resource Path: RESOURCE_ID_WEATHER_SUNNY (or appropriate weather condition)\
   - Compositing Mode: GCompOpSet\
   - Note: Icon should be centered at x=72+36-14=94 pixels from left\
\
2. **Temperature Text Layer**\
   - Position: (72, 42, 72, 64)\
   - Font: FONT_KEY_GOTHIC_28_BOLD\
   - Text: Temperature + degree symbol (e.g., "72\'b0")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
3. **Condition Text Layer**\
   - Position: (72, 68, 72, 80)\
   - Font: FONT_KEY_GOTHIC_14\
   - Text: Weather condition in uppercase (e.g., "SUNNY", "RAIN")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
---\
\
### QUADRANT 3 - BOTTOM LEFT (TIME)\
**Position**: (0, 84) to (72, 168)\
**Background**: GColorLightGray\
**Border**: 1px black line on right edge (x=71)\
\
**Elements**:\
1. **Hour Text Layer**\
   - Position: (0, 90, 72, 126)\
   - Font: FONT_KEY_BITHAM_42_BOLD\
   - Text: Hour in 12-hour or 24-hour format (e.g., "09" or "21")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
2. **Minute Text Layer**\
   - Position: (0, 128, 72, 164)\
   - Font: FONT_KEY_BITHAM_42_BOLD\
   - Text: Minutes with leading zero (e.g., "05", "42")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorDarkGray (to differentiate from hours)\
   - Background: GColorClear\
\
---\
\
### QUADRANT 4 - BOTTOM RIGHT (STATS)\
**Position**: (72, 84) to (144, 168)\
**Background**: GColorWhite\
\
**Elements**:\
1. **Battery Icon Bitmap Layer**\
   - Position: (86, 94, 16, 16)\
   - Size: 16px \'d7 16px\
   - Format: 1-bit BMP\
   - Resource Path: RESOURCE_ID_BATTERY_ICON\
   - Compositing Mode: GCompOpSet\
\
2. **Battery Percentage Text Layer**\
   - Position: (104, 91, 40, 105)\
   - Font: FONT_KEY_GOTHIC_18_BOLD\
   - Text: Battery percentage (e.g., "85%")\
   - Alignment: GTextAlignmentLeft\
   - Color: GColorBlack\
   - Background: GColorClear\
\
3. **Divider Line**\
   - Type: graphics_draw_line() in layer update proc\
   - Start Point: (78, 116)\
   - End Point: (138, 116)\
   - Color: GColorBlack\
   - Stroke Width: 1px\
\
4. **Steps Count Text Layer**\
   - Position: (72, 122, 72, 145)\
   - Font: FONT_KEY_GOTHIC_24_BOLD\
   - Text: Step count formatted with comma (e.g., "6,842")\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorBlack\
   - Background: GColorClear\
\
5. **Steps Label Text Layer**\
   - Position: (72, 144, 72, 160)\
   - Font: FONT_KEY_GOTHIC_14\
   - Text: "STEPS" in uppercase\
   - Alignment: GTextAlignmentCenter\
   - Color: GColorDarkGray\
   - Background: GColorClear\
\
---\
\
## Pebble SDK Fonts Reference\
\
Use these exact font constants from the Pebble SDK:\
\
```c\
// Available system fonts for this design:\
FONT_KEY_GOTHIC_14          // 14px regular\
FONT_KEY_GOTHIC_14_BOLD     // 14px bold - used for labels\
FONT_KEY_GOTHIC_18_BOLD     // 18px bold - used for battery %\
FONT_KEY_GOTHIC_24_BOLD     // 24px bold - used for steps count\
FONT_KEY_GOTHIC_28_BOLD     // 28px bold - used for temperature\
FONT_KEY_BITHAM_42_BOLD     // 42px bold - used for time and date number\
```\
\
Access fonts in code:\
```c\
GFont font_label = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);\
GFont font_time = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);\
```\
\
---\
\
## Icons & Resources\
\
### Icon Specifications\
All icons must be 1-bit BMP format (black and white, no transparency in traditional sense - use white as "transparent").\
\
**Required Icons**:\
1. **Weather Icons** (28\'d728 pixels each):\
   - `RESOURCE_ID_WEATHER_SUNNY` - Sun icon\
   - `RESOURCE_ID_WEATHER_CLOUDY` - Cloud icon\
   - `RESOURCE_ID_WEATHER_RAIN` - Rain cloud icon\
   - `RESOURCE_ID_WEATHER_SNOW` - Snow icon\
   - `RESOURCE_ID_WEATHER_CLEAR_NIGHT` - Moon icon\
\
2. **Battery Icon** (16\'d716 pixels):\
   - `RESOURCE_ID_BATTERY_ICON` - Battery outline\
\
### Where to Get Icons\
1. **Create Custom Icons**: Use image editor (GIMP, Photoshop) and export as 1-bit BMP\
2. **Pebble Icon Resources**: Check https://github.com/pebble-examples for sample resources\
3. **Font Awesome to BMP**: Convert Font Awesome icons to 1-bit BMP at specified sizes\
4. **Online BMP Converters**: Use online tools to convert PNG/SVG to 1-bit BMP\
\
### Adding to appinfo.json\
```json\
"resources": \{\
  "media": [\
    \{\
      "type": "bitmap",\
      "name": "WEATHER_SUNNY",\
      "file": "images/weather_sunny.bmp"\
    \},\
    \{\
      "type": "bitmap",\
      "name": "BATTERY_ICON",\
      "file": "images/battery.bmp"\
    \}\
  ]\
\}\
```\
\
---\
\
## C Code Structure\
\
### Required Includes\
```c\
#include <pebble.h>\
```\
\
### Global Variables (Main Window & Layers)\
```c\
static Window *s_main_window;\
\
// Quadrant 1 - Date\
static TextLayer *s_day_name_layer;\
static TextLayer *s_day_number_layer;\
static TextLayer *s_month_name_layer;\
\
// Quadrant 2 - Weather\
static BitmapLayer *s_weather_icon_layer;\
static GBitmap *s_weather_icon_bitmap;\
static TextLayer *s_temperature_layer;\
static TextLayer *s_condition_layer;\
\
// Quadrant 3 - Time\
static TextLayer *s_hour_layer;\
static TextLayer *s_minute_layer;\
\
// Quadrant 4 - Stats\
static BitmapLayer *s_battery_icon_layer;\
static GBitmap *s_battery_icon_bitmap;\
static TextLayer *s_battery_text_layer;\
static TextLayer *s_steps_count_layer;\
static TextLayer *s_steps_label_layer;\
static Layer *s_divider_layer; // For drawing the horizontal line\
\
// Buffers for text\
static char s_day_name_buffer[4];\
static char s_day_number_buffer[3];\
static char s_month_name_buffer[4];\
static char s_temperature_buffer[8];\
static char s_condition_buffer[12];\
static char s_hour_buffer[3];\
static char s_minute_buffer[3];\
static char s_battery_buffer[6];\
static char s_steps_buffer[12];\
```\
\
### Main Window Load Function\
```c\
static void main_window_load(Window *window) \{\
  Layer *window_layer = window_get_root_layer(window);\
  GRect bounds = layer_get_bounds(window_layer);\
  \
  // QUADRANT 1 - DATE (Top Left)\
  s_day_name_layer = text_layer_create(GRect(0, 8, 72, 22));\
  text_layer_set_background_color(s_day_name_layer, GColorClear);\
  text_layer_set_text_color(s_day_name_layer, GColorBlack);\
  text_layer_set_font(s_day_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));\
  text_layer_set_text_alignment(s_day_name_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_day_name_layer));\
  \
  s_day_number_layer = text_layer_create(GRect(0, 20, 72, 56));\
  text_layer_set_background_color(s_day_number_layer, GColorClear);\
  text_layer_set_text_color(s_day_number_layer, GColorBlack);\
  text_layer_set_font(s_day_number_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));\
  text_layer_set_text_alignment(s_day_number_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_day_number_layer));\
  \
  s_month_name_layer = text_layer_create(GRect(0, 64, 72, 78));\
  text_layer_set_background_color(s_month_name_layer, GColorClear);\
  text_layer_set_text_color(s_month_name_layer, GColorBlack);\
  text_layer_set_font(s_month_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));\
  text_layer_set_text_alignment(s_month_name_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_month_name_layer));\
  \
  // QUADRANT 2 - WEATHER (Top Right)\
  s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_SUNNY);\
  s_weather_icon_layer = bitmap_layer_create(GRect(94, 12, 28, 28));\
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon_bitmap);\
  bitmap_layer_set_compositing_mode(s_weather_icon_layer, GCompOpSet);\
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_icon_layer));\
  \
  s_temperature_layer = text_layer_create(GRect(72, 42, 72, 64));\
  text_layer_set_background_color(s_temperature_layer, GColorClear);\
  text_layer_set_text_color(s_temperature_layer, GColorBlack);\
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));\
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));\
  \
  s_condition_layer = text_layer_create(GRect(72, 68, 72, 80));\
  text_layer_set_background_color(s_condition_layer, GColorClear);\
  text_layer_set_text_color(s_condition_layer, GColorBlack);\
  text_layer_set_font(s_condition_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));\
  text_layer_set_text_alignment(s_condition_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_condition_layer));\
  \
  // QUADRANT 3 - TIME (Bottom Left)\
  s_hour_layer = text_layer_create(GRect(0, 90, 72, 126));\
  text_layer_set_background_color(s_hour_layer, GColorClear);\
  text_layer_set_text_color(s_hour_layer, GColorBlack);\
  text_layer_set_font(s_hour_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));\
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));\
  \
  s_minute_layer = text_layer_create(GRect(0, 128, 72, 164));\
  text_layer_set_background_color(s_minute_layer, GColorClear);\
  text_layer_set_text_color(s_minute_layer, GColorDarkGray);\
  text_layer_set_font(s_minute_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));\
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));\
  \
  // QUADRANT 4 - STATS (Bottom Right)\
  s_battery_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON);\
  s_battery_icon_layer = bitmap_layer_create(GRect(86, 94, 16, 16));\
  bitmap_layer_set_bitmap(s_battery_icon_layer, s_battery_icon_bitmap);\
  bitmap_layer_set_compositing_mode(s_battery_icon_layer, GCompOpSet);\
  layer_add_child(window_layer, bitmap_layer_get_layer(s_battery_icon_layer));\
  \
  s_battery_text_layer = text_layer_create(GRect(104, 91, 40, 105));\
  text_layer_set_background_color(s_battery_text_layer, GColorClear);\
  text_layer_set_text_color(s_battery_text_layer, GColorBlack);\
  text_layer_set_font(s_battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));\
  text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentLeft);\
  layer_add_child(window_layer, text_layer_get_layer(s_battery_text_layer));\
  \
  // Create divider layer for horizontal line\
  s_divider_layer = layer_create(GRect(72, 84, 72, 84));\
  layer_set_update_proc(s_divider_layer, divider_layer_update_proc);\
  layer_add_child(window_layer, s_divider_layer);\
  \
  s_steps_count_layer = text_layer_create(GRect(72, 122, 72, 145));\
  text_layer_set_background_color(s_steps_count_layer, GColorClear);\
  text_layer_set_text_color(s_steps_count_layer, GColorBlack);\
  text_layer_set_font(s_steps_count_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));\
  text_layer_set_text_alignment(s_steps_count_layer, GTextAlignmentCenter);\
  layer_add_child(window_layer, text_layer_get_layer(s_steps_count_layer));\
  \
  s_steps_label_layer = text_layer_create(GRect(72, 144, 72, 160));\
  text_layer_set_background_color(s_steps_label_layer, GColorClear);\
  text_layer_set_text_color(s_steps_label_layer, GColorDarkGray);\
  text_layer_set_font(s_steps_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));\
  text_layer_set_text_alignment(s_steps_label_layer, GTextAlignmentCenter);\
  text_layer_set_text(s_steps_label_layer, "STEPS");\
  layer_add_child(window_layer, text_layer_get_layer(s_steps_label_layer));\
\}\
```\
\
### Divider Line Drawing\
```c\
static void divider_layer_update_proc(Layer *layer, GContext *ctx) \{\
  graphics_context_set_stroke_color(ctx, GColorBlack);\
  graphics_draw_line(ctx, GPoint(6, 32), GPoint(66, 32)); // Relative to layer position\
\}\
```\
\
### Update Functions\
```c\
static void update_time() \{\
  time_t temp = time(NULL);\
  struct tm *tick_time = localtime(&temp);\
  \
  // Update time\
  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);\
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", tick_time);\
  \
  // Remove leading zero for 12-hour format\
  if (!clock_is_24h_style() && s_hour_buffer[0] == '0') \{\
    memmove(s_hour_buffer, s_hour_buffer + 1, sizeof(s_hour_buffer) - 1);\
  \}\
  \
  text_layer_set_text(s_hour_layer, s_hour_buffer);\
  text_layer_set_text(s_minute_layer, s_minute_buffer);\
  \
  // Update date\
  strftime(s_day_name_buffer, sizeof(s_day_name_buffer), "%a", tick_time);\
  strftime(s_day_number_buffer, sizeof(s_day_number_buffer), "%e", tick_time);\
  strftime(s_month_name_buffer, sizeof(s_month_name_buffer), "%b", tick_time);\
  \
  // Convert to uppercase\
  for (int i = 0; s_day_name_buffer[i]; i++) \{\
    s_day_name_buffer[i] = toupper((unsigned char)s_day_name_buffer[i]);\
  \}\
  for (int i = 0; s_month_name_buffer[i]; i++) \{\
    s_month_name_buffer[i] = toupper((unsigned char)s_month_name_buffer[i]);\
  \}\
  \
  text_layer_set_text(s_day_name_layer, s_day_name_buffer);\
  text_layer_set_text(s_day_number_layer, s_day_number_buffer);\
  text_layer_set_text(s_month_name_layer, s_month_name_buffer);\
\}\
\
static void update_battery() \{\
  BatteryChargeState battery_state = battery_state_service_peek();\
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", battery_state.charge_percent);\
  text_layer_set_text(s_battery_text_layer, s_battery_buffer);\
\}\
\
static void update_steps() \{\
  HealthMetric metric = HealthMetricStepCount;\
  time_t start = time_start_of_today();\
  time_t end = time(NULL);\
  \
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);\
  \
  if (mask & HealthServiceAccessibilityMaskAvailable) \{\
    int steps = (int)health_service_sum_today(metric);\
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", steps);\
    text_layer_set_text(s_steps_count_layer, s_steps_buffer);\
  \} else \{\
    text_layer_set_text(s_steps_count_layer, "N/A");\
  \}\
\}\
\
static void update_weather() \{\
  // This would be updated via AppMessage from phone\
  // For now, set placeholder\
  snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "72\'b0");\
  text_layer_set_text(s_temperature_layer, s_temperature_buffer);\
  text_layer_set_text(s_condition_layer, "SUNNY");\
\}\
```\
\
### Event Handlers\
```c\
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) \{\
  update_time();\
  \
  // Update weather every 30 minutes\
  if (tick_time->tm_min % 30 == 0) \{\
    update_weather();\
  \}\
\}\
\
static void battery_handler(BatteryChargeState charge_state) \{\
  update_battery();\
\}\
\
static void health_handler(HealthEventType event, void *context) \{\
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) \{\
    update_steps();\
  \}\
\}\
```\
\
---\
\
## Clay Settings Configuration\
\
### Configurable Module Options\
\
Create a `config.js` file for Clay Settings with these options:\
\
**Available Data Modules** (based on Pebble SDK):\
\
1. **Time Modules**:\
   - Hour (12h or 24h format)\
   - Minute\
   - Second\
   - AM/PM indicator\
\
2. **Date Modules**:\
   - Day of week (short/long)\
   - Day of month\
   - Month name (short/long)\
   - Week number\
   - Year\
\
3. **Weather Modules** (via AppMessage from phone):\
   - Temperature (\'b0F or \'b0C)\
   - Weather condition (text)\
   - Weather icon\
   - Location name\
   - Humidity percentage\
   - Wind speed\
\
4. **Battery Modules**:\
   - Battery percentage\
   - Battery icon\
   - Charging indicator\
   - Battery bar graph\
\
5. **Health Modules** (requires Health API):\
   - Step count\
   - Distance walked (miles/km)\
   - Calories burned\
   - Active time (minutes)\
   - Sleep time\
   - Heart rate (if available on device)\
   - Average heart rate\
\
6. **Connectivity Modules**:\
   - Bluetooth connection status\
   - Bluetooth icon\
   - Phone battery level (via AppMessage)\
\
7. **Astronomical Modules**:\
   - Sunrise time\
   - Sunset time\
   - Moon phase\
\
8. **Calendar Modules** (via AppMessage):\
   - Next appointment time\
   - Next appointment title\
\
### Clay Configuration Structure\
```javascript\
module.exports = [\
  \{\
    "type": "heading",\
    "defaultValue": "Modules Watchface Configuration"\
  \},\
  \{\
    "type": "section",\
    "items": [\
      \{\
        "type": "heading",\
        "defaultValue": "Quadrant 1 - Top Left"\
      \},\
      \{\
        "type": "select",\
        "messageKey": "Quadrant1Module",\
        "label": "Display Module",\
        "defaultValue": "date",\
        "options": [\
          \{ "label": "Date (Day Name, Number, Month)", "value": "date" \},\
          \{ "label": "Week Number", "value": "weeknum" \},\
          \{ "label": "Battery Stats", "value": "battery" \},\
          \{ "label": "Step Count", "value": "steps" \},\
          \{ "label": "Disabled", "value": "none" \}\
        ]\
      \}\
    ]\
  \},\
  \{\
    "type": "section",\
    "items": [\
      \{\
        "type": "heading",\
        "defaultValue": "Quadrant 2 - Top Right"\
      \},\
      \{\
        "type": "select",\
        "messageKey": "Quadrant2Module",\
        "label": "Display Module",\
        "defaultValue": "weather",\
        "options": [\
          \{ "label": "Weather (Temp + Icon)", "value": "weather" \},\
          \{ "label": "Sunrise/Sunset", "value": "sundata" \},\
          \{ "label": "Heart Rate", "value": "heartrate" \},\
          \{ "label": "Next Appointment", "value": "calendar" \},\
          \{ "label": "Disabled", "value": "none" \}\
        ]\
      \}\
    ]\
  \},\
  \{\
    "type": "section",\
    "items": [\
      \{\
        "type": "heading",\
        "defaultValue": "Quadrant 3 - Bottom Left"\
      \},\
      \{\
        "type": "select",\
        "messageKey": "Quadrant3Module",\
        "label": "Display Module",\
        "defaultValue": "time",\
        "options": [\
          \{ "label": "Time (Hour:Minute)", "value": "time" \},\
          \{ "label": "Time with Seconds", "value": "timesec" \},\
          \{ "label": "Date (Full)", "value": "datefull" \},\
          \{ "label": "Disabled", "value": "none" \}\
        ]\
      \},\
      \{\
        "type": "toggle",\
        "messageKey": "Use24HourFormat",\
        "label": "Use 24-Hour Format",\
        "defaultValue": false\
      \}\
    ]\
  \},\
  \{\
    "type": "section",\
    "items": [\
      \{\
        "type": "heading",\
        "defaultValue": "Quadrant 4 - Bottom Right"\
      \},\
      \{\
        "type": "select",\
        "messageKey": "Quadrant4Module",\
        "label": "Display Module",\
        "defaultValue": "stats",\
        "options": [\
          \{ "label": "Battery + Steps", "value": "stats" \},\
          \{ "label": "Battery Only", "value": "battery" \},\
          \{ "label": "Steps Only", "value": "steps" \},\
          \{ "label": "Distance Walked", "value": "distance" \},\
          \{ "label": "Calories Burned", "value": "calories" \},\
          \{ "label": "Bluetooth Status", "value": "bluetooth" \},\
          \{ "label": "Disabled", "value": "none" \}\
        ]\
      \}\
    ]\
  \},\
  \{\
    "type": "section",\
    "items": [\
      \{\
        "type": "heading",\
        "defaultValue": "Weather Settings"\
      \},\
      \{\
        "type": "input",\
        "messageKey": "WeatherAPIKey",\
        "label": "OpenWeather API Key",\
        "defaultValue": ""\
      \},\
      \{\
        "type": "toggle",\
        "messageKey": "UseCelsius",\
        "label": "Use Celsius",\
        "defaultValue": false\
      \}\
    ]\
  \},\
  \{\
    "type": "submit",\
    "defaultValue": "Save Settings"\
  \}\
];\
```\
\
### Reading Settings in C\
```c\
// In your main.c, add message keys to appinfo.json\
static void inbox_received_handler(DictionaryIterator *iter, void *context) \{\
  Tuple *quadrant1_tuple = dict_find(iter, MESSAGE_KEY_Quadrant1Module);\
  Tuple *quadrant2_tuple = dict_find(iter, MESSAGE_KEY_Quadrant2Module);\
  Tuple *quadrant3_tuple = dict_find(iter, MESSAGE_KEY_Quadrant3Module);\
  Tuple *quadrant4_tuple = dict_find(iter, MESSAGE_KEY_Quadrant4Module);\
  Tuple *use24h_tuple = dict_find(iter, MESSAGE_KEY_Use24HourFormat);\
  \
  if (quadrant1_tuple) \{\
    persist_write_int(MESSAGE_KEY_Quadrant1Module, quadrant1_tuple->value->int32);\
  \}\
  \
  // Similar for other settings...\
  \
  // Refresh display\
  main_window_unload(s_main_window);\
  main_window_load(s_main_window);\
\}\
```\
\
---\
\
## appinfo.json Configuration\
\
```json\
\{\
  "uuid": "YOUR-UUID-HERE",\
  "shortName": "Modules",\
  "longName": "Modules Watchface",\
  "companyName": "Your Name",\
  "versionLabel": "1.0",\
  "sdkVersion": "3",\
  "targetPlatforms": ["aplite", "basalt"],\
  "watchapp": \{\
    "watchface": true\
  \},\
  "appKeys": \{\
    "Quadrant1Module": 0,\
    "Quadrant2Module": 1,\
    "Quadrant3Module": 2,\
    "Quadrant4Module": 3,\
    "Use24HourFormat": 4,\
    "UseCelsius": 5,\
    "WeatherAPIKey": 6,\
    "WeatherTemp": 10,\
    "WeatherCondition": 11,\
    "WeatherIcon": 12\
  \},\
  "resources": \{\
    "media": [\
      \{\
        "type": "bitmap",\
        "name": "WEATHER_SUNNY",\
        "file": "images/weather_sunny.bmp"\
      \},\
      \{\
        "type": "bitmap",\
        "name": "WEATHER_CLOUDY",\
        "file": "images/weather_cloudy.bmp"\
      \},\
      \{\
        "type": "bitmap",\
        "name": "WEATHER_RAIN",\
        "file": "images/weather_rain.bmp"\
      \},\
      \{\
        "type": "bitmap",\
        "name": "BATTERY_ICON",\
        "file": "images/battery.bmp"\
      \}\
    ]\
  \},\
  "capabilities": [\
    "health"\
  ]\
\}\
```\
\
---\
\
## Background Drawing for Grid Lines\
\
To draw the grid separator lines between quadrants:\
\
```c\
// Create a background layer for drawing grid lines\
static Layer *s_background_layer;\
\
static void background_layer_update_proc(Layer *layer, GContext *ctx) \{\
  GRect bounds = layer_get_bounds(layer);\
  \
  // Fill quadrant backgrounds\
  graphics_context_set_fill_color(ctx, GColorWhite);\
  graphics_fill_rect(ctx, GRect(0, 0, 72, 84), 0, GCornerNone); // Q1\
  \
  graphics_context_set_fill_color(ctx, GColorLightGray);\
  graphics_fill_rect(ctx, GRect(72, 0, 72, 84), 0, GCornerNone); // Q2\
  graphics_fill_rect(ctx, GRect(0, 84, 72, 84), 0, GCornerNone); // Q3\
  \
  graphics_context_set_fill_color(ctx, GColorWhite);\
  graphics_fill_rect(ctx, GRect(72, 84, 72, 84), 0, GCornerNone); // Q4\
  \
  // Draw grid lines\
  graphics_context_set_stroke_color(ctx, GColorBlack);\
  graphics_context_set_stroke_width(ctx, 1);\
  \
  // Vertical line at x=72\
  graphics_draw_line(ctx, GPoint(72, 0), GPoint(72, 168));\
  \
  // Horizontal line at y=84\
  graphics_draw_line(ctx, GPoint(0, 84), GPoint(144, 84));\
\}\
\
// In main_window_load, create this FIRST before other layers:\
s_background_layer = layer_create(GRect(0, 0, 144, 168));\
layer_set_update_proc(s_background_layer, background_layer_update_proc);\
layer_add_child(window_layer, s_background_layer);\
```\
\
---\
\
## Complete Module Data Options from Pebble SDK\
\
### Time & Date (No External APIs Required)\
- `time(NULL)` - Current timestamp\
- `localtime()` - Convert to tm struct\
- `tick_time->tm_hour` - Hour (0-23)\
- `tick_time->tm_min` - Minute (0-59)\
- `tick_time->tm_sec` - Second (0-59)\
- `tick_time->tm_mday` - Day of month (1-31)\
- `tick_time->tm_mon` - Month (0-11)\
- `tick_time->tm_year` - Year since 1900\
- `tick_time->tm_wday` - Day of week (0-6, Sunday=0)\
- `strftime()` - Format time/date strings\
\
### Battery (No External APIs Required)\
```c\
BatteryChargeState battery_state = battery_state_service_peek();\
battery_state.charge_percent // 0-100\
battery_state.is_charging // bool\
battery_state.is_plugged // bool\
```\
\
### Health (Requires Health API - Pebble 2+ only)\
```c\
// Enable in appinfo.json: "capabilities": ["health"]\
HealthMetricStepCount\
HealthMetricActiveSeconds\
HealthMetricWalkedDistanceMeters\
HealthMetricSleepSeconds\
HealthMetricSleepRestfulSeconds\
HealthMetricRestingKCalories\
HealthMetricActiveKCalories\
HealthMetricHeartRateBPM // Requires HR sensor\
\
// Usage:\
int steps = (int)health_service_sum_today(HealthMetricStepCount);\
```\
\
### Bluetooth Connection (No External APIs Required)\
```c\
bool connected = connection_service_peek_pebble_app_connection();\
```\
\
### Weather, Calendar, Phone Battery (Requires AppMessage from Phone)\
These require a companion phone app using PebbleKit to send data via AppMessage.\
\
---\
\
## Build Instructions for Claude\
\
1. **Initialize Pebble Project**:\
   ```bash\
   pebble new-project modules-watchface\
   cd modules-watchface\
   ```\
\
2. **Create Directory Structure**:\
   ```\
   modules-watchface/\
   \uc0\u9500 \u9472 \u9472  src/\
   \uc0\u9474    \u9500 \u9472 \u9472  c/\
   \uc0\u9474    \u9474    \u9492 \u9472 \u9472  main.c\
   \uc0\u9474    \u9492 \u9472 \u9472  pkjs/\
   \uc0\u9474        \u9500 \u9472 \u9472  index.js\
   \uc0\u9474        \u9492 \u9472 \u9472  config.js\
   \uc0\u9500 \u9472 \u9472  resources/\
   \uc0\u9474    \u9492 \u9472 \u9472  images/\
   \uc0\u9474        \u9500 \u9472 \u9472  weather_sunny.bmp (28x28, 1-bit)\
   \uc0\u9474        \u9500 \u9472 \u9472  weather_cloudy.bmp (28x28, 1-bit)\
   \uc0\u9474        \u9500 \u9472 \u9472  weather_rain.bmp (28x28, 1-bit)\
   \uc0\u9474        \u9492 \u9472 \u9472  battery.bmp (16x16, 1-bit)\
   \uc0\u9500 \u9472 \u9472  appinfo.json\
   \uc0\u9492 \u9472 \u9472  package.json\
   ```\
\
3. **Implement main.c** with all code from sections above\
\
4. **Create BMPs** for icons at specified sizes (1-bit format)\
\
5. **Configure appinfo.json** with resources and message keys\
\
6. **Implement Clay config** in src/pkjs/config.js\
\
7. **Build**:\
   ```bash\
   pebble build\
   ```\
\
8. **Install on Pebble**:\
   ```bash\
   pebble install --emulator aplite\
   # or\
   pebble install --phone [phone-ip]\
   ```\
\
---\
\
## Testing Checklist\
\
- [ ] All text layers display correctly at specified coordinates\
- [ ] Fonts render at correct sizes (14, 18, 24, 28, 42pt)\
- [ ] Icons load and display at correct positions (28x28 weather, 16x16 battery)\
- [ ] Grid lines appear at x=72 (vertical) and y=84 (horizontal)\
- [ ] Quadrant backgrounds: Q1=white, Q2=lightgray, Q3=lightgray, Q4=white\
- [ ] Time updates every minute\
- [ ] Date updates at midnight\
- [ ] Battery percentage updates on change\
- [ ] Step count updates on health events\
- [ ] 12/24 hour format toggle works\
- [ ] Clay settings page opens and saves\
- [ ] Settings persist after watchface reload\
\
---\
\
## Additional Notes\
\
- **Memory Management**: Always destroy layers and bitmaps in `main_window_unload()`\
- **Performance**: Update only changed elements, not entire display\
- **Battery Life**: Use `TickTimeUnits` to subscribe only to needed time updates\
- **Accessibility**: Ensure text is readable on monochrome display with proper contrast\
- **Error Handling**: Check for NULL on layer creation and resource loading\
\
---\
\
This guide provides pixel-perfect specifications for building the Modules watchface. Follow each section precisely for accurate implementation.\
}