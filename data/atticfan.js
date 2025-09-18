// --- System Info Modal Logic ---
function openSystemInfoModal() {
  // Show modal
  document.getElementById('systemInfoModal').style.display = 'block';
  // Load system info (mock for now)
  loadSystemInfo();
}

function closeSystemInfoModal() {
  document.getElementById('systemInfoModal').style.display = 'none';
}

function loadSystemInfo() {
  fetch('/system_info')
    .then(res => res.json())
    .then(info => {
      const fsInfo = info.fs_total !== null ? `${(info.fs_free/1024).toFixed(1)} KB free / ${(info.fs_total/1024).toFixed(1)} KB total` : '--';
      const uptime = info.uptime_ms !== undefined ? formatUptime(info.uptime_ms) : '--';
      const html = `
        <div><strong>Board:</strong> ${info.board || '--'}</div>
        <div><strong>Firmware:</strong> ${info.firmware || '--'}</div>
        <div><strong>Filesystem:</strong> ${fsInfo}</div>
        <div><strong>Memory Free:</strong> ${info.memory_free !== undefined ? info.memory_free + ' bytes' : '--'}</div>
        <div><strong>Uptime:</strong> ${uptime}</div>
        <div><strong>IP Address:</strong> ${info.ip || '--'}</div>
      `;
      document.getElementById('systemInfoContent').innerHTML = html;
    })
    .catch(() => {
      document.getElementById('systemInfoContent').innerHTML = '<div class="error-message">Unable to load system info.</div>';
    });
}

function formatUptime(ms) {
  const sec = Math.floor(ms / 1000);
  const days = Math.floor(sec / 86400);
  const hours = Math.floor((sec % 86400) / 3600);
  const minutes = Math.floor((sec % 3600) / 60);
  const seconds = sec % 60;
  let str = '';
  if (days > 0) str += days + 'd ';
  if (hours > 0 || days > 0) str += hours + 'h ';
  if (minutes > 0 || hours > 0 || days > 0) str += minutes + 'm ';
  str += seconds + 's';
  return str.trim();
}

// Make modal functions global
window.openSystemInfoModal = openSystemInfoModal;
window.closeSystemInfoModal = closeSystemInfoModal;
function renderForecast(container, forecast, isMock = false) {
  container.innerHTML = "";
  forecast.forEach((day, index) => {
    let dayOfWeek, icon, max, min;
    if (isMock) {
      dayOfWeek = day.dayOfWeek;
      icon = day.icon;
      max = day.max;
      min = day.min;
    } else {
      dayOfWeek = day.dayOfWeek;
      icon = day.icon;
      max = day.max;
      min = day.min;
    }
    const dayDiv = document.createElement("div");
    dayDiv.className = "forecast-day";
    dayDiv.innerHTML = `
      <div>${dayOfWeekToString(dayOfWeek, index === 0)}</div>
      <div class="emoji">${icon}</div>
      <div><strong>${max}°</strong> / ${min}°</div>
    `;
    container.appendChild(dayDiv);
  });
}
// Set to true to enable debug logging
const DEBUG = false;
function debugLog(...args) { if (DEBUG) console.log(...args); }
function debugWarn(...args) { if (DEBUG) console.warn(...args); }
function debugError(...args) { if (DEBUG) console.error(...args); }

function saveCoreAutomation() {
  saveConfig();
}

function saveSmartPreCooling() {
  saveConfig();
}

