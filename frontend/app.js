const DAEMON = 'http://localhost:8080';

let currentState   = 'IDLE';
let currentTrack   = null;
let progressTimer  = null;
let progressSecs   = 0;
let apiFailureOn   = false;

// ── Poll daemon every second ───────────────────────────────────────────────
async function poll() {
  try {
    const res  = await fetch(`${DAEMON}/status`);
    const data = await res.json();
    updateUI(data);
  } catch (e) {
    document.getElementById('state-label').textContent = 'OFFLINE';
  }

  try {
    const res  = await fetch(`${DAEMON}/health`);
    const data = await res.json();
    updateHealth(data);
  } catch (_) {}
}

setInterval(poll, 1000);
poll();

// ── Update entire UI from /status response ─────────────────────────────────
function updateUI(data) {
  const prevState = currentState;
  currentState    = data.state;

  // State dot + label
  const dot   = document.getElementById('state-dot');
  const label = document.getElementById('state-label');
  dot.className   = `state-dot ${data.state_color}`;
  label.textContent = data.state;

  // FSM visualizer - highlight current state node
  ['IDLE','BUFFERING','PLAYING','PAUSED','STOPPED'].forEach(s => {
    const node = document.getElementById(`fsm-${s}`);
    if (node) node.classList.toggle('active', s === data.state);
  });

  // Transition log
  if (prevState !== currentState) {
    document.getElementById('last-transition').textContent =
      `${prevState} → ${currentState}`;
  }

  // Track info
  if (data.current_track) {
    const t = data.current_track;
    if (!currentTrack || currentTrack.id !== t.id) {
      currentTrack = t;
      document.getElementById('track-title').textContent  = t.title;
      document.getElementById('track-artist').textContent = t.artist;
      document.getElementById('track-album').textContent  = t.album;
      document.getElementById('album-art').src = t.album_art ||
        'https://via.placeholder.com/220x220/1a1a2e/ffffff?text=♪';

      // Load audio preview
      const audio = document.getElementById('audio-player');
      audio.src   = t.preview_url;

      // Reset progress for the new track
      clearInterval(progressTimer);
      progressTimer = null;
      progressSecs  = 0;
      updateProgressBar(0);
      document.getElementById('time-current').textContent = '0:00';
      if (data.state === 'PLAYING') startProgressTimer();
    }
  }

  // Play button icon
  const playBtn = document.getElementById('btn-play');
  playBtn.textContent = (currentState === 'PLAYING') ? '⏸' : '▶';

  // Audio sync with state
  syncAudio(data.state);

  // Progress bar
  handleProgress(data.state, prevState);

  // Up Next queue
  updateUpNext(data.up_next || []);

  // Volume sync (only if not currently dragging)
  const slider = document.getElementById('volume-slider');
  if (Math.abs(slider.value - data.volume) > 5) {
    slider.value = data.volume;
    document.getElementById('vol-label').textContent = data.volume;
  }
}

// ── Audio sync ─────────────────────────────────────────────────────────────
function syncAudio(state) {
  const audio = document.getElementById('audio-player');
  if (state === 'PLAYING' && audio.paused && audio.src) {
    audio.play().catch(() => {});  // catch autoplay policy errors
  } else if (state !== 'PLAYING' && !audio.paused) {
    audio.pause();
  }
}

// ── Progress bar (fake 30s timer) ──────────────────────────────────────────
function handleProgress(state, prevState) {
  if (state === 'PLAYING') {
    // Reset progress when coming from a fresh start (not resuming from pause)
    if (prevState === 'BUFFERING' || prevState === 'IDLE') {
      progressSecs = 0;
    }
    // Always ensure a timer is running while playing — covers the case where
    // BUFFERING resolves within one poll interval and prevState === 'PLAYING'
    if (!progressTimer) startProgressTimer();
  } else if (state === 'PAUSED' && progressTimer) {
    clearInterval(progressTimer);
    progressTimer = null;
  } else if (state === 'IDLE' || state === 'STOPPED') {
    clearInterval(progressTimer);
    progressTimer = null;
    progressSecs  = 0;
    updateProgressBar(0);
    document.getElementById('time-current').textContent = '0:00';
  }
}

function startProgressTimer() {
  if (progressTimer) clearInterval(progressTimer);
  progressTimer = setInterval(() => {
    progressSecs++;
    updateProgressBar(progressSecs / 30);
    document.getElementById('time-current').textContent = formatTime(progressSecs);
    if (progressSecs >= 30) {
      clearInterval(progressTimer);
      progressTimer = null;
      progressSecs  = 0;
      updateProgressBar(0);
      document.getElementById('time-current').textContent = '0:00';
      sendCommand('skip');  // preview ended — advance to next track
    }
  }, 1000);
}

