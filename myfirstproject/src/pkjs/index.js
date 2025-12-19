// Clay configuration - handles settings UI automatically
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var WEATHER_API_KEY = '680df99f9c264bde83a142148250811';
var WEATHER_API_URL = 'http://api.weatherapi.com/v1/current.json';

// Import message keys
var MessageKeys = require('message_keys');
console.log('MessageKeys loaded: ' + JSON.stringify(MessageKeys));

// Global settings state - initialize from localStorage on app load
var globalSettings = {};
try {
  var stored = localStorage.getItem('clay-settings');
  console.log('RAW localStorage clay-settings: ' + stored);
  if (stored) {
    globalSettings = JSON.parse(stored);
    console.log('Loaded settings from localStorage: ' + JSON.stringify(globalSettings));
  } else {
    console.log('WARNING: clay-settings is null/empty in localStorage');
  }
} catch (err) {
  console.log('Failed to load settings from localStorage: ' + err);
}

// Helpers to read settings regardless of Clay storage shape
function getBool(settings, key, defaultValue) {
  var v = settings && settings[key];
  if (v && typeof v === 'object' && v !== null && 'value' in v) {
    v = v.value;
  }
  if (v === undefined || v === null) return defaultValue;
  if (typeof v === 'string') {
    var lower = v.toLowerCase();
    if (lower === 'true') return true;
    if (lower === 'false') return false;
  }
  return !!v;
}

function getString(settings, key, defaultValue) {
  var v = settings && settings[key];
  if (v && typeof v === 'object' && v !== null && 'value' in v) {
    v = v.value;
  }
  if (v === undefined || v === null) return defaultValue;
  return String(v);
}

// Map WeatherAPI condition codes to watch icon IDs (0-7)
function getWeatherIconFromCode(code, conditionText) {
  // Sunny/Clear (icon 0)
  if (code === 1000) return 0;
  
  // Partly cloudy (icon 1)
  if (code === 1003) return 1;
  
  // Cloudy/Overcast (icon 2)
  if (code === 1006 || code === 1009) return 2;
  
  // Light rain/drizzle (icon 3)
  var lightRainCodes = [1063, 1150, 1153, 1168, 1171, 1180, 1183, 1186, 1189, 1240];
  if (lightRainCodes.indexOf(code) !== -1) return 3;
  
  // Heavy rain (icon 4)
  var heavyRainCodes = [1192, 1195, 1198, 1201, 1243, 1246, 1273, 1276];
  if (heavyRainCodes.indexOf(code) !== -1) return 4;
  
  // Light snow (icon 5)
  var lightSnowCodes = [1066, 1210, 1213, 1216, 1255, 1261, 1279];
  if (lightSnowCodes.indexOf(code) !== -1) return 5;
  
  // Heavy snow/blizzard (icon 6)
  var heavySnowCodes = [1114, 1117, 1219, 1222, 1225, 1258, 1282];
  if (heavySnowCodes.indexOf(code) !== -1) return 6;
  
  // Mixed rain and snow (icon 7)
  var mixedCodes = [1069, 1072, 1204, 1207, 1237, 1249, 1252, 1264];
  if (mixedCodes.indexOf(code) !== -1) return 7;
  
  // Mist/fog - map to cloudy
  if (code === 1030 || code === 1135 || code === 1147) return 2;
  
  // Fallback: try text-based matching if code is unknown
  if (conditionText) {
    var lower = conditionText.toLowerCase();
    if (lower.indexOf('snow') !== -1 || lower.indexOf('blizzard') !== -1) return 6;
    if (lower.indexOf('sleet') !== -1 || lower.indexOf('ice') !== -1) return 7;
    if (lower.indexOf('rain') !== -1 || lower.indexOf('drizzle') !== -1 || lower.indexOf('shower') !== -1) return 4;
    if (lower.indexOf('partly') !== -1) return 1;
    if (lower.indexOf('cloud') !== -1 || lower.indexOf('overcast') !== -1) return 2;
    if (lower.indexOf('clear') !== -1 || lower.indexOf('sun') !== -1) return 0;
  }
  
  // Default to generic/cloudy
  return 2;
}

