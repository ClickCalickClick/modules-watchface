// Clay configuration - handles the settings UI and sends all settings to the
// watch automatically (autoHandleEvents). This file only handles weather.
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var preview = require('./preview');
// Register the live-preview component on the Clay INSTANCE so Clay serializes it
// into the config page (window.clayComponents) and the webview can build the
// `watchPreview` item. All of the component's webview logic lives inside the
// component itself (see preview.js) — no customFn referencing this module scope,
// which would throw ReferenceError in the webview and blank the settings screen.
var clay = new Clay(clayConfig);
clay.registerComponent(preview.component);

var WEATHER_API_KEY = '680df99f9c264bde83a142148250811';
// forecast.json (1 day, aqi=yes) returns current conditions + today's hi/lo +
// humidity/wind/uv + air quality + sunrise/sunset in a single request.
var WEATHER_API_URL = 'https://api.weatherapi.com/v1/forecast.json';

// Parse a WeatherAPI clock string like "04:49 AM" into minutes since midnight.
function parseClockToMinutes(s) {
  if (!s) return -1;
  var m = s.match(/(\d+):(\d+)\s*(AM|PM)?/i);
  if (!m) return -1;
  var h = parseInt(m[1], 10), min = parseInt(m[2], 10);
  var ap = m[3] ? m[3].toUpperCase() : '';
  if (ap === 'PM' && h !== 12) h += 12;
  if (ap === 'AM' && h === 12) h = 0;
  return h * 60 + min;
}