window.saveCoreAutomation = saveCoreAutomation;
window.saveSmartPreCooling = saveSmartPreCooling;
async function fetchAndRenderHistory() {
  const chartMsgEl = document.getElementById('historyChartMsg');
  const chartEl = document.getElementById('historyChart');
  debugLog('[AtticFan] Fetching /history.csv...');
  try {
    const res = await fetch('/history.csv');
    if (!res.ok) {
  debugWarn('[AtticFan] /history.csv not found or not OK:', res.status);
      throw new Error('No history log found.');
    }
  debugLog('[AtticFan] /history.csv fetch OK');
    const csv = await res.text();
  debugLog('[AtticFan] /history.csv loaded, length:', csv.length);
    const lines = csv.trim().split(/\r?\n/);
    if (lines.length < 2) {
  debugWarn('[AtticFan] /history.csv has no data rows.');
      chartMsgEl.textContent = 'No historical data logged yet.';
      if (historyChart) historyChart.destroy();
      return;
    }
    const header = lines[0].split(',');
    const timeIdx = header.indexOf('timestamp');
    const atticIdx = header.indexOf('attic_temp');
    if (timeIdx < 0 || atticIdx < 0) {
  debugError('[AtticFan] /history.csv missing required columns.');
      chartMsgEl.textContent = 'CSV format error: missing columns.';
      return;
    }
    const outdoorIdx = header.indexOf('outdoor_temp');
    const humidityIdx = header.indexOf('humidity');
    const fanOnIdx = header.indexOf('fan_on');
    const labels = [];
    const attic = [];
    const outdoor = [];
    const humidity = [];
    const fanOn = [];
    for (let i = 1; i < lines.length; ++i) {
      const row = lines[i].split(',');
      const ts = row[timeIdx];
      if (ts) {
        labels.push(ts.substring(5, 16).replace('T', ' '));
      } else {
        labels.push('');
      }
      attic.push(parseFloat(row[atticIdx]));
      if (outdoorIdx >= 0) outdoor.push(parseFloat(row[outdoorIdx]));
      if (humidityIdx >= 0) humidity.push(parseFloat(row[humidityIdx]));
      if (fanOnIdx >= 0) fanOn.push(parseInt(row[fanOnIdx], 10));
    }
    const N = 100;
    const start = labels.length > N ? labels.length - N : 0;
    const chartLabels = labels.slice(start);
    const chartAttic = attic.slice(start);
    const chartOutdoor = outdoorIdx >= 0 ? outdoor.slice(start) : undefined;
    const chartHumidity = humidityIdx >= 0 ? humidity.slice(start) : undefined;
    const chartFanOn = fanOnIdx >= 0 ? fanOn.slice(start) : undefined;
    const datasets = [
      { label: 'Attic Temp (°F)', data: chartAttic, borderColor: '#e67e22', backgroundColor: 'rgba(230,126,34,0.1)', yAxisID: 'y', tension: 0.2 },
      { label: 'Outdoor Temp (°F)', data: chartOutdoor, borderColor: '#2980b9', backgroundColor: 'rgba(41,128,185,0.1)', yAxisID: 'y', tension: 0.2, hidden: chartOutdoor === undefined }
    ];
    if (chartHumidity) datasets.push({ label: 'Attic Humidity (%)', data: chartHumidity, borderColor: '#27ae60', backgroundColor: 'rgba(39,174,96,0.1)', yAxisID: 'y2', tension: 0.2, hidden: chartHumidity === undefined });
    if (chartFanOn) datasets.push({ label: 'Fan On', data: chartFanOn, borderColor: '#9b59b6', backgroundColor: 'rgba(155,89,182,0.2)', yAxisID: 'y3', tension: 0.2, stepped: true, fill: true, hidden: chartFanOn === undefined });
    const ctx = chartEl.getContext('2d');
    if (historyChart) historyChart.destroy();
  debugLog('[AtticFan] Chart rendered with', chartLabels.length, 'points.');
    historyChart = new Chart(ctx, {
      type: 'line',
      data: { labels: chartLabels, datasets: datasets },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        interaction: { mode: 'index', intersect: false },
        plugins: {
          legend: { position: 'top' },
          title: { display: false },
          tooltip: { enabled: true }
        },
        scales: {
          y: {
            type: 'linear',
            display: true,
            position: 'left',
            title: { display: true, text: 'Temperature (°F)' }
          },
          y2: { type: 'linear', display: chartHumidity !== undefined, position: 'right', title: { display: true, text: 'Humidity (%)' }, grid: { drawOnChartArea: false }, min: 0, max: 100 },
          y3: { type: 'linear', display: false, position: 'right', min: -0.5, max: 1.5 }
        }
      }
    });
    chartMsgEl.textContent = '';
  } catch (err) {
  debugError('[AtticFan] Error loading /history.csv:', err);
    chartMsgEl.textContent = 'No historical data available.';
    if (historyChart) historyChart.destroy();
  }
}

