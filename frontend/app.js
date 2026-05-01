const DAEMON = 'http://localhost:8080';

// ── Toast notifications ────────────────────────────────────────────────────
function showToast(title, detail = '', type = 'info', duration = 2500) {
  const icons = { success: '✓', error: '✕', info: '♪' };
  const container = document.getElementById('toast-container');

  const toast = document.createElement('div');
  toast.className = 'toast';

  const iconSpan = document.createElement('span');
  iconSpan.className = `toast-icon ${type}`;
  iconSpan.textContent = icons[type] || '';

  const body = document.createElement('div');
  body.className = 'toast-body';

  const titleDiv = document.createElement('div');
  titleDiv.className = 'toast-title';
  titleDiv.textContent = title;
  body.appendChild(titleDiv);

  if (detail) {
    const detailDiv = document.createElement('div');
    detailDiv.className = 'toast-detail';
    detailDiv.textContent = detail;
    body.appendChild(detailDiv);
  }

  toast.appendChild(iconSpan);
  toast.appendChild(body);
  container.appendChild(toast);

  const dismiss = () => {
    toast.classList.add('toast-out');
    toast.addEventListener('animationend', () => toast.remove(), { once: true });
  };

  setTimeout(dismiss, duration);
  toast.addEventListener('click', dismiss);
}

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
      const art = document.getElementById('album-art');
      art.src = t.album_art || 'https://via.placeholder.com/210x210/1a1a2e/ffffff?text=%E2%99%AA';
      art.alt = `${t.title} by ${t.artist}`;

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

  // Play/pause SVG icon swap
  const isPlaying = currentState === 'PLAYING';
  document.getElementById('icon-play').style.display  = isPlaying ? 'none' : '';
  document.getElementById('icon-pause').style.display = isPlaying ? ''     : 'none';
  document.getElementById('btn-play').setAttribute('aria-label', isPlaying ? 'Pause' : 'Play');

  // Album ring spins while playing
  document.getElementById('album-ring').classList.toggle('playing', isPlaying);

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
  container.replaceChildren();

  if (!tracks.length) {
    const empty = document.createElement('div');
    empty.className = 'up-next-empty';
    empty.textContent = 'Queue empty';
    container.appendChild(empty);
    return;
  }

  for (const t of tracks) {
    const item = document.createElement('div');
    item.className = 'up-next-item';

    const img = document.createElement('img');
    img.className = 'up-next-art';
    img.alt = '';
    img.src = t.album_art || '';
    img.addEventListener('error', () => { img.src = 'https://via.placeholder.com/36'; });

    const info = document.createElement('div');
    info.className = 'up-next-info';

    const title = document.createElement('div');
    title.className = 'up-next-title';
    title.textContent = t.title;

    const artist = document.createElement('div');
    artist.className = 'up-next-artist';
    artist.textContent = t.artist;

    info.appendChild(title);
    info.appendChild(artist);
    item.appendChild(img);
    item.appendChild(info);
    container.appendChild(item);
  }
}

// ── Health panel ───────────────────────────────────────────────────────────
function updateHealth(data) {
  const pct = Math.round((data.memory_used / data.memory_cap) * 100);
  document.getElementById('health-info').innerHTML = `
    <span class="health-key">uptime</span>   <span class="health-val">${data.uptime_seconds}s</span>
    <span class="health-key">watchdog</span> <span class="health-val">${data.watchdog}</span>
    <span class="health-key">queue</span>    <span class="health-val">${data.queue_size} tracks</span>
    <span class="health-key">memory</span>  <span class="health-val">${pct}% of 256 KB</span>
    <div class="health-bar-wrap"><div class="health-bar-fill" style="width:${pct}%"></div></div>
  `;
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
    showToast('API failure ON', 'iTunes searches will return empty', 'error');
  } else {
    btn.classList.remove('active');
    msg.classList.remove('visible');
    showToast('API failure OFF', 'iTunes API calls resumed', 'success');
  }
  await sendCommand('simulate_api_failure', { enabled: apiFailureOn });
}


// ── Search ─────────────────────────────────────────────────────────────────
async function doSearch() {
  const query = document.getElementById('search-input').value.trim();
  if (!query) return;

  const container = document.getElementById('search-results');
  const setStatus = (text, color = '#888') => {
    container.replaceChildren();
    const div = document.createElement('div');
    div.style.cssText = `color:${color};font-size:13px`;
    div.textContent = text;
    container.appendChild(div);
  };

  setStatus('Searching...');

  try {
    const res   = await fetch(`${DAEMON}/search?q=${encodeURIComponent(query)}`);
    const data  = await res.json();
    const items = data.results || [];

    if (!items.length) {
      setStatus('No results');
      return;
    }

    container.replaceChildren();

    for (const t of items) {
      const item = document.createElement('div');
      item.className = 'search-item';

      const img = document.createElement('img');
      img.className = 'search-art';
      img.alt = '';
      img.src = t.album_art || '';
      img.addEventListener('error', () => { img.src = 'https://via.placeholder.com/36'; });

      const info = document.createElement('div');
      info.className = 'search-info';

      const title = document.createElement('div');
      title.className = 'search-title';
      title.textContent = t.title;

      const artist = document.createElement('div');
      artist.className = 'search-artist';
      artist.textContent = t.artist;

      info.appendChild(title);
      info.appendChild(artist);

      const actions = document.createElement('div');
      actions.className = 'search-actions';

      const btnNext = document.createElement('button');
      btnNext.className = 'search-queue-btn';
      btnNext.textContent = '▶ Next';
      btnNext.addEventListener('click', () => enqueueTrack(t, 'start'));

      const btnEnd = document.createElement('button');
      btnEnd.className = 'search-queue-btn';
      btnEnd.textContent = '+ End';
      btnEnd.addEventListener('click', () => enqueueTrack(t, 'end'));

      actions.appendChild(btnNext);
      actions.appendChild(btnEnd);

      item.appendChild(img);
      item.appendChild(info);
      item.appendChild(actions);
      container.appendChild(item);
    }
  } catch (e) {
    setStatus('Search failed - is daemon running?', '#e74c3c');
  }
}

async function enqueueTrack(track, position) {
  try {
    await fetch(`${DAEMON}/enqueue`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ ...track, position })
    });
    const label = position === 'start' ? 'Playing next' : 'Added to queue';
    showToast(label, `${track.title} — ${track.artist}`, 'success');
    if (currentState === 'IDLE' || currentState === 'STOPPED') await sendCommand('play');
  } catch (e) {
    showToast('Enqueue failed', 'Check that the daemon is running', 'error');
    console.error('Enqueue failed:', e);
  }
}