var MessageKeys = require('message_keys');

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
  var url = WEATHER_API_URL + '?key=' + WEATHER_API_KEY +
    '&q=' + encodeURIComponent(location) + '&days=1&aqi=yes';

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);

  xhr.onload = function() {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var response = JSON.parse(xhr.responseText);
        var cur = response.current;
        var forecastDay = response.forecast.forecastday[0];
        var day = forecastDay.day;
        var astro = forecastDay.astro;

        var temperature = useCelsius ? cur.temp_c : cur.temp_f;
        var condition = cur.condition.text;
        var conditionCode = cur.condition.code;
        var iconCode = getWeatherIconFromCode(conditionCode, condition);

        console.log('Weather: ' + temperature + ', ' + condition +
          ' (code ' + conditionCode + '), icon ' + iconCode);

        var dictionary = {};
        dictionary[MessageKeys.Temperature] = Math.round(temperature);
        dictionary[MessageKeys.Condition] = condition;
        dictionary[MessageKeys.WeatherIcon] = iconCode;

        // Phase 3d weather-family fields (all in the selected unit system).
        dictionary[MessageKeys.WeatherHigh] = Math.round(useCelsius ? day.maxtemp_c : day.maxtemp_f);
        dictionary[MessageKeys.WeatherLow] = Math.round(useCelsius ? day.mintemp_c : day.mintemp_f);
        dictionary[MessageKeys.Humidity] = Math.round(cur.humidity);
        dictionary[MessageKeys.Wind] = Math.round(useCelsius ? cur.wind_kph : cur.wind_mph);
        dictionary[MessageKeys.UV] = Math.round(cur.uv);
        if (cur.air_quality && cur.air_quality['us-epa-index'] !== undefined) {
          dictionary[MessageKeys.AQI] = cur.air_quality['us-epa-index'];
        }
        dictionary[MessageKeys.SunriseMin] = parseClockToMinutes(astro.sunrise);
        dictionary[MessageKeys.SunsetMin] = parseClockToMinutes(astro.sunset);

        Pebble.sendAppMessage(dictionary,
          function() {},
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

// Read weather-related settings from Clay's persisted store
function getWeatherSettings() {
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  function val(item) {
    return (item && typeof item === 'object') ? item.value : item;
  }
  return {
    useGPS: val(settings.UseGPS) !== false,  // default true
    zipCode: val(settings.ZipCode) || '',
    useCelsius: !!val(settings.TemperatureUnit)
  };
}

function getWeather() {
  var s = getWeatherSettings();

  if (s.useGPS) {
    navigator.geolocation.getCurrentPosition(
      function(pos) {
        fetchWeather(pos.coords.latitude + ',' + pos.coords.longitude, s.useCelsius);
      },
      function(err) {
        console.log('Location error: ' + err.message);
        if (s.zipCode.length > 0) {
          fetchWeather(s.zipCode, s.useCelsius);  // fall back to manual location
        }
      },
      { timeout: 15000, maximumAge: 60000 }
    );
  } else if (s.zipCode.length > 0) {
    fetchWeather(s.zipCode, s.useCelsius);
  } else {
    console.log('No location method configured');
  }
}

// Read a single Clay setting value from persisted storage.
function getClaySetting(key) {
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var item = settings[key];
  return (item && typeof item === 'object') ? item.value : item;
}

// --- ICS calendar: fetch a public .ics feed and push the next upcoming event ---

// RFC 5545 line folding: a CRLF (or LF) followed by space/tab continues a line.
function unfoldICS(text) {
  return text.replace(/\r\n[ \t]/g, '').replace(/\n[ \t]/g, '');
}

// Unescape the common ICS text escapes in a SUMMARY value.
function unescapeICS(s) {
  return s.replace(/\\n/gi, ' ').replace(/\\,/g, ',').replace(/\\;/g, ';').replace(/\\\\/g, '\\');
}

// Parse an ICS date/time (20260705, 20260705T140000, or ...T140000Z) to a Date.
function parseICSDate(val) {
  var m = val.match(/(\d{4})(\d{2})(\d{2})(?:T(\d{2})(\d{2})(\d{2})(Z)?)?/);
  if (!m) return null;
  var y = +m[1], mo = +m[2] - 1, d = +m[3];
  var hh = m[4] ? +m[4] : 0, mi = m[5] ? +m[5] : 0, ss = m[6] ? +m[6] : 0;
  if (m[7] === 'Z') return new Date(Date.UTC(y, mo, d, hh, mi, ss));
  return new Date(y, mo, d, hh, mi, ss);  // floating / TZID -> treat as local
}

// Short time string: "14:30" if today, else "Fri Jul 5".
function formatEventTime(date) {
  var now = new Date();
  if (date.toDateString() === now.toDateString()) {
    var m = date.getMinutes();
    return date.getHours() + ':' + (m < 10 ? '0' : '') + m;
  }
  var days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
  var mon = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
  return days[date.getDay()] + ' ' + mon[date.getMonth()] + ' ' + date.getDate();
}

function fetchCalendar() {
  var url = getClaySetting('CalendarUrl');
  if (!url || url.length === 0) return;

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.onload = function() {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var lines = unfoldICS(xhr.responseText).split(/\r\n|\n|\r/);
        var now = new Date();
        var best = null, bestSummary = '';
        var curStart = null, curSummary = '', inEvent = false;
        for (var i = 0; i < lines.length; i++) {
          var line = lines[i];
          if (line.indexOf('BEGIN:VEVENT') === 0) {
            inEvent = true; curStart = null; curSummary = '';
          } else if (line.indexOf('END:VEVENT') === 0) {
            if (curStart && curStart >= now && (!best || curStart < best)) {
              best = curStart; bestSummary = curSummary;
            }
            inEvent = false;
          } else if (inEvent) {
            if (line.indexOf('DTSTART') === 0) {
              curStart = parseICSDate(line.substring(line.indexOf(':') + 1));
            } else if (line.indexOf('SUMMARY') === 0) {
              curSummary = unescapeICS(line.substring(line.indexOf(':') + 1));
            }
          }
        }

        var dict = {};
        // Empty title tells the watch to show its "--" placeholder.
        dict[MessageKeys.CalendarTitle] = best ? bestSummary.substring(0, 20) : '';
        dict[MessageKeys.CalendarTime] = best ? formatEventTime(best) : '';
        Pebble.sendAppMessage(dict, function() {}, function(e) {
          console.log('Failed to send calendar: ' + JSON.stringify(e));
        });
        console.log('Calendar next event: ' + (best ? bestSummary : '(none)'));
      } catch (e) {
        console.log('Error parsing calendar: ' + e);
      }
    } else {
      console.log('Calendar request failed: ' + xhr.status);
    }
  };
  xhr.onerror = function() { console.log('Calendar request error'); };
  xhr.send();
}