(() => {
  const isLocalDev = location.hostname === "" || location.hostname === "localhost" || location.hostname === "127.0.0.1" || location.protocol === "file:";
  if (!isLocalDev) return;
  console.info("ESP not detected, running in local test mode.");
  let __fanOn=false, __fanMode='AUTO', __timerStart=0, __timerEnd=0, __postAction='stay_manual';
  const mockConfig={fanOnTemp:95,fanDeltaTemp:2,fanHysteresis:2,preCoolTriggerTemp:78,preCoolTempOffset:2,preCoolingEnabled:false,onboardLedEnabled:true,dailyRestartEnabled:false,mqttEnabled:false,mqttDiscoveryEnabled:false,testModeEnabled:false,restartHour:3,restartMinute:0};
  const now=Date.now(), pad=n=>(n<10?'0':'')+n, fmt=t=>{const d=new Date(t);return `${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`};
  const rows=[]; for(let i=0;i<96;i++){const t=now-(95-i)*15*60*1000; rows.push(`${fmt(t)},95,92,40,0`);} const mockHistory="timestamp,attic_temp,outdoor_temp,attic_humidity,fan_on\n"+rows.join("\n");
  const mockWeather={currentIcon:"☀️",currentTemp:72.4,currentHumidity:53,forecast:[{dayOfWeek:(new Date()).getDay(),icon:"⛅️",max:83,min:60},{dayOfWeek:(new Date(Date.now()+86400000)).getDay(),icon:"☀️",max:88,min:60},{dayOfWeek:(new Date(Date.now()+2*86400000)).getDay(),icon:"☀️",max:91,min:62}]};
  const getMockStatus = () => {
    const now=Date.now(),inDelay=(__timerStart>0&&now<__timerStart&&__timerEnd>0),inRun=(__timerStart>0&&now>=__timerStart&&now<__timerEnd),remaining=Math.max(0,Math.floor(((__timerEnd>0?__timerEnd:0)-now)/1000));
    const fanOnNow=inRun?true:__fanOn;
    return {
      firmwareVersion:"v0.0.0-demo",atticTemp:95,atticHumidity:40,outdoorTemp:92,fanOn:fanOnNow,fanMode:__fanMode,timerActive:(__timerEnd>0&&(inDelay||inRun)),timerRemainingSec:remaining,
      // --- Added for indoor sensor testing ---
      indoorSensorsEnabled: true,
      indoorSensorCount: 2,
      avgIndoorTemp: '72.2',
      avgIndoorHumidity: '45.3'
    };
  }
  const mockIndoorSensors = {
      "sensors": [
          { "sensorId": "living_room_01", "name": "Living Room", "temperature": "72.5", "humidity": "45.1", "ipAddress": "192.168.1.150", "secondsSinceUpdate": 25 },
          { "sensorId": "bedroom_01", "name": "Master Bedroom", "temperature": "70.2", "humidity": "48.9", "ipAddress": "192.168.1.151", "secondsSinceUpdate": 45 },
          { "sensorId": "office_01", "name": "Office", "temperature": "73.8", "humidity": "42.0", "ipAddress": "192.168.1.152", "secondsSinceUpdate": 310 }
      ], "count": 3, "averageTemperature": "72.2", "averageHumidity": "45.3"
  };
  const F=window.fetch.bind(window);
  window.fetch = (input,init) => {
    try {
      const method = (init && (init.method || 'GET').toUpperCase()) || 'GET';
      const raw = (typeof input === 'string') ? input : input.url;
      const u = new URL(raw, location.origin);
      const full = u.pathname + (u.search || "");
      if (/^\/fan\?state=ping\b/.test(full)) return Promise.resolve(new Response('', { status: 200 }));
      if (/^\/fan\?state=on\b/.test(full)) { __fanOn = true; __fanMode = 'MANUAL'; return Promise.resolve(new Response('', { status: 200 })); }
      if (/^\/fan\?state=off\b/.test(full)) { __fanOn = false; __fanMode = 'MANUAL'; return Promise.resolve(new Response('', { status: 200 })); }
      if (/^\/fan\?state=auto\b/.test(full)) { __fanMode = 'AUTO'; return Promise.resolve(new Response('', { status: 200 })); }
      if (full === '/status') return Promise.resolve(new Response(JSON.stringify(getMockStatus()), { status: 200, headers: { 'Content-Type': 'application/json' } }));
      if (full === '/indoor_sensors') return Promise.resolve(new Response(JSON.stringify(mockIndoorSensors), { status: 200, headers: { 'Content-Type': 'application/json' } }));
      if (full === '/config' && method === 'GET') return Promise.resolve(new Response(JSON.stringify(mockConfig), { status: 200, headers: { 'Content-Type': 'application/json' } }));
      if (full === '/config' && method === 'POST') {
        try {
          const body = init && typeof init.body === 'string' ? JSON.parse(init.body) : {};
          Object.assign(mockConfig, body || {});
          return Promise.resolve(new Response('Configuration saved!', { status: 200 }));
        } catch (e) {
          return Promise.resolve(new Response('Invalid JSON', { status: 400 }));
        }
      }
      if (full === '/history.csv' && method === 'GET') return Promise.resolve(new Response(mockHistory, { status: 200, headers: { 'Content-Type': 'text/csv' } }));
      if (u.pathname === '/fan' && method === 'POST') {
        try {
          const body = init && typeof init.body === 'string' ? JSON.parse(init.body) : {};
          if (body && body.action === 'start_timed') {
            const delayMs = (parseInt(body.delay || 0, 10) || 0) * 60 * 1000,
              durationMs = (parseInt(body.duration || 0, 10) || 0) * 60 * 1000;
            __postAction = (body.postAction || 'stay_manual');
            __fanMode = 'MANUAL';
            const t0 = Date.now();
            __timerStart = t0 + delayMs;
            __timerEnd = __timerStart + durationMs;
            if (delayMs === 0) { __fanOn = true; }
            try { if (window.updateSensorData) setTimeout(window.updateSensorData, 50); } catch (e) { }
            if (delayMs > 0) try { if (window.updateSensorData) setTimeout(window.updateSensorData, delayMs + 250); } catch (e) { }
            clearInterval(window.__timerTick);
            window.__timerTick = setInterval(() => {
              const nowT = Date.now();
              if (__timerStart && nowT >= __timerStart && nowT < __timerEnd) __fanOn = true;
              if (__timerEnd && nowT >= __timerEnd) {
                clearInterval(window.__timerTick);
                __timerStart = 0;
                __timerEnd = 0;
                if (__postAction === 'go_auto') { __fanMode = 'AUTO'; __fanOn = false; } else { __fanMode = 'MANUAL'; __fanOn = false; }
                try { if (window.updateSensorData) setTimeout(window.updateSensorData, 50); } catch (e) { }
              }
            }, 1000);
            return Promise.resolve(new Response('', { status: 200 }));
          }
        } catch (e) { }
        return Promise.resolve(new Response('', { status: 200 }));
      }
      return F(input, init);
    } catch (e) {
      return F(input, init);
    }
  };
})();

let testMode = false;
let isInteractingWithSlider = false;
let historyChart;
let currentFanState = false;
const fanIconEl = document.getElementById("fan-icon");
const fanKeyframes = `
  @keyframes spin {
    from { transform: rotate(0deg); }
    to { transform: rotate(360deg); }
  }`;
const styleSheet = document.createElement("style");
styleSheet.innerText = fanKeyframes;
document.head.appendChild(styleSheet);

function buildConfigPayload(fields) {
  const payload = {};
  fields.forEach(field => {
    const el = document.getElementById(field.id);
    if (!el) return;
    let value = el.type === 'checkbox' ? el.checked : el.value;
    if (field.parse) value = field.parse(value);
    payload[field.key] = value;
  });
  return payload;
}