function updateProgressBar(fraction) {
  document.getElementById('progress-fill').style.width = `${fraction * 100}%`;
}

function formatTime(secs) {
  return `${Math.floor(secs / 60)}:${String(secs % 60).padStart(2, '0')}`;
}

// ── Up Next ────────────────────────────────────────────────────────────────
function updateUpNext(tracks) {
  const container = document.getElementById('up-next-list');
  if (!tracks.length) {
    container.innerHTML = '<div class="up-next-empty">Queue empty</div>';
    return;
  }
  container.innerHTML = tracks.map(t => `
    <div class="up-next-item">
      <img class="up-next-art" src="${t.album_art}" alt="" onerror="this.src='https://via.placeholder.com/36'"/>
      <div class="up-next-info">
        <div class="up-next-title">${t.title}</div>
        <div class="up-next-artist">${t.artist}</div>
      </div>
    </div>
  `).join('');
}

// ── Health panel ───────────────────────────────────────────────────────────
function updateHealth(data) {
  const pct = Math.round((data.memory_used / data.memory_cap) * 100);
  document.getElementById('health-info').innerHTML =
    `uptime: ${data.uptime_seconds}s &nbsp;|&nbsp; ` +
    `memory: ${pct}% of 256KB &nbsp;|&nbsp; ` +
    `watchdog: ${data.watchdog} &nbsp;|&nbsp; ` +
    `queue: ${data.queue_size} tracks`;
}

// ── Commands ───────────────────────────────────────────────────────────────
async function sendCommand(action, extra = {}) {
  try {
    const res = await fetch(`${DAEMON}/command`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ action, ...extra })
    });
    return await res.json();
  } catch (e) {
    console.error('Command failed:', e);
  }
}

function handlePlayPause() {
  if (currentState === 'PLAYING') {
    sendCommand('pause');
  } else if (currentState === 'PAUSED') {
    sendCommand('resume');
  } else {
    sendCommand('play');
  }
}

function setVolume(val) {
  document.getElementById('vol-label').textContent = val;
  document.getElementById('audio-player').volume = val / 100;
  sendCommand('volume', { value: parseInt(val) });
}

async function simulateApiFailure() {
  apiFailureOn = !apiFailureOn;
  const btn = document.getElementById('btn-api-failure');
  const msg = document.getElementById('msg-api-failure');
  if (apiFailureOn) {
    btn.classList.add('active');
    msg.textContent = 'API failure simulation toggled. When ON: iTunes searches return empty results. Daemon gracefully degrades to local catalog. When OFF: iTunes API calls resume normally.';
    msg.classList.add('visible');
  } else {
    btn.classList.remove('active');
    msg.classList.remove('visible');
  }
  await sendCommand('simulate_api_failure', { enabled: apiFailureOn });
}


// ── Search ─────────────────────────────────────────────────────────────────
async function doSearch() {
  const query = document.getElementById('search-input').value.trim();
  if (!query) return;

  const container = document.getElementById('search-results');
  container.innerHTML = '<div style="color:#888;font-size:13px">Searching...</div>';

  try {
    const res   = await fetch(`${DAEMON}/search?q=${encodeURIComponent(query)}`);
    const data  = await res.json();
    const items = data.results || [];

    if (!items.length) {
      container.innerHTML = '<div style="color:#888;font-size:13px">No results</div>';
      return;
    }

    container.innerHTML = items.map(t => {
      const td = JSON.stringify(t).replace(/"/g, '&quot;');
      return `
        <div class="search-item">
          <img class="search-art" src="${t.album_art}" alt="" onerror="this.src='https://via.placeholder.com/36'"/>
          <div class="search-info">
            <div class="search-title">${t.title}</div>
            <div class="search-artist">${t.artist}</div>
          </div>
          <div class="search-actions">
            <button class="search-queue-btn" onclick="enqueueTrack(${td}, 'start')">▶ Next</button>
            <button class="search-queue-btn" onclick="enqueueTrack(${td}, 'end')">+ End</button>
          </div>
        </div>
      `;
    }).join('');
  } catch (e) {
    container.innerHTML = '<div style="color:#e74c3c;font-size:13px">Search failed - is daemon running?</div>';
  }
}

async function enqueueTrack(track, position) {
  try {
    await fetch(`${DAEMON}/enqueue`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ ...track, position })
    });
    if (currentState === 'IDLE' || currentState === 'STOPPED') await sendCommand('play');
  } catch (e) {
    console.error('Enqueue failed:', e);
  }
}