// --- Crypto ticker: fetch a spot price from CoinGecko (no API key required) ---

// Compact price string: 62199 -> "62.2k", 1743 -> "1.7k", 3.5 -> "3.50", 0.42 -> "0.420".
function formatPrice(p) {
  if (p >= 1000) return (p / 1000).toFixed(1) + 'k';
  if (p >= 1) return p.toFixed(2);
  return p.toFixed(3);
}

function fetchCrypto() {
  var id = getClaySetting('CryptoId');
  if (!id) return;
  id = id.toLowerCase().trim();
  var label = getClaySetting('CryptoLabel') || id.toUpperCase().substring(0, 4);
  var url = 'https://api.coingecko.com/api/v3/simple/price?ids=' +
    encodeURIComponent(id) + '&vs_currencies=usd';

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.onload = function() {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var r = JSON.parse(xhr.responseText);
        if (r[id] && r[id].usd !== undefined) {
          var dict = {};
          dict[MessageKeys.CryptoPrice] = label + ' ' + formatPrice(r[id].usd);
          Pebble.sendAppMessage(dict, function() {}, function(e) {
            console.log('Failed to send crypto: ' + JSON.stringify(e));
          });
          console.log('Crypto ' + label + ': ' + r[id].usd);
        } else {
          console.log('Crypto id not found in response: ' + id);
        }
      } catch (e) {
        console.log('Error parsing crypto response: ' + e);
      }
    } else {
      console.log('Crypto request failed: ' + xhr.status);
    }
  };
  xhr.onerror = function() { console.log('Crypto request error'); };
  xhr.send();
}

// --- Second time zone: resolve an IANA city to a DST-correct offset ---

// Offset (minutes east of UTC) of `date` in the given IANA time zone.
function zoneOffsetMinutes(tz, date) {
  var dtf = new Intl.DateTimeFormat('en-US', {
    timeZone: tz, hour12: false,
    year: 'numeric', month: '2-digit', day: '2-digit',
    hour: '2-digit', minute: '2-digit', second: '2-digit'
  });
  var map = {};
  dtf.formatToParts(date).forEach(function(p) { map[p.type] = p.value; });
  var h = map.hour === '24' ? 0 : +map.hour;  // some engines emit '24' for midnight
  var asUTC = Date.UTC(+map.year, +map.month - 1, +map.day, h, +map.minute, +map.second);
  return Math.round((asUTC - date.getTime()) / 60000);
}

// Compute TZOffset (target minus local, in minutes) for the chosen city and push
// it to the watch. Recomputed on launch / save / periodic tick so DST stays right.
function updateSecondTimeZone() {
  var city = getClaySetting('TZCity');
  if (!city) return;
  try {
    var now = new Date();
    var localOffset = -now.getTimezoneOffset();          // minutes east of UTC
    var targetOffset = zoneOffsetMinutes(city, now);
    var dict = {};
    dict[MessageKeys.TZOffset] = targetOffset - localOffset;
    Pebble.sendAppMessage(dict, function() {}, function(e) {
      console.log('Failed to send TZOffset: ' + JSON.stringify(e));
    });
    console.log('TZ ' + city + ' offset from local: ' + (targetOffset - localOffset) + ' min');
  } catch (e) {
    console.log('Intl time-zone unavailable: ' + e);  // older JS runtimes
  }
}

// Watchface opened
Pebble.addEventListener('ready', function() {
  getWeather();
  fetchCalendar();
  updateSecondTimeZone();
  fetchCrypto();
});

// Watch requests a periodic refresh (every 30 minutes)
Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.Temperature !== undefined) {
    getWeather();
    fetchCalendar();
    updateSecondTimeZone();
    fetchCrypto();
  }
});

// Settings saved. Clay's own handler (registered first) persists the settings
// and sends the full dictionary to the watch; we refetch weather + calendar,
// delayed so it doesn't collide with Clay's in-flight AppMessage.
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) {
    setTimeout(function() { getWeather(); fetchCalendar(); updateSecondTimeZone(); fetchCrypto(); }, 2000);
  }
});