function fetchWeather(location) {
  // Read current settings from localStorage to ensure we have latest values
  var stored = localStorage.getItem("clay-settings");
  var currentSettings = stored ? JSON.parse(stored) : {};
  var useCelsius = getBool(currentSettings, "TemperatureUnit", false);
  console.log('Fetching weather for: ' + location + ', useCelsius: ' + useCelsius);
  
  var url = WEATHER_API_URL + '?key=' + WEATHER_API_KEY + '&q=' + encodeURIComponent(location);
  
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  
  xhr.onload = function() {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var response = JSON.parse(xhr.responseText);
        console.log('Weather response: ' + JSON.stringify(response));
        
        var temperature = useCelsius ? response.current.temp_c : response.current.temp_f;
        var condition = response.current.condition.text;
        var conditionCode = response.current.condition.code;
        var iconCode = getWeatherIconFromCode(conditionCode, condition);
        
        // Always send raw Fahrenheit - let the watch convert based on its setting
        var tempFahrenheit = response.current.temp_f;
        var tempCelsius = response.current.temp_c;
        
        console.log('Temperature data - F: ' + tempFahrenheit + ', C: ' + tempCelsius);
        console.log('Sending weather: F=' + tempFahrenheit + '°F, C=' + tempCelsius + '°C, ' + condition + ' (code: ' + conditionCode + '), icon: ' + iconCode);
        
        // Send weather data to watch
        var dictionary = {};
        dictionary[MessageKeys.Temperature] = Math.round(tempFahrenheit);
        dictionary[MessageKeys.Condition] = condition;
        dictionary[MessageKeys.WeatherIcon] = iconCode;
        // Also send the temperature unit setting so watch can convert properly
        var tempUnitValue = getBool(currentSettings, 'TemperatureUnit', false) ? 1 : 0;
        dictionary[MessageKeys.TemperatureUnit] = tempUnitValue;
        console.log('Current settings: ' + JSON.stringify(currentSettings));
        console.log('getBool result: ' + getBool(globalSettings, 'TemperatureUnit', false));
        console.log('TemperatureUnit value to send: ' + tempUnitValue);
        console.log('Full dictionary: ' + JSON.stringify(dictionary));
        
        console.log('Dictionary keys: ' + Object.keys(dictionary).join(', '));
        console.log('Dictionary values: ' + JSON.stringify(dictionary));
        
        Pebble.sendAppMessage(dictionary,
          function() {
            console.log('Weather data sent successfully');
          },
          function(e) {
            console.log('Failed to send weather data: ' + JSON.stringify(e));
          }
        );
      } catch (e) {
        console.log('Error parsing weather response: ' + e);
      }
    } else {
      console.log('Weather request failed with status: ' + xhr.status);
    }
  };
  
  xhr.onerror = function() {
    console.log('Weather request error');
  };
  
  xhr.send();
}

function locationSuccess(pos) {
  var coords = pos.coords.latitude + ',' + pos.coords.longitude;
  console.log('Got location: ' + coords);
  
  fetchWeather(coords);
}

function locationError(err) {
  console.log('Location error: ' + err.message);
  
  // Fall back to ZIP code if available
  var stored = localStorage.getItem("clay-settings");
  var currentSettings = stored ? JSON.parse(stored) : {};
  var zipCode = getString(currentSettings, 'ZipCode', '');
  
  if (zipCode && zipCode.length > 0) {
    fetchWeather(zipCode);
  } else {
    console.log('No fallback location available');
  }
}

function getWeather() {
  var stored = localStorage.getItem("clay-settings");
  console.log("RAW localStorage value: " + stored);
  var currentSettings = stored ? JSON.parse(stored) : {};
  console.log("Parsed currentSettings: " + JSON.stringify(currentSettings).substring(0, 200));
  var useCelsius = getBool(currentSettings, "TemperatureUnit", false);
  console.log("useCelsius read from localStorage: " + useCelsius);
  var useGPS = getBool(currentSettings, 'UseGPS', true); // Default to true
  var zipCode = getString(currentSettings, 'ZipCode', '');
  
  console.log('Getting weather - GPS: ' + useGPS + ', ZIP: ' + zipCode + ', Celsius: ' + useCelsius);
  
  if (useGPS) {
    // Use GPS location
    navigator.geolocation.getCurrentPosition(
      locationSuccess,
      locationError,
      {
        timeout: 15000,
        maximumAge: 60000
      }
    );
  } else if (zipCode.length > 0) {
    // Use manual ZIP code/city
    fetchWeather(zipCode);
  } else {
    console.log('No location method configured');
  }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  
  // Send current settings to watch immediately so it uses correct temperature unit
  var settingsDict = {};
  settingsDict[MessageKeys.TemperatureUnit] = getBool(globalSettings, 'TemperatureUnit', false) ? 1 : 0;
  console.log('Sending initial TemperatureUnit setting: ' + settingsDict[MessageKeys.TemperatureUnit]);
  Pebble.sendAppMessage(settingsDict, function() {
    console.log('Initial settings sent');
  }, function(e) {
    console.log('Failed to send initial settings: ' + JSON.stringify(e));
  });
  
  // Get initial weather
  getWeather();
});

