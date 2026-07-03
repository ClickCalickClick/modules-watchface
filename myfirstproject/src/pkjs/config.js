// Clay settings. Built programmatically so the 9 cells (emery 3x3; other
// platforms use the first 4) and their colour controls don't have to be repeated by hand.

var MODULE_OPTIONS = [
  { "label": "Empty", "value": "0" },
  { "label": "Date", "value": "1" },
  { "label": "Weather", "value": "2" },
  { "label": "Time", "value": "3" },
  { "label": "Stats (Battery + Steps)", "value": "4" },
  { "label": "Second Time Zone", "value": "5" },
  { "label": "Week Number", "value": "6" },
  { "label": "Countdown", "value": "7" },
  { "label": "Distance", "value": "8" },
  { "label": "Calendar (next event)", "value": "9" }
];

// Default module per cell (1-indexed cell -> module value)
var CELL_DEFAULTS = ["1", "2", "3", "4", "5", "6", "7", "8", "9"];
var BG_DEFAULTS = [false, true, true, false, true, false, false, true, false];

function moduleSelects() {
  var items = [{
    "type": "text",
    "defaultValue": "Cells 1-4 are used on every watch (2x2). Cells 5-9 add the extra " +
      "cells on the Pebble Time 2 / emery 3x3 grid."
  }];
  for (var i = 1; i <= 9; i++) {
    items.push({
      "type": "select",
      "messageKey": "Quadrant" + i + "Module",
      "label": "Cell " + i,
      "defaultValue": CELL_DEFAULTS[i - 1],
      "options": MODULE_OPTIONS
    });
  }
  return items;
}

function colorControls() {
  var items = [{
    "type": "text",
    "defaultValue": "Per-cell background and text colours. Colour controls apply to colour watches."
  }];
  for (var i = 1; i <= 9; i++) {
    items.push({
      "type": "toggle",
      "messageKey": "Quadrant" + i + "Background",
      "label": "Cell " + i + " - Background",
      "description": "Enable custom background",
      "defaultValue": BG_DEFAULTS[i - 1]
    });
    items.push({
      "type": "color",
      "messageKey": "Quadrant" + i + "Color",
      "label": "Cell " + i + " Background Color",
      "defaultValue": "0xAAAAAA",
      "sunlight": false,
      "allowGray": true,
      "capabilities": ["COLOR"]
    });
    items.push({
      "type": "toggle",
      "messageKey": "Quadrant" + i + "AutoTextColor",
      "label": "Cell " + i + " - Auto Text Color",
      "description": "Automatically choose white/black text based on background",
      "defaultValue": true,
      "capabilities": ["COLOR"]
    });
    items.push({
      "type": "color",
      "messageKey": "Quadrant" + i + "TextColor",
      "label": "Cell " + i + " Custom Text Color",
      "description": "Used when Auto Text Color is OFF",
      "defaultValue": "0x000000",
      "sunlight": false,
      "allowGray": true,
      "capabilities": ["COLOR"]
    });
  }
  return items;
}

function tzOffsetOptions() {
  var opts = [];
  for (var h = -12; h <= 14; h++) {
    opts.push({ "label": "Local " + (h >= 0 ? "+" : "") + h + "h", "value": String(h * 60) });
  }
  return opts;
}

module.exports = [
  { "type": "heading", "defaultValue": "Modules Watchface Settings" },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Weather Settings" },
      { "type": "toggle", "messageKey": "TemperatureUnit", "label": "Use Celsius",
        "description": "Temperature in Celsius instead of Fahrenheit", "defaultValue": false },
      { "type": "toggle", "messageKey": "UseGPS", "label": "Auto Location (GPS)",
        "description": "Use phone's GPS for weather location", "defaultValue": true },
      { "type": "input", "messageKey": "ZipCode", "defaultValue": "", "label": "ZIP Code / City",
        "description": "Enter ZIP code or city name (used when GPS is off)",
        "attributes": { "placeholder": "e.g., 90210 or London" } }
    ]
  },
  {
    "type": "section",
    "items": [{ "type": "heading", "defaultValue": "Module Layout" }].concat(moduleSelects())
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Module Options" },
      { "type": "select", "messageKey": "TZOffset", "label": "Second Time Zone Offset",
        "defaultValue": "0", "options": tzOffsetOptions() },
      { "type": "input", "messageKey": "TZLabel", "label": "Second Time Zone Label",
        "defaultValue": "TZ2", "attributes": { "placeholder": "e.g., LON", "maxlength": 6 } },
      { "type": "input", "messageKey": "CountdownDate", "label": "Countdown Target Date",
        "defaultValue": "", "attributes": { "placeholder": "YYYY-MM-DD", "type": "date" } },
      { "type": "input", "messageKey": "CountdownLabel", "label": "Countdown Label",
        "defaultValue": "EVENT", "attributes": { "placeholder": "e.g., TRIP", "maxlength": 8 } },
      { "type": "toggle", "messageKey": "DistanceUnits", "label": "Use Kilometers",
        "description": "Distance in km instead of miles", "defaultValue": true }
    ]
  },
  {
    "type": "section",
    "items": [{ "type": "heading", "defaultValue": "Cell Colors" }].concat(colorControls())
  },
  { "type": "submit", "defaultValue": "Save Settings" }
];
