# Modules Watchface for Pebble

A highly customizable modular watchface for Pebble smartwatches that displays time, date, weather, and fitness statistics in a clean, organized grid layout.

![Modules Watchface](https://via.placeholder.com/300x300?text=Watchface+Screenshot)

## 📋 Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Configuration](#configuration)
  - [Weather Settings](#weather-settings)
  - [Module Layout Customization](#module-layout-customization)
  - [Background Color Customization](#background-color-customization)
  - [Text Color Customization](#text-color-customization)
- [Modules](#modules)
- [Technical Details](#technical-details)
- [Troubleshooting](#troubleshooting)
- [Development](#development)
- [Credits](#credits)
- [License](#license)

## ✨ Features

- **4-Quadrant Modular Design**: Display information in a 2x2 grid layout
- **Customizable Module Placement**: Move any module to any quadrant
- **Live Weather Data**: Real-time weather information with icons
- **Multiple Display Modules**:
  - 📅 **Date**: Day of week, day number, and month
  - 🌤️ **Weather**: Temperature, conditions, and weather icon
  - ⏰ **Time**: Hours and minutes in large, readable font
  - 📊 **Stats**: Battery level with icon and step count
- **Color Customization** (on color Pebble devices):
  - Custom background colors for each quadrant
  - Manual or automatic text color selection
  - Smart text color based on background brightness
- **Multi-Platform Support**: Works on Aplite, Basalt, Diorite, and Emery platforms
- **GPS or Manual Location**: Choose automatic GPS location or enter a ZIP code/city
- **Temperature Units**: Toggle between Celsius and Fahrenheit

## 📦 Installation

### From Pebble App Store

1. Open the Pebble app on your phone
2. Go to the "Watchfaces" section
3. Search for "Modules"
4. Tap "Install" to add it to your Pebble watch

### From Source

1. Install the [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/index.html)
2. Clone this repository:
   ```bash
   git clone https://github.com/ClickCalickClick/modules-watchface.git
   cd modules-watchface/myfirstproject
   ```
3. Install dependencies:
   ```bash
   npm install
   ```
4. Build and install:
   ```bash
   pebble build
   pebble install --phone [YOUR_PHONE_IP]
   ```

## ⚙️ Configuration

Access the configuration page by opening the Pebble app on your phone, selecting the Modules watchface, and tapping the "Settings" button.

### Weather Settings

The watchface uses [WeatherAPI.com](https://www.weatherapi.com/) to fetch current weather data. Weather updates automatically every 30 minutes.

#### How Weather Data is Added

1. **Automatic Location (GPS)**:
   - Enable the "Auto Location (GPS)" toggle in settings
   - The watchface uses your phone's GPS to determine your location
   - Weather data is fetched automatically based on your coordinates
   - This is the default and recommended method

2. **Manual Location**:
   - Disable the "Auto Location (GPS)" toggle
   - Enter a ZIP code (e.g., "90210") or city name (e.g., "London") in the "ZIP Code / City" field
   - Weather data will be fetched for the specified location

3. **Weather API Integration**:
   - The JavaScript companion app (`src/pkjs/index.js`) handles all weather requests
   - Weather data includes:
     - Current temperature (in Celsius or Fahrenheit)
     - Weather condition (e.g., "Sunny", "Cloudy", "Rain")
     - Weather icon (9 different weather icons available)
   - Weather icons are mapped from WeatherAPI condition codes to appropriate visual representations
   - Data is transmitted to the watch via AppMessage protocol

#### Temperature Units

- **Use Celsius**: Toggle this option to switch between Celsius (°C) and Fahrenheit (°F)
- Default is Fahrenheit
- Changes apply immediately to the displayed temperature

#### Weather Icons

The watchface includes 9 weather icon states:
- ☀️ Clear/Sunny
- ⛅ Partly Cloudy
- ☁️ Cloudy
- 🌦️ Light Rain
- 🌧️ Heavy Rain
- 🌨️ Light Snow
- ❄️ Heavy Snow
- 🌨️ Rain and Snow (Mixed)
- 🌫️ Generic/Unknown

### Module Layout Customization

The watchface is divided into 4 quadrants in a 2x2 grid:

```
┌─────────┬─────────┐
│   Q1    │   Q2    │
│ Top     │ Top     │
│ Left    │ Right   │
├─────────┼─────────┤
│   Q3    │   Q4    │
│ Bottom  │ Bottom  │
│ Left    │ Right   │
└─────────┴─────────┘
```

#### How to Reposition Modules

1. Open the watchface settings
2. Navigate to "Quadrant Module Layout" section
3. For each quadrant, select which module to display:
   - **Empty**: No module (blank quadrant)
   - **Date**: Day name, day number, and month
   - **Weather**: Weather icon, temperature, and condition
   - **Time**: Hour and minute display
   - **Stats**: Battery level and step count

#### Module Assignment Rules

- Each module can only appear **once** across all quadrants
- If you try to assign the same module to multiple quadrants, you'll get an error
- You can have up to 4 empty quadrants if desired
- Changes take effect immediately on the watch

#### Default Layout

By default, the watchface uses this configuration:
- **Q1 (Top Left)**: Date
- **Q2 (Top Right)**: Weather
- **Q3 (Bottom Left)**: Time
- **Q4 (Bottom Right)**: Stats

### Background Color Customization

Customize the background color of each quadrant independently.

#### On Color Pebble Devices (Basalt, Chalk, Diorite, Emery)

1. Open the watchface settings
2. Navigate to "Quadrant Background Colors" section
3. For each quadrant:
   - **Enable Background**: Toggle to enable custom background for that quadrant
   - **Background Color**: Choose a custom color using the color picker
     - When enabled: Displays your selected custom color
     - When disabled: Displays white background
   - Supports the full Pebble color palette (64 colors)

#### On Black & White Pebble Devices (Aplite)

1. Open the watchface settings
2. Navigate to "Quadrant Background Colors" section
3. For each quadrant:
   - **Enable Background**: Toggle to enable background for that quadrant
     - When enabled: Displays light gray background
     - When disabled: Displays white background
   - Font sizes automatically adjust when backgrounds are enabled for better readability

#### Default Background Settings

- **Q1 (Top Left)**: White background (disabled)
- **Q2 (Top Right)**: Gray background (enabled)
- **Q3 (Bottom Left)**: Gray background (enabled)
- **Q4 (Bottom Right)**: White background (disabled)

### Text Color Customization

Control the text color for each quadrant independently (Color Pebble devices only).

#### Automatic Text Color (Recommended)

1. Open the watchface settings
2. For each quadrant, enable "Auto Text Color"
3. The watchface automatically chooses text color based on background brightness:
   - **Dark backgrounds**: White text
   - **Light backgrounds**: Black text
4. Uses a perceptual brightness calculation to ensure optimal readability

#### Manual Text Color

1. Open the watchface settings
2. For each quadrant, disable "Auto Text Color"
3. Select "Custom Text Color" to choose your preferred text color
4. Useful when you want specific color combinations regardless of readability

#### How Automatic Text Color Works

The watchface uses a brightness calculation algorithm:
```
brightness = (red × 299 + green × 587 + blue × 114) / 1000
```
- If brightness < 128: Use white text (dark background)
- If brightness ≥ 128: Use black text (light background)

This ensures text is always readable regardless of the background color you choose.

#### Default Text Color Settings

- All quadrants default to "Auto Text Color" enabled
- Custom text colors default to black when auto mode is disabled

## 📊 Modules

### Date Module

Displays the current date in a clean, vertical layout:
- **Day Name**: Three-letter abbreviation (e.g., MON, TUE)
- **Day Number**: Large, prominent display (1-31)
- **Month Name**: Three-letter abbreviation (e.g., JAN, FEB)

All text is displayed in uppercase for consistency.

### Weather Module

Shows current weather conditions:
- **Weather Icon**: Visual representation of current conditions (28x28 pixels)
- **Temperature**: Large, readable temperature display with unit (°C or °F)
- **Condition**: Single-word description (e.g., "Sunny", "Rain", "Snow")

Weather updates automatically:
- Every 30 minutes
- When settings are changed
- When the watchface starts

### Time Module

Large, clear time display:
- **Hour**: Displays in 12-hour or 24-hour format (based on watch settings)
- **Minute**: Always displays with leading zero (e.g., 05, 30)
- Minutes are displayed in a slightly lighter color for visual hierarchy

### Stats Module

Displays fitness and device information:
- **Battery Level**: 
  - Percentage display (e.g., "85%")
  - Battery icon showing charge level:
    - Full (≥75%)
    - Medium (40-74%)
    - Low (15-39%)
    - Empty (<15%)
    - Charging icon when plugged in
- **Step Count**:
  - Today's step count
  - "STEPS" label
  - Shows "--" if health data is unavailable
  - Updates automatically when you move

## 🔧 Technical Details

### Architecture

The watchface consists of three main components:

1. **C Application** (`src/c/modules.c`):
   - Main watchface logic and rendering
   - UI layer management and positioning
   - Module layout system
   - Text and background color management
   - Battery and step tracking
   - Persistent storage of settings

2. **JavaScript Companion** (`src/pkjs/index.js`):
   - Weather API integration
   - GPS location services
   - Settings validation and transmission
   - AppMessage protocol handling

3. **Configuration UI** (`src/pkjs/config.js`):
   - Clay-based settings interface
   - Platform-specific options
   - Input validation
   - Color pickers (on supported devices)

### Data Persistence

Settings are stored persistently on the watch:
- Module assignments (4 values)
- Background enable states (4 values)
- Background colors (4 values, color platforms only)
- Auto text color flags (4 values, color platforms only)
- Custom text colors (4 values, color platforms only)

### Communication Protocol

The watch and phone communicate via Pebble's AppMessage API:

**Watch → Phone** (Requests):
- Weather update request (every 30 minutes)

**Phone → Watch** (Data):
- Temperature (integer)
- Weather condition (string, max 16 chars)
- Weather icon ID (0-8)
- All configuration settings

### Performance

- **Memory Usage**: Optimized for all Pebble platforms
- **Battery Impact**: Minimal (~2-3% per day with 30-minute weather updates)
- **Update Frequency**: Time updates every minute, weather every 30 minutes

### Weather API Details

The watchface uses WeatherAPI.com with the following characteristics:
- **Endpoint**: `http://api.weatherapi.com/v1/current.json`
- **API Key**: Embedded in companion app
- **Rate Limits**: 48 requests per day (30-minute update interval)
- **Location Methods**:
  - GPS coordinates (latitude, longitude)
  - ZIP code (US and international)
  - City name (international)

Weather condition codes are intelligently mapped to appropriate icons:
- 100+ condition codes supported
- Fallback to text-based matching for unknown codes
- Time-aware (e.g., "Clear" vs "Sunny" based on time of day)

## 🐛 Troubleshooting

### Weather Not Updating

**Problem**: Weather shows "--" or doesn't update

**Solutions**:
1. Check that Bluetooth is connected between phone and watch
2. Verify that the Pebble app has location permissions (for GPS mode)
3. If using manual location, ensure ZIP code or city name is correct
4. Check internet connectivity on your phone
5. Wait up to 30 minutes for the next automatic update
6. Try toggling between GPS and manual location modes

### Duplicate Module Error

**Problem**: Error message when saving settings

**Solution**: Each module can only appear once. Check that you haven't assigned the same module (Date, Weather, Time, or Stats) to multiple quadrants.

### Text Not Readable

**Problem**: Text is hard to read against the background

**Solutions**:
1. Enable "Auto Text Color" for that quadrant (color devices only)
2. Choose a different background color with better contrast
3. Manually select a contrasting text color

### Settings Not Saving

**Problem**: Settings revert after closing the configuration page

**Solutions**:
1. Make sure to tap "Save Settings" button
2. Wait for the success confirmation
3. Check that the Pebble app is still connected to your watch
4. Try closing and reopening the Pebble app

### Steps Showing "--"

**Problem**: Step count displays "--" instead of a number

**Solutions**:
1. Ensure health tracking is enabled on your Pebble
2. Give the watch some time to collect step data (walk around)
3. Some older Pebble models don't support health tracking

## 💻 Development

### Building from Source

```bash
cd myfirstproject
npm install
pebble build
```

### Project Structure

```
myfirstproject/
├── src/
│   ├── c/
│   │   ├── modules.c          # Main watchface code
│   │   └── config.h           # Configuration header
│   └── pkjs/
│       ├── index.js           # JavaScript companion app
│       └── config.js          # Settings UI configuration
├── resources/
│   └── images/                # Weather and battery icons
├── package.json               # Project metadata and dependencies
└── wscript                    # Build configuration
```

### Dependencies

- **pebble-clay**: ^1.0.4 - Configuration framework

### Testing

Test on different Pebble platforms:
```bash
pebble build
pebble install --emulator aplite    # Black & white
pebble install --emulator basalt    # Color
pebble install --emulator chalk     # Color round
pebble install --emulator diorite   # B&W new
pebble install --emulator emery     # Color large
```

### Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on multiple Pebble platforms
5. Submit a pull request

## 🙏 Credits

- **Developer**: ClickCalickClick
- **Weather Data**: [WeatherAPI.com](https://www.weatherapi.com/)
- **Framework**: [Pebble SDK](https://developer.rebble.io/)
- **Configuration UI**: [Pebble Clay](https://github.com/pebble/clay)
- **Weather Icons**: Custom designed for this watchface
- **Battery Icons**: Custom designed for this watchface

### Special Thanks

- The Rebble Alliance for keeping Pebble alive
- The Pebble development community
- WeatherAPI.com for providing free weather data API

## 📄 License

This project is open source and available for personal use and modification.

---

## 🚀 Quick Start Guide

1. **Install** the watchface from the Pebble App Store
2. **Configure** your location (GPS or manual)
3. **Choose** your temperature unit (°C or °F)
4. **Arrange** modules in your preferred layout
5. **Customize** background colors (color Pebbles only)
6. **Adjust** text colors for optimal readability
7. **Enjoy** your personalized watchface!

---

**Version**: 1.0.0  
**Platform**: Pebble (Aplite, Basalt, Chalk, Diorite, Emery)  
**SDK Version**: 3

For questions, issues, or feature requests, please visit the [GitHub repository](https://github.com/ClickCalickClick/modules-watchface).
