// Clay configuration - handles settings UI automatically
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var WEATHER_API_KEY = '680df99f9c264bde83a142148250811';
var WEATHER_API_URL = 'http://api.weatherapi.com/v1/current.json';

// Import message keys
var MessageKeys = require('message_keys');
console.log('MessageKeys loaded: ' + JSON.stringify(MessageKeys));

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

function fetchWeather(location, useCelsius) {
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
        
        console.log('useCelsius=' + useCelsius + ', temp_c=' + response.current.temp_c + ', temp_f=' + response.current.temp_f + ', selected=' + temperature);
        console.log('Sending weather: ' + temperature + '°, ' + condition + ' (code: ' + conditionCode + '), icon: ' + iconCode);
        
        // Send weather data to watch
        var dictionary = {};
        dictionary[MessageKeys.Temperature] = Math.round(temperature);
        dictionary[MessageKeys.Condition] = condition;
        dictionary[MessageKeys.WeatherIcon] = iconCode;
        
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
  
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var useCelsius = (settings.TemperatureUnit && settings.TemperatureUnit.value) || false;
  
  fetchWeather(coords, useCelsius);
}

function locationError(err) {
  console.log('Location error: ' + err.message);
  
  // Fall back to ZIP code if available
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var zipCode = (settings.ZipCode && settings.ZipCode.value) || '';
  var useCelsius = (settings.TemperatureUnit && settings.TemperatureUnit.value) || false;
  
  if (zipCode && zipCode.length > 0) {
    fetchWeather(zipCode, useCelsius);
  } else {
    console.log('No fallback location available');
  }
}

function getWeather() {
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var useGPS = (settings.UseGPS && settings.UseGPS.value !== false); // Default to true
  var zipCode = (settings.ZipCode && settings.ZipCode.value) || '';
  var useCelsius = (settings.TemperatureUnit && settings.TemperatureUnit.value) || false;
  
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
    fetchWeather(zipCode, useCelsius);
  } else {
    console.log('No location method configured');
  }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  
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
  if (e && !e.response) {
    return;
  }
  
  // Settings were saved, send them to the watch
  var settings = JSON.parse(e.response);
  console.log('Settings received: ' + JSON.stringify(settings));
  
  // Send settings to watch
  var dictionary = {};
  dictionary[MessageKeys.TemperatureUnit] = (settings.TemperatureUnit && settings.TemperatureUnit.value) ? 1 : 0;
  dictionary[MessageKeys.UseGPS] = (settings.UseGPS && settings.UseGPS.value !== false) ? 1 : 0;
  dictionary[MessageKeys.ZipCode] = (settings.ZipCode && settings.ZipCode.value) || '';
  
  console.log('Sending settings to watch: ' + JSON.stringify(dictionary));
  
  Pebble.sendAppMessage(dictionary,
    function() {
      console.log('Settings sent successfully');
      
      // Fetch weather with the updated settings from the event (not localStorage yet)
      var useCelsius = (settings.TemperatureUnit && settings.TemperatureUnit.value) || false;
      var useGPS = (settings.UseGPS && settings.UseGPS.value !== false);
      var zipCode = (settings.ZipCode && settings.ZipCode.value) || '';
      
      console.log('Fetching weather with updated settings - Celsius: ' + useCelsius + ', GPS: ' + useGPS);
      
      if (useGPS) {
        navigator.geolocation.getCurrentPosition(
          function(pos) {
            var coords = pos.coords.latitude + ',' + pos.coords.longitude;
            fetchWeather(coords, useCelsius);
          },
          function(err) {
            console.log('Location error: ' + err.message);
            if (zipCode.length > 0) {
              fetchWeather(zipCode, useCelsius);
            }
          },
          {
            timeout: 15000,
            maximumAge: 60000
          }
        );
      } else if (zipCode.length > 0) {
        fetchWeather(zipCode, useCelsius);
      }
    },
    function(e) {
      console.log('Failed to send settings: ' + JSON.stringify(e));
    }
  );
});