let allIndoorSensors = []; // Cache for modal
function updateSensorData() {
  if (testMode) {
    return;
  }
  fetch("/status")
    .then(res => res.json())
    .then(data => {
      document.getElementById("firmwareVersion").textContent = data.firmwareVersion;
      document.getElementById("atticTemp").textContent = data.atticTemp;
      document.getElementById("atticHumidity").textContent = data.atticHumidity;
      document.getElementById("outdoorTemp").textContent = data.outdoorTemp;
      const fanOn = data.fanOn;
      currentFanState = fanOn;
      updateFanVisual(fanOn);
      document.getElementById("currentMode").textContent = data.fanMode;
      const timerStatusEl = document.getElementById('timerStatus');
      if (data.timerActive) {
        const minutes = Math.floor(data.timerRemainingSec / 60);
        const seconds = data.timerRemainingSec % 60;
        const remainingTime = `${minutes}m ${String(seconds).padStart(2, '0')}s`;
        if (data.timerMode === 'delay') {
          timerStatusEl.innerHTML = `Timer starts in: ${remainingTime}`;
        } else {
          timerStatusEl.innerHTML = `Timer active for: ${remainingTime}`;
        }
        timerStatusEl.style.display = 'block'; // Make it visible
        timerStatusEl.style.textAlign = 'center'; // Center the text
        timerStatusEl.style.marginTop = '8px'; // Add some space above it
      } else {
        timerStatusEl.style.display = 'none'; // Hide it
      }
      if (data.testModeEnabled) {
        document.getElementById('test-panel').style.display = 'block';
        const atticSlider = document.getElementById('simulatedAtticTemp');
        const outdoorSlider = document.getElementById('simulatedOutdoorTemp');
        if (!isInteractingWithSlider) {
          atticSlider.value = data.simulatedAtticTemp;
          outdoorSlider.value = data.simulatedOutdoorTemp;
        }
        updateTempDisplay('attic-temp-val', atticSlider.value);
        updateTempDisplay('outdoor-temp-val', outdoorSlider.value);
      }
      setMode(data.fanMode, false); // Update UI without sending a command back
      if (data.fanMode === 'MANUAL') {
  const manualOnBtn = document.getElementById("manualOnBtn");
  const manualOffBtn = document.getElementById("manualOffBtn");
  if (manualOnBtn) manualOnBtn.disabled = fanOn;
  if (manualOffBtn) manualOffBtn.disabled = !fanOn;
      } else {
        // If not in manual mode, disable all manual buttons
        if (manualOnBtn) manualOnBtn.disabled = true;
        if (manualOffBtn) manualOffBtn.disabled = true;
      }
    })
    .catch(() => {
      document.getElementById("firmwareVersion").textContent = "--";
      document.getElementById("atticTemp").textContent = "--";
      document.getElementById("atticHumidity").textContent = "--";
      document.getElementById("outdoorTemp").textContent = "--";
      document.getElementById("currentMode").textContent = "--";
    });

  // Fetch and handle indoor sensor display logic
  fetch("/indoor_sensors")
    .then(res => res.json())
    .then(data => {
      allIndoorSensors = data.sensors || []; // Cache for modal
      const count = data.count || 0;

      const avgDisplay = document.getElementById('indoorSensorDisplay');
      const sensor1Display = document.getElementById('indoorSensor1');
      const sensor2Display = document.getElementById('indoorSensor2');

      // Hide all first
      if (avgDisplay) avgDisplay.style.display = 'none';
      if (sensor1Display) sensor1Display.style.display = 'none';
      if (sensor2Display) sensor2Display.style.display = 'none';

      if (count > 2) {
        // Show average card
        if (avgDisplay) avgDisplay.style.display = 'block';
        document.getElementById('avgIndoorTemp').textContent = data.averageTemperature || '--';
        document.getElementById('avgIndoorHumidity').textContent = data.averageHumidity || '--';
      } else if (count === 2) {
        // Show two individual cards
        if (sensor1Display) sensor1Display.style.display = 'block';
        document.getElementById('indoorSensor1Name').textContent = data.sensors[0].name;
        document.getElementById('indoorSensor1Temp').textContent = data.sensors[0].temperature;
        document.getElementById('indoorSensor1Humidity').textContent = data.sensors[0].humidity;

        if (sensor2Display) sensor2Display.style.display = 'block';
        document.getElementById('indoorSensor2Name').textContent = data.sensors[1].name;
        document.getElementById('indoorSensor2Temp').textContent = data.sensors[1].temperature;
        document.getElementById('indoorSensor2Humidity').textContent = data.sensors[1].humidity;
      } else if (count === 1) {
        // Show one individual card
        if (sensor1Display) sensor1Display.style.display = 'block';
        document.getElementById('indoorSensor1Name').textContent = data.sensors[0].name;
        document.getElementById('indoorSensor1Temp').textContent = data.sensors[0].temperature;
        document.getElementById('indoorSensor1Humidity').textContent = data.sensors[0].humidity;
      }
    })
    .catch(err => {
      debugError("Failed to fetch indoor sensors:", err);
      allIndoorSensors = [];
    });
}
function updateWeatherData() {
  const iconEl = document.getElementById("currentWeatherIcon");
  const tempEl = document.getElementById("currentWeatherTemp");
  const humidityEl = document.getElementById("currentWeatherHumidity");
  const forecastContainer = document.getElementById("forecastContainer");
  const isLocalDev = location.hostname === "" || location.hostname === "localhost" || location.hostname === "127.0.0.1" || location.protocol === "file:";
  debugLog('[AtticFan] updateWeatherData called.');
  if (isLocalDev) {
    const lat = 38.72;
    const lon = -121.36;
    const url = `http://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&current=temperature_2m,relativehumidity_2m,weathercode&daily=weathercode,temperature_2m_max,temperature_2m_min,sunrise,sunset&hourly=temperature_2m,weathercode&temperature_unit=fahrenheit&windspeed_unit=mph&precipitation_unit=inch&forecast_days=3&timezone=auto`;
  debugLog('[AtticFan] Fetching weather from Open-Meteo:', url);
    (async () => {
      try {
        const res = await fetch(url);
  debugLog('[AtticFan] Open-Meteo response status:', res.status);
        const data = await res.json();
  debugLog('[AtticFan] Open-Meteo data:', data);
        iconEl.textContent = weatherCodeToEmoji(data.current.weathercode);
        tempEl.textContent = data.current.temperature_2m.toFixed(1);
        humidityEl.textContent = data.current.relativehumidity_2m;
        
        // Update sunrise/sunset display
        updateSunriseSunset(data.daily.sunrise[0], data.daily.sunset[0]);
        
        // Build forecast array for rendering
        const forecast = data.daily.time.map((time, index) => ({
          dayOfWeek: new Date(time).getUTCDay(),
          icon: weatherCodeToEmoji(data.daily.weathercode[index]),
          max: Math.round(data.daily.temperature_2m_max[index]),
          min: Math.round(data.daily.temperature_2m_min[index])
        }));
        renderForecast(forecastContainer, forecast);
        
        // Render hourly forecast
        if (data.hourly && data.hourly.time) {
          const hourlyData = data.hourly.time.slice(0, 5).map((time, index) => ({
            time: time,
            temperature: data.hourly.temperature_2m[index],
            weatherCode: data.hourly.weathercode[index]
          }));
          renderHourlyForecast(hourlyData);
        }
      } catch (err) {
  debugWarn('[AtticFan] Open-Meteo fetch failed, using mock weather data:', err);
        forecastContainer.innerHTML = "";
        iconEl.textContent = "☀️";
        tempEl.textContent = "75";
        humidityEl.textContent = "55";
        
        // Mock sunrise/sunset
        updateSunriseSunset("06:30", "19:45");
        
        const mockForecast = [
          { dayOfWeek: new Date().getUTCDay(), icon: '☀️', max: '85', min: '65' },
          { dayOfWeek: (new Date().getUTCDay() + 1) % 7, icon: '⛅', max: '82', min: '63' },
          { dayOfWeek: (new Date().getUTCDay() + 2) % 7, icon: '🌦️', max: '78', min: '60' }
        ];
        renderForecast(forecastContainer, mockForecast, true);
        
        // Mock hourly forecast
        const now = new Date();
        const mockHourlyData = [];
        for (let i = 0; i < 5; i++) {
          const hour = new Date(now.getTime() + i * 60 * 60 * 1000);
          const timeStr = hour.toISOString().substring(0, 16);
          mockHourlyData.push({
            time: timeStr,
            temperature: 75 + Math.random() * 10,
            weatherCode: [0, 1, 1, 80, 61][i]
          });
        }
        renderHourlyForecast(mockHourlyData);
      }
    })();
    return;
  }
  debugLog('[AtticFan] Fetching /weather from backend...');
  (async () => {
    try {
      const res = await fetch("/weather");
  debugLog('[AtticFan] /weather response status:', res.status);
      const data = await res.json();
  debugLog('[AtticFan] /weather data:', data);
      iconEl.textContent = data.currentIcon;
      tempEl.textContent = data.currentTemp;
      humidityEl.textContent = data.currentHumidity;
      
      // Update sunrise/sunset if available
      if (data.sunrise && data.sunset) {
        updateSunriseSunset(data.sunrise, data.sunset);
      } else {
        // Clear any existing sunrise/sunset display if data isn't available
        const sunriseEl = document.getElementById('sunriseTime');
        const sunsetEl = document.getElementById('sunsetTime');
        if (sunriseEl) sunriseEl.textContent = '🌅 --:--';
        if (sunsetEl) sunsetEl.textContent = '🌇 --:--';
      }
      
      renderForecast(forecastContainer, data.forecast);
      
      // Render hourly forecast if available
      if (data.hourly) {
        renderHourlyForecast(data.hourly);
      }
    } catch (err) {
  debugError('[AtticFan] Failed to fetch /weather:', err);
    }
  })();
}
function dayOfWeekToString(dayIndex, isToday) {
  if (isToday) return "Today";
  const days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
  if (dayIndex >= 0 && dayIndex < 7) {
    return days[dayIndex];
  }
}
function weatherCodeToEmoji(code) {
  if (code === 0) return "☀️"; // Clear sky
  if (code >= 1 && code <= 3) return "⛅"; // Mainly clear, partly cloudy, overcast
  if (code >= 45 && code <= 48) return "🌫️"; // Fog
  if (code >= 51 && code <= 57) return "💧"; // Drizzle
  if (code >= 61 && code <= 67) return "🌧️"; // Rain
  if (code >= 71 && code <= 77) return "❄️"; // Snow
  if (code >= 80 && code <= 82) return "🌦️"; // Rain showers
  if (code >= 85 && code <= 86) return "🌨️"; // Snow showers
  if (code >= 95 && code <= 99) return "⛈️"; // Thunderstorm
  return "❓";
}
function setMode(newMode, sendCommand = true) {
  const isManual = newMode === 'MANUAL';
  document.getElementById('autoModeBtn').disabled = !isManual;
  document.getElementById('manualModeBtn').disabled = isManual;
  if (testMode) {
    document.getElementById("currentMode").textContent = newMode;
  }
  const manualOnBtn = document.getElementById("manualOnBtn");
  const manualOffBtn = document.getElementById("manualOffBtn");
  const setTimerBtn = document.getElementById("setTimerBtn");
  if (isManual) {
    // When switching to manual, enable buttons based on the current fan state
    if (manualOnBtn) manualOnBtn.disabled = currentFanState;
    if (manualOffBtn) manualOffBtn.disabled = !currentFanState;
    if (setTimerBtn) setTimerBtn.disabled = false;
  } else {
    // If not in manual mode, disable all manual buttons
    if (manualOnBtn) manualOnBtn.disabled = true;
    if (manualOffBtn) manualOffBtn.disabled = true;
    if (setTimerBtn) setTimerBtn.disabled = true;
  }
  if (sendCommand) {
    if (newMode === 'AUTO') {
  fetch("/fan?state=auto").then(res => { if(res.ok) updateSensorData(); }).catch(err => debugError("Failed to set auto mode:", err));
    } else if (newMode === 'MANUAL') {
      // Preserve the current fan state when switching to manual.
      const stateToSet = currentFanState ? 'on' : 'off';
  fetch("/fan?state=" + stateToSet).then(res => { if(res.ok) updateSensorData(); }).catch(err => debugError("Failed to set manual mode:", err));
    }
  }
}
function setFan(state) {
  if (testMode) {
    const fanIsOn = (state === 'on');
    currentFanState = fanIsOn; // Update the global state
    updateFanVisual(fanIsOn);
    // Update the button disabled states to reflect the new fan state
    document.getElementById('manualOnBtn').disabled = fanIsOn;
    document.getElementById('manualOffBtn').disabled = !fanIsOn;
    return;
  }
  // Send the command and then immediately trigger a status update for a responsive UI.
  fetch("/fan?state=" + state)
    .then(res => {
      if (res.ok) updateSensorData();
    })
    .catch(err => {
  debugError("Fan toggle failed:", err);
    });
}
async function loadConfig() {
  if (testMode) {
    return; // In local test mode, defaults are used from the HTML value attributes.
  }
  try {
    const response = await fetch("/config");
    const data = await response.json();
    document.getElementById("fanOnTemp").value = data.fanOnTemp;
    document.getElementById("fanDeltaTemp").value = data.fanDeltaTemp;
    document.getElementById("fanHysteresis").value = data.fanHysteresis;
    document.getElementById("preCoolTriggerTemp").value = data.preCoolTriggerTemp;
    document.getElementById("preCoolTempOffset").value = data.preCoolTempOffset;
    document.getElementById("preCoolingEnabled").checked = data.preCoolingEnabled;
    document.getElementById("onboardLedEnabled").checked = data.onboardLedEnabled;
    document.getElementById("testModeEnabled").checked = data.testModeEnabled;
    document.getElementById("dailyRestartEnabled").checked = data.dailyRestartEnabled;
    document.getElementById("mqttEnabled").checked = data.mqttEnabled;
    document.getElementById("mqttDiscoveryEnabled").checked = data.mqttDiscoveryEnabled;
    if (data.historyLogIntervalMs) {
      const intervalInput = document.getElementById("historyLogInterval");
      if(intervalInput) intervalInput.value = Math.round(data.historyLogIntervalMs / 60000);
    }
  } catch (err) {
  debugError("Failed to load config:", err);
  }
}
function saveConfig() {
  if (testMode) return;
  const fanOnTemp = document.getElementById("fanOnTemp").value;
  const fanDeltaTemp = document.getElementById("fanDeltaTemp").value;
  const fanHysteresis = document.getElementById("fanHysteresis").value;
  const preCoolTriggerTemp = document.getElementById("preCoolTriggerTemp").value;
  const preCoolTempOffset = document.getElementById("preCoolTempOffset").value;
  const preCoolingEnabled = document.getElementById("preCoolingEnabled").checked;
  const onboardLedEnabled = document.getElementById("onboardLedEnabled").checked;
  const testModeEnabled = document.getElementById("testModeEnabled").checked;
  const dailyRestartEnabled = document.getElementById("dailyRestartEnabled").checked;
  const mqttEnabled = document.getElementById("mqttEnabled").checked;
  const mqttDiscoveryEnabled = document.getElementById("mqttDiscoveryEnabled").checked;
  const logIntervalMin = parseInt(document.getElementById("historyLogInterval").value, 10);
  const configPayload = {
    fanOnTemp: parseFloat(fanOnTemp),
    fanDeltaTemp: parseFloat(fanDeltaTemp),
    fanHysteresis: parseFloat(fanHysteresis),
    preCoolTriggerTemp: parseFloat(preCoolTriggerTemp),
    preCoolTempOffset: parseFloat(preCoolTempOffset),
    preCoolingEnabled: preCoolingEnabled,
    onboardLedEnabled: onboardLedEnabled,
    dailyRestartEnabled: dailyRestartEnabled,
    testModeEnabled: testModeEnabled,
    mqttEnabled: mqttEnabled,
    mqttDiscoveryEnabled: mqttDiscoveryEnabled
  };
  if (!isNaN(logIntervalMin) && logIntervalMin > 0) {
    configPayload.historyLogIntervalMs = logIntervalMin * 60000;
  }
  fetch("/config", {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(configPayload) // Corrected: use the payload object
  })
  .then(res => Promise.all([res.ok, res.text()]))
  .then(([ok, text]) => {
    if (ok) {
      showToast("Configuration saved!");
      // Use the more informative message from the server
      if (text.includes("restart") && confirm(text + " Restart now?")) { restartDevice(); }
    } else {
      showToast("Save failed!");
    }
  })
  .catch(err => {
  debugError("Failed to save config:", err);
    showToast("Save failed!");
  });
}
function restartDevice() {
  if (testMode) {
    alert("Restart command sent (test mode).");
    return;
  }
  if (confirm("Are you sure you want to restart the device?")) {
    fetch("/restart")
      .then(() => {
        alert("Device is restarting. The page will become unresponsive.");
      })
  .catch(err => debugError("Failed to send restart command:", err));
  }
}
function resetConfig() {
  if (testMode) {
    alert("Config reset to defaults (test mode).");
    return;
  }
  if (confirm("Are you sure you want to reset all settings to their factory defaults? This action cannot be undone.")) {
    fetch("/reset_config")
      .then(() => {
        alert("Configuration has been reset. The device will now restart.");
        setTimeout(() => location.reload(), 5000); // Reload the page to show the new default settings
      })
  .catch(err => debugError("Failed to reset config:", err));
  }
}
function setSliderInteraction(isInteracting) {
  // When the user interacts with a slider, set a flag.
  // This prevents the periodic status update from overwriting the slider's position.
  // The flag is reset only when "Update Temps" is clicked successfully.
  isInteractingWithSlider = isInteracting;
}
function openIndoorSensorsModal(event) {
  if (event) event.preventDefault();
  const listEl = document.getElementById('indoorSensorsList');
  if (!listEl) return;

  if (allIndoorSensors.length > 0) {
    listEl.innerHTML = allIndoorSensors.map(sensor => `
      <div class="sensor-modal-item">
        <div class="sensor-modal-name">${sensor.name}</div>
        <div class="sensor-modal-data">
          <span>${sensor.temperature}°F</span> / <span>${sensor.humidity}%</span>
        </div>
      </div>
    `).join('');
  } else {
    listEl.innerHTML = '<p>No indoor sensor data available.</p>';
  }

  document.getElementById('indoorSensorsModal').style.display = 'block';
}
function closeIndoorSensorsModal() {
  document.getElementById('indoorSensorsModal').style.display = 'none';
}
// Make functions globally available for onclick handlers
window.openIndoorSensorsModal = openIndoorSensorsModal;
window.closeIndoorSensorsModal = closeIndoorSensorsModal;

