module.exports = [
  {
    "type": "heading",
    "defaultValue": "Modules Watchface Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather Settings"
      },
      {
        "type": "toggle",
        "messageKey": "TemperatureUnit",
        "label": "Use Celsius",
        "description": "Temperature in Celsius instead of Fahrenheit",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "AltWeatherLayout",
        "label": "Alternative day and night icons",
        "description": "Use larger weather icons",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "UseGPS",
        "label": "Auto Location (GPS)",
        "description": "Use phone's GPS for weather location",
        "defaultValue": true
      },
      {
        "type": "input",
        "messageKey": "ZipCode",
        "defaultValue": "",
        "label": "ZIP Code / City",
        "description": "Enter ZIP code or city name (used when GPS is off)",
        "attributes": {
          "placeholder": "e.g., 90210 or London"
        }
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Quadrant Module Layout"
      },
      {
        "type": "text",
        "defaultValue": "Drag modules to different quadrants. Each module can only appear once."
      },
      {
        "type": "select",
        "messageKey": "Quadrant1Module",
        "label": "Top Left Quadrant (Q1)",
        "defaultValue": "1",
        "options": [
          { "label": "Empty", "value": "0" },
          { "label": "Date", "value": "1" },
          { "label": "Weather", "value": "2" },
          { "label": "Time", "value": "3" },
          { "label": "Stats (Battery + Steps)", "value": "4" }
        ]
      },
      {
        "type": "select",
        "messageKey": "Quadrant2Module",
        "label": "Top Right Quadrant (Q2)",
        "defaultValue": "2",
        "options": [
          { "label": "Empty", "value": "0" },
          { "label": "Date", "value": "1" },
          { "label": "Weather", "value": "2" },
          { "label": "Time", "value": "3" },
          { "label": "Stats (Battery + Steps)", "value": "4" }
        ]
      },
      {
        "type": "select",
        "messageKey": "Quadrant3Module",
        "label": "Bottom Left Quadrant (Q3)",
        "defaultValue": "3",
        "options": [
          { "label": "Empty", "value": "0" },
          { "label": "Date", "value": "1" },
          { "label": "Weather", "value": "2" },
          { "label": "Time", "value": "3" },
          { "label": "Stats (Battery + Steps)", "value": "4" }
        ]
      },
      {
        "type": "select",
        "messageKey": "Quadrant4Module",
        "label": "Bottom Right Quadrant (Q4)",
        "defaultValue": "4",
        "options": [
          { "label": "Empty", "value": "0" },
          { "label": "Date", "value": "1" },
          { "label": "Weather", "value": "2" },
          { "label": "Time", "value": "3" },
          { "label": "Stats (Battery + Steps)", "value": "4" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Quadrant Background Colors"
      },
      {
        "type": "text",
        "defaultValue": function(config) {
          var platform = config.platform;
          if (platform && (platform === 'basalt' || platform === 'chalk' || platform === 'diorite' || platform === 'emery')) {
            return "Enable background colors for each quadrant. Choose custom colors below.";
          }
          return "Enable light gray background for each quadrant (off = white).";
        }
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant1Background",
        "label": "Q1 - Top Left Background",
        "description": "Enable custom background",
        "defaultValue": false
      },
      {
        "type": "color",
        "messageKey": "Quadrant1Color",
        "label": "Q1 Background Color",
        "defaultValue": "0xAAAAAA",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant1AutoTextColor",
        "label": "Q1 - Auto Text Color",
        "description": "Automatically choose white/black text based on background",
        "defaultValue": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "color",
        "messageKey": "Quadrant1TextColor",
        "label": "Q1 Custom Text Color",
        "description": "Used when Auto Text Color is OFF",
        "defaultValue": "0x000000",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant2Background",
        "label": "Q2 - Top Right Background",
        "description": "Enable custom background",
        "defaultValue": true
      },
      {
        "type": "color",
        "messageKey": "Quadrant2Color",
        "label": "Q2 Background Color",
        "defaultValue": "0xAAAAAA",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant2AutoTextColor",
        "label": "Q2 - Auto Text Color",
        "description": "Automatically choose white/black text based on background",
        "defaultValue": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "color",
        "messageKey": "Quadrant2TextColor",
        "label": "Q2 Custom Text Color",
        "description": "Used when Auto Text Color is OFF",
        "defaultValue": "0x000000",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant3Background",
        "label": "Q3 - Bottom Left Background",
        "description": "Enable custom background",
        "defaultValue": true
      },
      {
        "type": "color",
        "messageKey": "Quadrant3Color",
        "label": "Q3 Background Color",
        "defaultValue": "0xAAAAAA",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant3AutoTextColor",
        "label": "Q3 - Auto Text Color",
        "description": "Automatically choose white/black text based on background",
        "defaultValue": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "color",
        "messageKey": "Quadrant3TextColor",
        "label": "Q3 Custom Text Color",
        "description": "Used when Auto Text Color is OFF",
        "defaultValue": "0x000000",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant4Background",
        "label": "Q4 - Bottom Right Background",
        "description": "Enable custom background",
        "defaultValue": false
      },
      {
        "type": "color",
        "messageKey": "Quadrant4Color",
        "label": "Q4 Background Color",
        "defaultValue": "0xAAAAAA",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "toggle",
        "messageKey": "Quadrant4AutoTextColor",
        "label": "Q4 - Auto Text Color",
        "description": "Automatically choose white/black text based on background",
        "defaultValue": true,
        "capabilities": ['COLOR']
      },
      {
        "type": "color",
        "messageKey": "Quadrant4TextColor",
        "label": "Q4 Custom Text Color",
        "description": "Used when Auto Text Color is OFF",
        "defaultValue": "0x000000",
        "sunlight": false,
        "allowGray": true,
        "capabilities": ['COLOR']
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