// Listen for messages from the watch
Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received: ' + JSON.stringify(e.payload));
  
  // Watch is requesting weather update
  if (e.payload.Temperature !== undefined) {
    getWeather();
  }
});

// Listen for when settings are saved
Pebble.addEventListener('webviewclosed', function(e) {
  console.log('=== webviewclosed event fired ===');
  console.log('Event response: ' + (e ? JSON.stringify(e.response).substring(0, 200) : 'no response'));
  if (e && !e.response) {
    console.log('No response, returning');
    return;
  }
  
  // Settings were saved, update global state and persist
  var settings = JSON.parse(e.response);
  // Flatten settings before storing in globalSettings
  var flatSettings = {};
  for (var key in settings) {
    if (settings[key] && typeof settings[key] === 'object' && 'value' in settings[key]) {
      flatSettings[key] = settings[key].value;
    } else {
      flatSettings[key] = settings[key];
    }
  }
  globalSettings = flatSettings;  // Update global state with flattened format
  console.log('Settings received and updated globally (flattened): ' + JSON.stringify(globalSettings).substring(0, 200));
  console.log('globalSettings.TemperatureUnit: ' + globalSettings.TemperatureUnit);
  
  // Persist the latest settings so periodic updates use the correct values
  // Flatten settings before storing (Clay stores with .value, but we need flattened format)
  var flatSettings = {};
  for (var key in settings) {
    if (settings[key] && typeof settings[key] === 'object' && 'value' in settings[key]) {
      flatSettings[key] = settings[key].value;
    } else {
      flatSettings[key] = settings[key];
    }
  }
  try {
    localStorage.setItem('clay-settings', JSON.stringify(flatSettings));
    console.log('Settings persisted to localStorage (flattened): ' + JSON.stringify(flatSettings).substring(0, 100));
  } catch (err) {
    console.log('Failed to persist settings: ' + err);
  }
  
  // Validate module assignments - each module can only appear once
  var modules = [
    parseInt((settings.Quadrant1Module && settings.Quadrant1Module.value) || '1'),
    parseInt((settings.Quadrant2Module && settings.Quadrant2Module.value) || '2'),
    parseInt((settings.Quadrant3Module && settings.Quadrant3Module.value) || '3'),
    parseInt((settings.Quadrant4Module && settings.Quadrant4Module.value) || '4')
  ];
  
  // Check for duplicate non-empty modules
  var seen = {};
  var hasDuplicate = false;
  var duplicateModule = '';
  
  for (var i = 0; i < modules.length; i++) {
    var module = modules[i];
    if (module !== 0) { // Skip empty modules
      if (seen[module]) {
        hasDuplicate = true;
        var moduleNames = ['', 'Date', 'Weather', 'Time', 'Stats'];
        duplicateModule = moduleNames[module] || 'Unknown';
        break;
      }
      seen[module] = true;
    }
  }
  
  if (hasDuplicate) {
    console.log('ERROR: Duplicate module detected: ' + duplicateModule);
    alert('Error: "' + duplicateModule + '" module is assigned to multiple quadrants. Each module can only appear once.');
    return;
  }
  
  // Send settings to watch
  var dictionary = {};
  dictionary[MessageKeys.TemperatureUnit] = (settings.TemperatureUnit && settings.TemperatureUnit.value) ? 1 : 0;
  dictionary[MessageKeys.UseGPS] = (settings.UseGPS && settings.UseGPS.value !== false) ? 1 : 0;
  dictionary[MessageKeys.ZipCode] = (settings.ZipCode && settings.ZipCode.value) || '';
  dictionary[MessageKeys.Quadrant1Module] = modules[0];
  dictionary[MessageKeys.Quadrant2Module] = modules[1];
  dictionary[MessageKeys.Quadrant3Module] = modules[2];
  dictionary[MessageKeys.Quadrant4Module] = modules[3];
  dictionary[MessageKeys.Quadrant1Background] = (settings.Quadrant1Background && settings.Quadrant1Background.value) ? 1 : 0;
  dictionary[MessageKeys.Quadrant2Background] = (settings.Quadrant2Background && settings.Quadrant2Background.value) ? 1 : 0;
  dictionary[MessageKeys.Quadrant3Background] = (settings.Quadrant3Background && settings.Quadrant3Background.value) ? 1 : 0;
  dictionary[MessageKeys.Quadrant4Background] = (settings.Quadrant4Background && settings.Quadrant4Background.value) ? 1 : 0;
  
  // Send color values if available (for color platforms)
  if (settings.Quadrant1Color && settings.Quadrant1Color.value) {
    dictionary[MessageKeys.Quadrant1Color] = parseInt(settings.Quadrant1Color.value);
  }
  if (settings.Quadrant2Color && settings.Quadrant2Color.value) {
    dictionary[MessageKeys.Quadrant2Color] = parseInt(settings.Quadrant2Color.value);
  }
  if (settings.Quadrant3Color && settings.Quadrant3Color.value) {
    dictionary[MessageKeys.Quadrant3Color] = parseInt(settings.Quadrant3Color.value);
  }
  if (settings.Quadrant4Color && settings.Quadrant4Color.value) {
    dictionary[MessageKeys.Quadrant4Color] = parseInt(settings.Quadrant4Color.value);
  }
  
  // Send auto text color toggles (default to true if not set)
  dictionary[MessageKeys.Quadrant1AutoTextColor] = (settings.Quadrant1AutoTextColor && settings.Quadrant1AutoTextColor.value !== false) ? 1 : 0;
  dictionary[MessageKeys.Quadrant2AutoTextColor] = (settings.Quadrant2AutoTextColor && settings.Quadrant2AutoTextColor.value !== false) ? 1 : 0;
  dictionary[MessageKeys.Quadrant3AutoTextColor] = (settings.Quadrant3AutoTextColor && settings.Quadrant3AutoTextColor.value !== false) ? 1 : 0;
  dictionary[MessageKeys.Quadrant4AutoTextColor] = (settings.Quadrant4AutoTextColor && settings.Quadrant4AutoTextColor.value !== false) ? 1 : 0;
  
  // Send custom text colors if available (for color platforms)
  if (settings.Quadrant1TextColor && settings.Quadrant1TextColor.value) {
    dictionary[MessageKeys.Quadrant1TextColor] = parseInt(settings.Quadrant1TextColor.value);
  }
  if (settings.Quadrant2TextColor && settings.Quadrant2TextColor.value) {
    dictionary[MessageKeys.Quadrant2TextColor] = parseInt(settings.Quadrant2TextColor.value);
  }
  if (settings.Quadrant3TextColor && settings.Quadrant3TextColor.value) {
    dictionary[MessageKeys.Quadrant3TextColor] = parseInt(settings.Quadrant3TextColor.value);
  }
  if (settings.Quadrant4TextColor && settings.Quadrant4TextColor.value) {
    dictionary[MessageKeys.Quadrant4TextColor] = parseInt(settings.Quadrant4TextColor.value);
  }
  
  console.log('Sending settings to watch: ' + JSON.stringify(dictionary));
  
  Pebble.sendAppMessage(dictionary,
    function() {
      console.log('Settings sent successfully');
      
      // Fetch weather with the updated settings from the event (not localStorage yet)
      var useCelsius = !!(settings.TemperatureUnit && settings.TemperatureUnit.value);
      var useGPS = (settings.UseGPS && settings.UseGPS.value !== false);
      var zipCode = (settings.ZipCode && settings.ZipCode.value) || '';
      
      console.log('Fetching weather with updated settings - Celsius: ' + useCelsius + ', GPS: ' + useGPS);
      
      if (useGPS) {
        navigator.geolocation.getCurrentPosition(
          function(pos) {
            var coords = pos.coords.latitude + ',' + pos.coords.longitude;
            fetchWeather(coords);
          },
          function(err) {
            console.log('Location error: ' + err.message);
            if (zipCode.length > 0) {
              fetchWeather(zipCode);
            }
          },
          {
            timeout: 15000,
            maximumAge: 60000
          }
        );
      } else if (zipCode.length > 0) {
        fetchWeather(zipCode);
      }
    },
    function(e) {
      console.log('Failed to send settings: ' + JSON.stringify(e));
    }
  );
});
