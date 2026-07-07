// Clay live preview (Phase 3f). A custom component that draws a canvas mock-up
// of the watchface layout for the active platform, simulating per-cell colors,
// auto text-color, and B&W dithering. The pure layout/color helpers are exported
// under `_test` so they can be unit-tested off-device (node); the canvas drawing
// and Clay wiring necessarily run in the config webview.

// Per-platform display model. Rect platforms tile a grid; round platforms draw a
// center pod plus a ring. Native sizes drive the preview's aspect ratio.
var PLATFORMS = {
  aplite:  { color: false, round: false, cols: 2, rows: 2, w: 144, h: 168 },
  basalt:  { color: true,  round: false, cols: 2, rows: 2, w: 144, h: 168 },
  diorite: { color: false, round: false, cols: 2, rows: 2, w: 144, h: 168 },
  flint:   { color: false, round: false, cols: 2, rows: 2, w: 144, h: 168 },
  emery:   { color: true,  round: false, cols: 3, rows: 3, w: 200, h: 228 },
  chalk:   { color: true,  round: true,  ring: 4, w: 180, h: 180 },
  gabbro:  { color: true,  round: true,  ring: 8, w: 260, h: 260 }
};

// Short labels shown inside each preview cell.
var MODULE_ABBR = {
  '0': '', '1': 'DATE', '2': 'WTHR', '3': 'TIME', '4': 'STAT', '5': 'TZ',
  '6': 'WEEK', '7': 'CNTD', '8': 'DIST', '9': 'EVENT', '10': 'BT', '11': 'HR',
  '12': 'RING', '13': 'SLEEP', '14': 'KCAL', '15': 'ACTV', '16': 'MOON',
  '17': 'DOY', '18': 'CNTUP', '19': 'TEXT', '20': 'QUIET', '21': 'CLOCK',
  '22': 'SUN', '23': 'HI/LO', '24': 'HUM', '25': 'WIND', '26': 'UV',
  '27': 'AQI', '28': 'CRYP'
};

// Watch-side default module assignment for round layouts (Time in the center
// pod, then Date/Weather/Week/Distance/... clockwise) — mirrors the round
// s_quadrant_modules defaults in modules.c. Grid platforms use the config's
// own CELL_DEFAULTS, which already match the C grid defaults.
var ROUND_MODULE_DEFAULTS = ['3', '1', '2', '6', '8', '4', '7', '9', '5'];

function platformModel(platform) {
  return PLATFORMS[platform] || PLATFORMS.basalt;
}

// How many cells this platform shows (grid area, or 1 + ring pods).
function cellCount(platform) {
  var m = platformModel(platform);
  return m.round ? (1 + m.ring) : (m.cols * m.rows);
}

