# Harmony

A C++ music-player daemon with a vanilla JS frontend. The daemon runs an HTTP server that manages a finite-state-machine player, streams 30-second iTunes previews, and exposes a REST API. The browser UI polls the daemon every second and renders live state.

![State: PLAYING](https://img.shields.io/badge/state-PLAYING-16a34a?style=flat-square)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)
![License: MIT](https://img.shields.io/badge/license-MIT-grey?style=flat-square)

---

## Architecture

```
Browser (port 5500)
  │  polls /status every 1s
  │  POST /command  (play, pause, resume, stop, skip, volume)
  │  GET  /search?q=…
  │  POST /enqueue
  ▼
C++ Daemon (port 8080)
  ├── StateMachine    — FSM enforcing valid state transitions
  ├── TrackQueue      — circular local catalog + enqueue/dequeue
  ├── ItunesClient    — iTunes Search API (30s previews)
  ├── Watchdog        — heartbeat thread, resets to IDLE on hang
  ├── MemoryBudget    — static allocation accounting
  └── CommandHandler  — cpp-httplib HTTP server, route dispatch
```

### Player FSM

```
IDLE ──PLAY──▶ BUFFERING ──PLAY──▶ PLAYING ◀──RESUME── PAUSED
  ▲                │                  │                    │
  │              STOP               PAUSE               STOP/SKIP
  │                ▼                  │                    │
  └──PLAY──── STOPPED ◀──────STOP─────┘◀────────────────────┘
                                    SKIP──▶ BUFFERING
```

State transitions not listed above are rejected. The watchdog can force any state to `IDLE` on a detected hang.

---

## Quick Start

**Requirements:** Linux or macOS (or WSL2 on Windows). Git Bash / MSYS2 are not supported.

```bash
git clone https://github.com/shaan812/harmony.git
cd harmony

# Install deps, build daemon, run unit tests (~2 min first run)
./scripts/setup.sh

# Start daemon + frontend server + open browser
./scripts/run.sh
```

Open `http://localhost:5500` — the UI connects to the daemon at `http://localhost:8080`.

To stop everything:

```bash
./scripts/stop.sh
```

---

## Scripts

| Script | What it does |
|--------|-------------|
| `scripts/setup.sh` | Installs system packages, builds cpr from source if needed, compiles the daemon, runs the test suite |
| `scripts/run.sh` | Starts the daemon (port 8080) and a Python static file server (port 5500), opens the browser |
| `scripts/stop.sh` | Sends SIGTERM to both processes, waits up to 5s, then SIGKILL fallback |
| `scripts/install-service.sh` | Installs daemon as a systemd service under a dedicated `harmony` user (Linux only) |

---

## REST API

All responses are `application/json`. CORS is open (`*`).

### `GET /status`
Returns current player state, now-playing track, up-next queue, and volume.

```json
{
  "state": "PLAYING",
  "state_color": "green",
  "volume": 75,
  "queue_size": 8,
  "current_track": { "id": "…", "title": "…", "artist": "…", "album": "…", "album_art": "…", "preview_url": "…" },
  "up_next": [ { … }, { … }, { … } ]
}
```

### `POST /command`
Body: `{ "action": "<action>", ...extras }`

| Action | Effect |
|--------|--------|
| `play` | IDLE/STOPPED → BUFFERING (800 ms) → PLAYING |
| `pause` | PLAYING → PAUSED |
| `resume` | PAUSED → PLAYING |
| `stop` | any → STOPPED → IDLE |
| `skip` | advance queue, restart buffer timer |
| `volume` | set volume 0–100, body: `{ "value": 80 }` |
| `simulate_hang` | trigger watchdog recovery |
| `simulate_api_failure` | toggle iTunes API failure, body: `{ "enabled": true }` |

### `GET /search?q=<query>`
Calls iTunes Search API, returns up to 8 tracks with 30s preview URLs.

### `POST /enqueue`
Body: track object + `"position": "start" | "end"`. Adds track to front or back of queue.

### `GET /health`
Returns uptime, watchdog status, queue size, memory usage.

---

## Project Structure

```
harmony/
├── daemon/
│   ├── src/
│   │   ├── main.cpp                — startup, signal handling, component wiring
│   │   ├── state_machine.{h,cpp}   — FSM: states, commands, transition table
│   │   ├── queue.{h,cpp}           — TrackQueue (circular, thread-safe)
│   │   ├── itunes_client.{h,cpp}   — iTunes Search API wrapper
│   │   ├── watchdog.{h,cpp}        — heartbeat + hang detection
│   │   ├── memory_budget.{h,cpp}   — static memory accounting singleton
│   │   ├── logger.{h,cpp}          — thread-safe logger singleton
│   │   └── command_handler.{h,cpp} — HTTP routes, buffer timer
│   ├── tests/
│   │   └── test_state_machine.cpp  — 67 doctest cases, 138 assertions
│   ├── data/
│   │   └── tracks.json             — local fallback catalog
│   ├── deploy/
│   │   └── harmony.service         — systemd unit
│   └── CMakeLists.txt
├── frontend/
│   ├── index.html
│   ├── app.js                      — polling, command dispatch, safe DOM
│   └── style.css                   — glassmorphism, CSS custom properties
└── scripts/
    ├── setup.sh
    ├── run.sh
    ├── stop.sh
    └── install-service.sh
```

---

## Running Tests

```bash
cd daemon/build
ctest --output-on-failure
# or directly:
./test_harmony
```

67 test cases across 8 suites: initialization, valid transitions, invalid transitions (per state), state enforcement, transition sequences, callback behavior, force-state, and state name strings.

---

## Production Deployment (Linux + systemd)

```bash
./scripts/setup.sh                  # build release binary
sudo ./scripts/install-service.sh   # install to /opt/harmony, create harmony user, register unit
sudo journalctl -u harmony -f       # follow logs
```

The service restarts automatically on failure (up to 5 times per 60s window) and runs with `NoNewPrivileges` and `PrivateTmp` hardening.

---

## Dependencies

| Library | Version | Use |
|---------|---------|-----|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | 0.14.1 | embedded HTTP server |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON parsing |
| [cpr](https://github.com/libcpr/cpr) | latest | HTTP client for iTunes API |
| [doctest](https://github.com/doctest/doctest) | 2.4.11 | unit testing |

All fetched automatically by CMake FetchContent except cpr (built by `setup.sh`).
