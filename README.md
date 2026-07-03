<div align="center">

# ⌚ Modules Watchface

### A Highly Customizable, Modular Watchface for Pebble Smartwatches

[![Pebble](https://img.shields.io/badge/Pebble-Compatible-orange?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMjQiIGhlaWdodD0iMjQiIHZpZXdCb3g9IjAgMCAyNCAyNCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48Y2lyY2xlIGN4PSIxMiIgY3k9IjEyIiByPSIxMCIgZmlsbD0id2hpdGUiLz48L3N2Zz4=)](https://rebble.io/)
[![SDK Version](https://img.shields.io/badge/SDK-3.0-blue?style=for-the-badge)](https://developer.rebble.io/developer.pebble.com/index.html)
[![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](LICENSE)

*Transform your Pebble into a personalized information hub with drag-and-drop modules*

*Note: This watchface supports color Pebbles.  I just didn't take color screenshots.*

---

![Modules Watchface Banner](myfirstproject/modules_design_resources/design_overview_color_bw2.jpg)

</div>

## 🎯 Purpose

**Modules** is a feature-rich watchface designed for Pebble smartwatches that puts *you* in control. Unlike traditional watchfaces that lock you into a fixed layout, Modules breaks down your display into a grid of customizable cells (four on most watches, nine on the Pebble Time 2), each capable of displaying a different information module. Whether you prioritize weather data, fitness tracking, or simply want a unique aesthetic, Modules adapts to your lifestyle.

Built with love for the Pebble community and fully compatible with [Rebble](https://rebble.io/) services, this watchface breathes new life into your beloved smartwatch.

---

## ✨ Features

### 📐 Modular Grid Layout

Your Pebble's display is divided into intelligent cells — a 2x2 grid on 144x168
watches, and a 3x3 grid (nine cells) on the Pebble Time 2 (emery):

```
┌─────────────┬─────────────┐    ┌────────┬────────┬────────┐
│             │             │    │   C1   │   C2   │   C3   │
│   C1 (TL)   │   C2 (TR)   │    ├────────┼────────┼────────┤
│             │             │    │   C4   │   C5   │   C6   │
├─────────────┼─────────────┤    ├────────┼────────┼────────┤
│             │             │    │   C7   │   C8   │   C9   │
│   C3 (BL)   │   C4 (BR)   │    └────────┴────────┴────────┘
│             │             │         Pebble Time 2 (3x3)
└─────────────┴─────────────┘
     Most watches (2x2)
```

Each cell can display any of the following modules (the same module can appear
in multiple cells if you like):

| Module | Description | Information Displayed |
|--------|-------------|----------------------|
| 📅 **Date** | Calendar information | Day name, date number, month |
| 🌤️ **Weather** | Current conditions | Weather icon, temperature, condition text |
| 🕐 **Time** | Digital clock | Hour and minute in large, readable format |
| 📊 **Stats** | Device statistics | Battery level with icon, step count |
| 🌍 **Second Time Zone** | World clock | Time in another zone with a custom label |
| 🗓️ **Week Number** | ISO week | Current week of the year |
| ⏳ **Countdown** | Days until a date | Days remaining to your target date |
| 🏃 **Distance** | Health data | Distance walked today (km or miles) |
| 📆 **Calendar** | Next event | Upcoming event title and time *(data source coming in Phase 3)* |
| ⬜ **Empty** | Blank cell | Clean, minimalist option |

### 🎨 Standout Features

#### 🔄 **Flexible Module Arrangement**
Rearrange your watchface layout directly from your phone. Want the time in the top-left? Weather front and center? Stats at a glance? Simply select which module appears in each cell through the intuitive settings page.

#### 🌈 **Full Color Support** (Pebble Time, Time Steel, Time Round, Pebble 2 HR)
- Custom background colors for each quadrant
- Choose from the full Pebble 64-color palette
- Automatic text color adjustment for optimal readability
- Manual text color override for complete creative control

#### ⚫ **Monochrome Optimization** (Pebble Classic, Pebble 2)
- Crisp black and white design
- Optimized font sizes for dithered backgrounds
- Light gray background option for visual contrast

#### 💾 **Persistent Settings**
Your preferences are saved directly on the watch. Configure once, and your perfect layout survives restarts, battery changes, and updates.

#### ⚡ **Battery Efficient**
- Updates only when needed (minute-based for time, event-based for stats)
- Weather refreshes every 30 minutes to conserve battery and API calls
- Efficient memory management with proper resource cleanup

---

## 🌦️ Weather API Integration

Modules uses the **WeatherAPI.com** service to deliver accurate, real-time weather data to your wrist.

### How It Works

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   Pebble     │────▶│  Companion   │────▶│  WeatherAPI  │
│   Watch      │     │  Phone App   │     │    Server    │
└──────────────┘     └──────────────┘     └──────────────┘
       ▲                    │                     │
       │                    │◀────────────────────┘
       │                    │   JSON Response
       └────────────────────┘
         AppMessage Protocol
```

1. **Location Detection**: The companion app on your phone determines your location via:
   - **GPS** (default): Automatic location using your phone's GPS
   - **Manual Entry**: ZIP code or city name for privacy-conscious users

2. **API Request**: Weather data is fetched from WeatherAPI.com including:
   - Current temperature (°F or °C)
   - Weather condition text
   - Condition code for icon mapping

3. **Smart Icon Mapping**: The watchface intelligently maps weather condition codes to appropriate icons:
   - ☀️ Sunny/Clear
   - ⛅ Partly Cloudy
   - ☁️ Cloudy/Overcast
   - 🌧️ Light Rain
   - 🌧️ Heavy Rain
   - 🌨️ Light Snow
   - ❄️ Heavy Snow
   - 🌨️ Mixed Rain/Snow
   - 🌫️ Fog/Mist (mapped to cloudy)

4. **Automatic Updates**: Weather refreshes:
   - On watchface load
   - Every 30 minutes while active
   - After settings changes

### Weather Settings

| Setting | Description |
|---------|-------------|
| **Temperature Unit** | Toggle between Fahrenheit (°F) and Celsius (°C) |
| **Auto Location (GPS)** | Use phone's GPS for automatic location |
| **ZIP Code / City** | Manual location entry (used when GPS is disabled) |

---

## 🎛️ Customization Guide

### Accessing Settings

1. Open the **Pebble** (or **Rebble**) app on your phone
2. Navigate to **My Pebble** → **Watchfaces**
3. Select **Modules** watchface
4. Tap the **Settings** gear icon

### Module Configuration

#### Changing Module Positions

Each cell has a dropdown selector:

```
┌─ Cell 1 ──────────────────────────────────┐
│  ○ Empty                                  │
│  ● Date        ← Currently selected       │
│  ○ Weather                                │
│  ○ Time                                   │
│  ○ Stats (Battery + Steps)                │
│  ○ Second Time Zone                       │
│  ○ Week Number                            │
│  ○ Countdown                              │
│  ○ Distance                               │
│  ○ Calendar (next event)                  │
└───────────────────────────────────────────┘
```

Cells 1-4 apply to every watch; cells 5-9 are used on the Pebble Time 2's 3x3
grid. The same module may be assigned to more than one cell.

### Background Customization

#### For Color Pebbles (Basalt, Emery)

Each cell offers:
- **Enable Background**: Toggle to enable/disable custom coloring
- **Background Color**: Full 64-color palette picker
- **Auto Text Color**: Automatically selects black or white text based on background brightness
- **Custom Text Color**: Manual color selection (when Auto is disabled)

#### For Monochrome Pebbles (Aplite, Diorite, Flint)

- **Enable Background**: Toggle between white and light gray (dithered pattern)
- Font sizes automatically adjust for better readability on dithered backgrounds

### Example Configurations

#### ⏰ Time-Focused Layout
```
┌────────┬────────┐
│  Time  │ Weather│
├────────┼────────┤
│  Date  │ Stats  │
└────────┴────────┘
```

#### 🏃 Fitness-First Layout
```
┌────────┬────────┐
│ Stats  │  Date  │
├────────┼────────┤
│  Time  │ Weather│
└────────┴────────┘
```

#### 🎨 Minimalist Layout
```
┌────────┬────────┐
│ Empty  │  Time  │
├────────┼────────┤
│  Date  │ Empty  │
└────────┴────────┘
```

---

## 📱 Platform Compatibility

| Platform | Display | Colors | Health API | Grid | Status |
|----------|---------|--------|------------|------|--------|
| **Aplite** (Pebble Classic, Steel) | 144×168 | B&W | ❌ | 2x2 | ✅ Supported |
| **Basalt** (Pebble Time, Time Steel) | 144×168 | 64 colors | ✅ | 2x2 | ✅ Supported |
| **Diorite** (Pebble 2) | 144×168 | B&W | ✅ | 2x2 | ✅ Supported |
| **Flint** (Core 2 Duo) | 144×168 | B&W | ✅ | 2x2 | ✅ Supported |
| **Emery** (Pebble Time 2) | 200×228 | 64 colors | ✅ | 3x3 | ✅ Supported |
| **Chalk** (Pebble Time Round) | 180×180 round | 64 colors | ✅ | — | 🔜 Phase 3 |
| **Gabbro** (round, touch) | 260×260 round | 64 colors | ✅ | — | 🔜 Phase 3 |

---

## 📥 Installation

### From Rebble App Store

<!-- REBBLE_APPSTORE_LINK: Replace the URL below with your actual Rebble app store link -->
> 🔗 **Rebble App Store**: *Coming Soon*
> 
> Once approved, the watchface will be available at: `https://apps.rebble.io/en_US/application/[APP_ID]`

### From Pebble App Store (Legacy)

<!-- PEBBLE_APPSTORE_LINK: The original Pebble App Store is discontinued -->
> 🔗 **Pebble App Store**: *No longer available*
> 
> The original Pebble App Store was discontinued in 2018. Please use [Rebble](https://rebble.io/) services instead.

### Manual Installation (Developers)

```bash
# Clone the repository
git clone <repository-url>
cd modules-watchface/myfirstproject

# Install dependencies
npm install

# Build for all platforms
pebble build

# Install on connected watch (via Pebble/Rebble app)
pebble install --phone YOUR_PHONE_IP
```

---

## 🛠️ Technical Details

### Architecture

```
myfirstproject/
├── src/
│   ├── c/
│   │   ├── modules.c      # Main watchface logic
│   │   └── config.h       # Message key definitions
│   └── pkjs/
│       ├── index.js       # Phone companion app logic
│       └── config.js      # Clay settings configuration
├── resources/
│   └── images/            # Weather & battery icons
└── package.json           # Project configuration
```

### Key Technologies

- **Pebble SDK 3.0**: Core watchface development
- **Pebble Clay**: Configuration framework for settings UI
- **WeatherAPI.com**: Real-time weather data provider
- **AppMessage**: Pebble ↔ Phone communication protocol

### Resource Usage

| Resource | Usage |
|----------|-------|
| **Memory** | ~2KB persistent storage for settings |
| **Battery** | Optimized minute-based updates |
| **Network** | Weather updates every 30 minutes |

---

## 🤝 Contributing

Contributions are welcome! Whether it's bug fixes, new modules, or improved documentation:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-module`)
3. Commit your changes (`git commit -m 'Add amazing new module'`)
4. Push to the branch (`git push origin feature/amazing-module`)
5. Open a Pull Request

### Ideas for New Modules

- 🔗 Bluetooth connection status
- 🌅 Sunrise/Sunset times
- 📍 Compass heading
- 🎵 Music control
- ⏱️ Stopwatch/Timer

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

---

## 🙏 Acknowledgments

- **[Rebble Alliance](https://rebble.io/)** - For keeping the Pebble dream alive
- **[WeatherAPI.com](https://www.weatherapi.com/)** - Weather data provider
- **[Pebble Clay](https://github.com/pebble/clay)** - Configuration framework
- The entire **Pebble Community** - For years of innovation and support

---

<div align="center">

### Made with ❤️ for the Pebble Community

*Keep those Pebbles ticking!*

[![GitHub Stars](https://img.shields.io/github/stars/ClickCalickClick/modules-watchface?style=social)](https://github.com/ClickCalickClick/modules-watchface)

</div>