function openTimerModal() {
  document.getElementById('timerModal').style.display = 'block';
}
function closeTimerModal() {
  document.getElementById('timerModal').style.display = 'none';
}
function startTimedRun() {
  const delay = document.getElementById('timerDelay').value;
  const duration = document.getElementById('timerDuration').value;
  const postAction = document.getElementById('postTimerAction').value;
  fetch('/fan', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      action: 'start_timed',
      delay: parseInt(delay, 10),
      duration: parseInt(duration, 10),
      postAction: postAction
    })
  })
  .then(res => {
    if (res.ok) {
      showToast("Timed run started!");
      closeTimerModal();
    } else {
      showToast("Failed to start timer.");
    }
  })
  .catch(err => debugError("Failed to start timed run:", err));
}
function toggleInfo(element) {
  const targetId = element.getAttribute('data-target');
  const targetElement = document.getElementById(targetId);
  if (targetElement) {
    targetElement.style.display = targetElement.style.display === 'block' ? 'none' : 'block';
  }
}
function updateTempDisplay(elementId, value) {
  document.getElementById(elementId).textContent = value;
}
function updateTestTemps() {
  const attic = document.getElementById('simulatedAtticTemp').value;
  const outdoor = document.getElementById('simulatedOutdoorTemp').value;
  fetch(`/test/set_temps?attic=${attic}&outdoor=${outdoor}`)
    .then(res => {
      const success = res.ok;
      showToast(success ? "Test temps updated." : "Update failed.");
      if (success) {
        // On success, reset the interaction flag so the UI will sync with the device again.
        setSliderInteraction(false);
      }
    })
  .catch(err => debugError("Failed to set test temps:", err));
}
// Add change event listeners to feature toggles for immediate save
document.addEventListener("DOMContentLoaded", function() {
  ["preCoolingEnabled", "onboardLedEnabled", "dailyRestartEnabled", "mqttEnabled", "mqttDiscoveryEnabled", "testModeEnabled"].forEach(id => {
    const el = document.getElementById(id);
    if (el) {
      el.addEventListener("change", saveConfig);
    }
  });
});
function forceAPMode() {
  if (testMode) {
    alert("AP Mode command sent (test mode)."); return;
  }
  if (confirm("This will force the device into AP mode and disconnect it from your WiFi. Are you sure?")) {
    fetch('/test/force_ap')
      .then(() => showToast("AP Mode command sent."))
  .catch(err => debugError("Failed to force AP mode:", err));
  }
}
function clearDiagnostics() {
  if (testMode) {
    alert("Clear diagnostics command sent (test mode)."); return;
  }
  if (confirm("Are you sure you want to permanently delete the diagnostics log? This action cannot be undone.")) {
    fetch("/clear_diagnostics")
      .then(res => res.text())
      .then(text => {
        showToast(text);
      })
      .catch(err => {
  debugError("Failed to clear diagnostics:", err);
        showToast("Error clearing diagnostics log.");
      });
  }
}
function clearHistory() {
  if (testMode) {
    alert("Clear history command sent (test mode)."); return;
  }
  if (confirm("Are you sure you want to permanently delete the history log? This action cannot be undone.")) {
    fetch("/clear_history")
      .then(res => res.text())
      .then(text => {
        showToast(text);
        fetchAndRenderHistory(); // Refresh the chart, which should now be empty
      })
      .catch(err => {
  debugError("Failed to clear history:", err);
        showToast("Error clearing history log.");
      });
  }
}
function showToast(message) {
  const toast = document.getElementById("saveToast");
  toast.textContent = message;
  toast.style.visibility = "visible";
  setTimeout(() => {
    toast.style.visibility = "hidden";
  }, 3000);
}
function updateFanVisual(isFanOn) {
  const fanStateEl = document.getElementById("fanState");
  fanStateEl.textContent = isFanOn ? "ON" : "OFF";
  if (isFanOn) {
    fanIconEl.style.animation = 'spin 1s linear infinite';
  } else {
    fanIconEl.style.animation = 'none';
  }
}
async function initializeApp() {
  // Set initial UI state for controls
  setMode('AUTO', false); // Set a default, will be corrected by first status update
  await loadConfig();
  updateSensorData();
  updateWeatherData();
  fetchAndRenderHistory();
  setInterval(fetchAndRenderHistory, 5 * 60 * 1000); // Refresh history every 5 min
  setInterval(updateSensorData, 3000); // Refresh status every 3 seconds for a responsive UI
  setInterval(updateWeatherData, 600000); // Update weather every 10 minutes
}
// Close modal if user clicks outside of it
window.onclick = function(event) {
  const modal = document.getElementById('timerModal'); // Existing timer modal
  const sensorsModal = document.getElementById('indoorSensorsModal');
  if (event.target === modal) {
    modal.style.display = "none";
  }
}
window.addEventListener("pageshow", function(event) {
  if (event.persisted) {
    loadConfig();
  }
});

