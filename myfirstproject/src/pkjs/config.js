// Clay settings. Built programmatically so the 9 cells (emery 3x3; other
// platforms use the first 4) and their colour controls don't have to be repeated by hand.

// Module dropdown options, grouped into categories. Clay's select renders an
// <optgroup> whenever an option's "value" is an array (see select.tpl), so each
// group is { label, value: [ {label, value}, ... ] }. Empty stays ungrouped.
var MODULE_OPTIONS = [
  { "label": "Empty", "value": "0" },
  { "label": "Core", "value": [
    { "label": "Time", "value": "3" },
    { "label": "Date", "value": "1" },
    { "label": "Weather", "value": "2" },
    { "label": "Stats (Battery + Steps)", "value": "4" }
  ] },
  { "label": "Clock & Calendar", "value": [
    { "label": "Second Time Zone", "value": "5" },
    { "label": "Analog Mini-clock", "value": "21" },
    { "label": "Week Number", "value": "6" },
    { "label": "Day of Year", "value": "17" },
    { "label": "Countdown", "value": "7" },
    { "label": "Count-up (days since)", "value": "18" },
    { "label": "Calendar (next event)", "value": "9" }
  ] },
  { "label": "Health", "value": [
    { "label": "Heart Rate", "value": "11" },
    { "label": "Step Goal Ring", "value": "12" },
    { "label": "Sleep", "value": "13" },
    { "label": "Calories", "value": "14" },
    { "label": "Active Minutes", "value": "15" },
    { "label": "Distance", "value": "8" }
  ] },
  { "label": "Weather & Sky", "value": [
    { "label": "Sunrise / Sunset", "value": "22" },
    { "label": "Weather High / Low", "value": "23" },
    { "label": "Humidity", "value": "24" },
    { "label": "Wind", "value": "25" },
    { "label": "UV Index", "value": "26" },
    { "label": "Air Quality (AQI)", "value": "27" },
    { "label": "Moon Phase", "value": "16" }
  ] },
  { "label": "Utility", "value": [
    { "label": "Bluetooth Status", "value": "10" },
    { "label": "Quiet Time", "value": "20" },
    { "label": "Custom Text", "value": "19" }
  ] },
  { "label": "Finance", "value": [
    { "label": "Crypto Ticker", "value": "28" }
  ] }
];

// Default module per cell (1-indexed cell -> module value)
var CELL_DEFAULTS = ["1", "2", "3", "4", "5", "6", "7", "8", "9"];
var BG_DEFAULTS = [false, true, true, false, true, false, false, true, false];

function moduleSelects() {
  var items = [{
    "type": "watchPreview"
  }, {
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

// IANA time zones for the second-time-zone picker. The phone (pkjs) resolves the
// selected zone to a DST-correct offset via Intl and pushes it as TZOffset, so
// the watch never has to know about DST.
function tzCityOptions() {
  return [
    { "label": "UTC", "value": "UTC" },
    { "label": "Honolulu", "value": "Pacific/Honolulu" },
    { "label": "Anchorage", "value": "America/Anchorage" },
    { "label": "Los Angeles", "value": "America/Los_Angeles" },
    { "label": "Denver", "value": "America/Denver" },
    { "label": "Chicago", "value": "America/Chicago" },
    { "label": "New York", "value": "America/New_York" },
    { "label": "Sao Paulo", "value": "America/Sao_Paulo" },
    { "label": "London", "value": "Europe/London" },
    { "label": "Paris / Berlin", "value": "Europe/Paris" },
    { "label": "Athens", "value": "Europe/Athens" },
    { "label": "Moscow", "value": "Europe/Moscow" },
    { "label": "Dubai", "value": "Asia/Dubai" },
    { "label": "India", "value": "Asia/Kolkata" },
    { "label": "Bangkok", "value": "Asia/Bangkok" },
    { "label": "Shanghai", "value": "Asia/Shanghai" },
    { "label": "Tokyo", "value": "Asia/Tokyo" },
    { "label": "Singapore", "value": "Asia/Singapore" },
    { "label": "Sydney", "value": "Australia/Sydney" },
    { "label": "Auckland", "value": "Pacific/Auckland" }
  ];
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
      { "type": "select", "messageKey": "TZCity", "label": "Second Time Zone (City)",
        "description": "Offset is computed on the phone with DST handled automatically",
        "defaultValue": "Europe/London", "options": tzCityOptions() },
      { "type": "input", "messageKey": "TZLabel", "label": "Second Time Zone Label",
        "defaultValue": "TZ2", "attributes": { "placeholder": "e.g., LON", "maxlength": 6 } },
      { "type": "input", "messageKey": "CountdownDate", "label": "Countdown Target Date",
        "defaultValue": "", "attributes": { "placeholder": "YYYY-MM-DD", "type": "date" } },
      { "type": "input", "messageKey": "CountdownLabel", "label": "Countdown Label",
        "defaultValue": "EVENT", "attributes": { "placeholder": "e.g., TRIP", "maxlength": 8 } },
      { "type": "toggle", "messageKey": "DistanceUnits", "label": "Use Kilometers",
        "description": "Distance in km instead of miles", "defaultValue": true },
      { "type": "toggle", "messageKey": "BluetoothVibe", "label": "Vibrate on Bluetooth Disconnect",
        "description": "Buzz when the phone connection drops", "defaultValue": false },
      { "type": "input", "messageKey": "CountupDate", "label": "Count-up Start Date",
        "defaultValue": "", "attributes": { "placeholder": "YYYY-MM-DD", "type": "date" } },
      { "type": "input", "messageKey": "CountupLabel", "label": "Count-up Label",
        "defaultValue": "SINCE", "attributes": { "placeholder": "e.g., SOBER", "maxlength": 8 } },
      { "type": "input", "messageKey": "CustomText", "label": "Custom Text",
        "defaultValue": "", "attributes": { "placeholder": "e.g., HELLO", "maxlength": 20 } },
      { "type": "input", "messageKey": "StepGoal", "label": "Daily Step Goal",
        "description": "Target for the Step Goal Ring module", "defaultValue": "10000",
        "attributes": { "placeholder": "10000", "type": "number" } },
      { "type": "input", "messageKey": "CalendarUrl", "label": "Calendar ICS URL",
        "description": "Public .ics feed URL for the Calendar module's next event",
        "defaultValue": "", "attributes": { "placeholder": "https://.../basic.ics", "type": "url" } },
      { "type": "input", "messageKey": "CryptoId", "label": "Crypto CoinGecko ID",
        "description": "CoinGecko coin id for the Crypto Ticker module",
        "defaultValue": "bitcoin", "attributes": { "placeholder": "e.g., bitcoin, ethereum" } },
      { "type": "input", "messageKey": "CryptoLabel", "label": "Crypto Ticker Label",
        "defaultValue": "BTC", "attributes": { "placeholder": "e.g., BTC", "maxlength": 5 } }
    ]
  },
  {
    "type": "section",
    "items": [{ "type": "heading", "defaultValue": "Cell Colors" }].concat(colorControls())
  },
  { "type": "submit", "defaultValue": "Save Settings" }
];