// Parse a Clay color value ("0xAABBCC", "AABBCC", or an int) to {r,g,b}.
function parseColor(v) {
  if (v === undefined || v === null) return { r: 170, g: 170, b: 170 };
  var n;
  if (typeof v === 'number') {
    n = v;
  } else {
    var s = String(v).replace(/^0x/i, '').replace(/^#/, '');
    n = parseInt(s, 16);
  }
  if (isNaN(n)) return { r: 170, g: 170, b: 170 };
  return { r: (n >> 16) & 0xff, g: (n >> 8) & 0xff, b: n & 0xff };
}

// Perceived brightness 0..255 (same weighting as the watch's auto-text logic).
function brightness(rgb) {
  return (rgb.r * 299 + rgb.g * 587 + rgb.b * 114) / 1000;
}

// Resolve a cell's background + text colors for the preview, honoring color vs
// B&W platforms and the auto/manual text-color setting.
function resolveColors(cell, isColor) {
  var bgEnabled = !!cell.bg;
  var bg, text;
  if (!bgEnabled) {
    bg = '#ffffff';
  } else if (isColor) {
    var c = parseColor(cell.color);
    bg = 'rgb(' + c.r + ',' + c.g + ',' + c.b + ')';
  } else {
    bg = '#aaaaaa'; // B&W: enabled background dithers to gray
  }

  if (!cell.autoText && isColor) {
    var t = parseColor(cell.textColor);
    text = 'rgb(' + t.r + ',' + t.g + ',' + t.b + ')';
  } else if (bgEnabled && isColor && brightness(parseColor(cell.color)) < 128) {
    text = '#ffffff';
  } else {
    text = '#000000';
  }
  return { bg: bg, text: text, dither: bgEnabled && !isColor };
}

// Cell rectangles for a grid platform within a WxH canvas.
function gridRects(platform, W, H) {
  var m = platformModel(platform);
  var cw = W / m.cols, ch = H / m.rows, rects = [];
  for (var i = 0; i < m.cols * m.rows; i++) {
    rects.push({ x: (i % m.cols) * cw, y: Math.floor(i / m.cols) * ch, w: cw, h: ch });
  }
  return rects;
}

// Cell rectangles for a round platform: center pod (index 0) + ring pods,
// starting at 12 o'clock and going clockwise (mirrors the watch renderer).
function roundRects(platform, W, H) {
  var m = platformModel(platform);
  var cx = W / 2, cy = H / 2;
  var cw = W * 0.40, chh = H * 0.44;
  var rects = [{ x: cx - cw / 2, y: cy - chh / 2, w: cw, h: chh }];
  var r = W * 0.35, pw = W * 0.28, ph = H * 0.20;
  for (var i = 0; i < m.ring; i++) {
    var a = (2 * Math.PI * i) / m.ring - Math.PI / 2;
    var px = cx + r * Math.cos(a), py = cy + r * Math.sin(a);
    rects.push({ x: px - pw / 2, y: py - ph / 2, w: pw, h: ph });
  }
  return rects;
}

function cellRects(platform, W, H) {
  return platformModel(platform).round ? roundRects(platform, W, H) : gridRects(platform, W, H);
}

// ---- Canvas drawing (browser only) ----------------------------------------

function roundedRect(ctx, x, y, w, h, r) {
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  ctx.closePath();
}

// Fill a cell rect with a coarse checkerboard to hint at B&W dithering.
function fillDither(ctx, rect) {
  ctx.save();
  ctx.fillStyle = '#ffffff';
  ctx.fillRect(rect.x, rect.y, rect.w, rect.h);
  ctx.fillStyle = '#000000';
  var step = 3;
  for (var yy = 0; yy < rect.h; yy += step) {
    for (var xx = ((yy / step) % 2) * step; xx < rect.w; xx += step * 2) {
      ctx.fillRect(rect.x + xx, rect.y + yy, 1.5, 1.5);
    }
  }
  ctx.restore();
}

function render(canvas, platform, settings) {
  if (!canvas || !canvas.getContext) return;
  var m = platformModel(platform);
  var PREVIEW_W = 132, PREVIEW_H = Math.round(PREVIEW_W * m.h / m.w);
  canvas.width = PREVIEW_W;
  canvas.height = PREVIEW_H;
  var ctx = canvas.getContext('2d');

  ctx.clearRect(0, 0, PREVIEW_W, PREVIEW_H);
  var rects = cellRects(platform, PREVIEW_W, PREVIEW_H);

  if (m.round) {
    // Face backdrop.
    ctx.fillStyle = '#ffffff';
    ctx.beginPath();
    ctx.arc(PREVIEW_W / 2, PREVIEW_H / 2, PREVIEW_W / 2 - 1, 0, 2 * Math.PI);
    ctx.fill();
    ctx.strokeStyle = '#000000';
    ctx.stroke();
  }

  for (var i = 0; i < rects.length; i++) {
    var rect = rects[i];
    var cell = settings.cells[i] || { module: '0', bg: false, autoText: true };
    var col = resolveColors(cell, m.color);

    if (m.round) {
      roundedRect(ctx, rect.x, rect.y, rect.w, rect.h, 6);
      ctx.save(); ctx.clip();
      if (col.dither) { fillDither(ctx, rect); }
      else { ctx.fillStyle = col.bg; ctx.fillRect(rect.x, rect.y, rect.w, rect.h); }
      ctx.restore();
      ctx.strokeStyle = '#888888'; roundedRect(ctx, rect.x, rect.y, rect.w, rect.h, 6); ctx.stroke();
    } else {
      if (col.dither) { fillDither(ctx, rect); }
      else { ctx.fillStyle = col.bg; ctx.fillRect(rect.x, rect.y, rect.w, rect.h); }
      ctx.strokeStyle = '#000000'; ctx.lineWidth = 0.5;
      ctx.strokeRect(rect.x, rect.y, rect.w, rect.h);
    }

    var label = MODULE_ABBR[String(cell.module)] || '';
    if (label) {
      ctx.fillStyle = col.text;
      ctx.font = 'bold 9px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
    }
  }
}

// ---- Clay custom component + wiring ---------------------------------------

var component = {
  name: 'watchPreview',
  template:
    '<div class="component component-watchpreview" style="text-align:center;padding:12px 0;">' +
    '  <canvas class="preview-canvas" data-manipulator-target ' +
    '          style="border:1px solid rgba(128,128,128,0.4);"></canvas>' +
    '  <div class="preview-caption" style="font-size:11px;opacity:0.55;margin-top:4px;">Live preview</div>' +
    '</div>',
  // Display-only: no value to persist or send to the watch.
  manipulator: { set: function() {}, get: function() { return null; } },
  defaults: {}
};

// Cell-numbering explainer for the active platform, shown above the dropdowns.
function layoutHelpText(platform) {
  var m = platformModel(platform);
  if (m.round) {
    return 'Round layout: Cell 1 is the center pod; cells 2-' + (1 + m.ring) +
      ' go clockwise around the ring, starting at the top.';
  }
  return m.cols + 'x' + m.rows + ' grid: cells 1-' + (m.cols * m.rows) +
    ' run left to right, top to bottom.';
}

// On round watches, if the user has never saved settings, preload the module
// dropdowns with the watch's own round defaults so the first save doesn't
// silently swap the center pod from Time to Date.
function applyRoundDefaults(clayConfig, platform, savedSettings) {
  if (!platformModel(platform).round) return false;
  if (savedSettings && typeof savedSettings.Quadrant1Module !== 'undefined') return false;
  for (var i = 1; i <= 9; i++) {
    var it = clayConfig.getItemByMessageKey('Quadrant' + i + 'Module');
    if (it && typeof it.set === 'function') { it.set(ROUND_MODULE_DEFAULTS[i - 1]); }
  }
  return true;
}

function getVal(clayConfig, key, dflt) {
  var it = clayConfig.getItemByMessageKey(key);
  return (it && typeof it.get === 'function') ? it.get() : dflt;
}

function readSettings(clayConfig) {
  var cells = [];
  for (var i = 1; i <= 9; i++) {
    cells.push({
      module: getVal(clayConfig, 'Quadrant' + i + 'Module', '0'),
      bg: !!getVal(clayConfig, 'Quadrant' + i + 'Background', false),
      color: getVal(clayConfig, 'Quadrant' + i + 'Color', '0xAAAAAA'),
      autoText: getVal(clayConfig, 'Quadrant' + i + 'AutoTextColor', true),
      textColor: getVal(clayConfig, 'Quadrant' + i + 'TextColor', '0x000000')
    });
  }
  return { cells: cells };
}

// Register the component and, after build, draw + keep the preview in sync.
function attach(clayConfig, minified) {
  clayConfig.registerComponent(component);
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var awi = clayConfig.meta && clayConfig.meta.activeWatchInfo;
    var platform = (awi && awi.platform) || 'basalt';

    // Platform-aware touches beyond the preview itself: cell-numbering help
    // text and (round watches, first run) the watch's own module defaults.
    var help = clayConfig.getItemById && clayConfig.getItemById('layoutHelp');
    if (help && typeof help.set === 'function') { help.set(layoutHelpText(platform)); }
    var saved = (typeof window !== 'undefined') ? window.claySettings : null;
    applyRoundDefaults(clayConfig, platform, saved);

    var previews = clayConfig.getItemsByType('watchPreview');
    if (!previews.length) return;

    function redraw() {
      var settings = readSettings(clayConfig);
      previews.forEach(function(item) {
        var canvas = item.$element.select('.preview-canvas')[0];
        render(canvas, platform, settings);
      });
    }
    redraw();

    var suffixes = ['Module', 'Background', 'Color', 'AutoTextColor', 'TextColor'];
    for (var i = 1; i <= 9; i++) {
      suffixes.forEach(function(sfx) {
        var it = clayConfig.getItemByMessageKey('Quadrant' + i + sfx);
        if (it && typeof it.on === 'function') { it.on('change', redraw); }
      });
    }
  });
}

module.exports = {
  component: component,
  attach: attach,
  render: render,
  // Pure helpers exported for off-device unit tests.
  _test: {
    PLATFORMS: PLATFORMS, MODULE_ABBR: MODULE_ABBR, cellCount: cellCount,
    parseColor: parseColor, brightness: brightness, resolveColors: resolveColors,
    cellRects: cellRects, platformModel: platformModel,
    ROUND_MODULE_DEFAULTS: ROUND_MODULE_DEFAULTS,
    layoutHelpText: layoutHelpText, applyRoundDefaults: applyRoundDefaults
  }
};