// Only perform the ESP ping if not running on localhost, 127.0.0.1, or file://
const isLocalDev = location.hostname === "" || location.hostname === "localhost" || location.hostname === "127.0.0.1" || location.protocol === "file:";
if (!isLocalDev) {
  fetch("/fan?state=ping", { method: 'GET', signal: AbortSignal.timeout(2000) })
    .then(res => {
      if (!res.ok) throw new Error("Not live");
  debugLog("ESP detected, running in live mode.");
      // No special setup needed for live mode.
    })
    .catch(() => {
      console.info("ESP not detected, running in local test mode.");
      testMode = true;
    });
}

// Manually show the test panel for local viewing.
document.getElementById('test-panel').style.display = 'block';
const atticSlider = document.getElementById('simulatedAtticTemp');
const outdoorSlider = document.getElementById('simulatedOutdoorTemp');
atticSlider.value = 95;
outdoorSlider.value = 88;
// Populate the main display with initial test values
document.getElementById("atticTemp").textContent = atticSlider.value;
document.getElementById("atticHumidity").textContent = "55"; // A reasonable default
document.getElementById("outdoorTemp").textContent = outdoorSlider.value;
document.getElementById("currentMode").textContent = "AUTO"; // Default to AUTO mode
// Populate the test panel slider labels
updateTempDisplay('attic-temp-val', atticSlider.value);
updateTempDisplay('outdoor-temp-val', outdoorSlider.value);


