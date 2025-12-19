# Celsius Temperature Unit Fix

## Problem
The Celsius temperature unit setting would revert to Fahrenheit after periodic weather updates, even though the user had set it to Celsius. The issue occurred when:
1. User set temperature unit to Celsius in the Clay settings UI
2. First weather update would correctly display Celsius
3. After ~26 minutes (when the phone's localStorage was cleared), subsequent weather updates would revert to Fahrenheit

## Root Cause
The issue had two parts:
1. **Phone-side**: The Pebble companion app's localStorage was being cleared by the system after ~26 minutes, causing the TemperatureUnit setting to be lost between weather updates
2. **Watch-side**: The watch didn't persist the temperature unit setting locally, so it relied entirely on the phone sending it with each weather update

## Solution
Instead of relying on the phone's localStorage which could be cleared, the **watch now persists the temperature unit setting locally** using Pebble's persistent storage API.

### Changes Made

#### C Code (Watch-side) - `src/c/modules.c`
1. **Load persisted setting on app startup**:
   - When the app initializes, it loads the previously stored temperature unit preference from persistent storage
   - Falls back to Fahrenheit (false) if no prior setting exists

2. **Save setting when received from phone**:
   - When the phone sends a TemperatureUnit message, the watch now persists this value locally
   - Uses `persist_write_bool(1, s_use_celsius)` to store the setting

3. **Display correct temperature regardless of source**:
   - The watch always uses its locally stored `s_use_celsius` setting to convert Fahrenheit to Celsius for display
   - Even if the phone's settings are lost, the watch remembers the user's preference

#### JavaScript Code (Phone-side) - `src/pkjs/index.js`
1. **Flatten settings before storage**:
   - Clay stores settings with nested `.value` properties
   - Settings are now flattened before storing to ensure consistent format

2. **Always read fresh settings from localStorage**:
   - Instead of relying on a global settings variable (which could become stale)
   - Each weather fetch reads fresh settings directly from localStorage

3. **Send TemperatureUnit with every weather update**:
   - Every weather data message includes the TemperatureUnit setting
   - This ensures the watch receives updates even if it missed a previous message

## Testing
To verify the fix works:
1. Set temperature unit to **Celsius** in the settings UI
2. Observe that weather displays in Celsius (e.g., 9°C)
3. Wait for periodic weather updates
4. Temperature should continue displaying in Celsius even after app restarts or settings are cleared on the phone

## Technical Details
- **Pebble persistent storage key**: `1` (used for `s_use_celsius`)
- **Storage method**: `persist_write_bool()` / `persist_read_bool()`
- **Fallback value**: Fahrenheit (false) if no persistent value exists
- **Conversion formula**: `(F - 32) * 5 / 9 = C` (integer division)