// Helper script for modal animation
(function(){const o=window.openTimerModal,c=window.closeTimerModal;window.openTimerModal=function(){try{document.getElementById('timerModal').classList.add('show')}catch(e){} return o?o():void 0};window.closeTimerModal=function(){try{document.getElementById('timerModal').classList.remove('show')}catch(e){} return c?c():void 0}})();


// Helper script to initialize slider display values on load
document.addEventListener('DOMContentLoaded', function(){
  const att = document.getElementById('simulatedAtticTemp');
  const out = document.getElementById('simulatedOutdoorTemp');
  if (att) document.getElementById('attic-temp-val').textContent = att.value;
  if (out) document.getElementById('outdoor-temp-val').textContent = out.value;
});

/**
 * Updates sunrise and sunset display in the weather section header
 */
function updateSunriseSunset(sunrise, sunset) {
  // Extract time from ISO datetime if needed
  const sunriseTime = sunrise.includes('T') ? sunrise.split('T')[1].substring(0, 5) : sunrise;
  const sunsetTime = sunset.includes('T') ? sunset.split('T')[1].substring(0, 5) : sunset;
  
  // Find or create sunrise/sunset elements in the weather section
  let sunriseEl = document.getElementById('sunriseTime');
  let sunsetEl = document.getElementById('sunsetTime');
  
  if (!sunriseEl || !sunsetEl) {
    // Create the sunrise/sunset header if it doesn't exist
    const weatherSection = document.querySelector('section .content-section h2');
    if (weatherSection && weatherSection.textContent.includes('Weather Forecast')) {
      const headerRow = document.createElement('div');
      headerRow.className = 'weather-header-row';
      headerRow.innerHTML = `
        <span class="sunrise-sunset"><span id="sunriseTime">🌅 ${sunriseTime}</span></span>
        <span class="sunrise-sunset"><span id="sunsetTime">🌇 ${sunsetTime}</span></span>
      `;
      weatherSection.parentNode.insertBefore(headerRow, weatherSection.nextSibling);
    }
  } else {
    sunriseEl.textContent = `🌅 ${sunriseTime}`;
    sunsetEl.textContent = `🌇 ${sunsetTime}`;
  }
}

/**
 * Renders hourly forecast for the next 4-5 hours
 */
function renderHourlyForecast(hourlyData) {
  let hourlyContainer = document.getElementById('hourlyForecastContainer');
  
  if (!hourlyContainer) {
    // Create hourly forecast container
    const weatherSection = document.querySelector('#forecastContainer').parentNode;
    hourlyContainer = document.createElement('div');
    hourlyContainer.id = 'hourlyForecastContainer';
    hourlyContainer.className = 'hourly-forecast';
    
    const title = document.createElement('h4');
    title.textContent = 'Hourly Forecast';
    title.style.marginTop = '1rem';
    title.style.marginBottom = '0.5rem';
    
    weatherSection.insertBefore(title, weatherSection.querySelector('#forecastContainer'));
    weatherSection.insertBefore(hourlyContainer, weatherSection.querySelector('#forecastContainer'));
  }
  
  hourlyContainer.innerHTML = "";
  
  hourlyData.forEach(hour => {
    const hourDiv = document.createElement("div");
    hourDiv.className = "hourly-item";
    
    // Extract hour from time string (e.g., "14:00" from "2025-01-15T14:00")
    let displayTime = hour.time;
    if (hour.time.includes('T')) {
      displayTime = hour.time.split('T')[1].substring(0, 5);
    }
    
    const icon = hour.icon || weatherCodeToEmoji(hour.weatherCode);
    const temp = typeof hour.temperature === 'number' ? Math.round(hour.temperature) : hour.temperature;
    
    hourDiv.innerHTML = `
      <div class="hourly-time">${displayTime}</div>
      <div class="hourly-icon">${icon}</div>
      <div class="hourly-temp">${temp}°</div>
    `;
    
    hourlyContainer.appendChild(hourDiv);
  });
}

// Ensure main app logic runs after DOM is ready
document.addEventListener('DOMContentLoaded', initializeApp);