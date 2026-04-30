#include "command_handler.h"

CommandHandler::CommandHandler(StateMachine& sm, TrackQueue& queue,
                               Watchdog& watchdog, ItunesClient& itunes)
    : sm_(sm)
    , queue_(queue)
    , watchdog_(watchdog)
    , itunes_(itunes)
    , volume_(75)
    , start_time_(std::chrono::steady_clock::now())
    , buffering_(false) {
    Logger::instance().info("HTTP", "Command handler initialized");
}

void CommandHandler::start(int port) {
    register_routes();
    Logger::instance().info("HTTP", "Server listening on port " + std::to_string(port));
    server_.listen("0.0.0.0", port);
}

void CommandHandler::stop() {
    server_.stop();
    if (buffer_thread_.joinable()) buffer_thread_.join();
    Logger::instance().info("HTTP", "Server stopped");
}

void CommandHandler::set_cors(httplib::Response& res) {
    // Required so browser can call daemon from a different origin
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

json CommandHandler::track_to_json(const Track& track) {
    return {
        {"id",          track.id},
        {"title",       track.title},
        {"artist",      track.artist},
        {"album",       track.album},
        {"album_art",   track.album_art},
        {"preview_url", track.preview_url}
    };
}

std::string CommandHandler::state_to_color(PlayerState state) {
    switch (state) {
        case PlayerState::IDLE:      return "grey";
        case PlayerState::BUFFERING: return "yellow";
        case PlayerState::PLAYING:   return "green";
        case PlayerState::PAUSED:    return "blue";
        case PlayerState::STOPPED:   return "red";
        default:                     return "grey";
    }
}

long CommandHandler::uptime_seconds() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>
           (now - start_time_).count();
}

void CommandHandler::start_buffer_timer() {
    // Runs in background thread - simulates 800ms buffer fill
    // After delay, auto-transitions BUFFERING → PLAYING
    if (buffer_thread_.joinable()) buffer_thread_.join();

    buffering_ = true;
    buffer_thread_ = std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        if (buffering_ && sm_.current_state() == PlayerState::BUFFERING) {
            Logger::instance().info("HTTP", "Buffer filled - transitioning to PLAYING");
            sm_.transition(PlayerCommand::PLAY);
            buffering_ = false;
        }
    });
}

void CommandHandler::register_routes() {

    // ── CORS preflight ────────────────────────────────────────────────────────
    server_.Options(".*", [this](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        res.status = 204;
    });

    // ── GET /status ───────────────────────────────────────────────────────────
    // Frontend polls this every second to sync UI with daemon state
    server_.Get("/status", [this](const httplib::Request& req, httplib::Response& res) {
        handle_status(req, res);
    });

    // ── POST /command ─────────────────────────────────────────────────────────
    // Frontend sends { "action": "play" } etc
    server_.Post("/command", [this](const httplib::Request& req, httplib::Response& res) {
        handle_command(req, res);
    });

    // ── GET /search ───────────────────────────────────────────────────────────
    // Frontend calls /search?q=taylor+swift
    server_.Get("/search", [this](const httplib::Request& req, httplib::Response& res) {
        handle_search(req, res);
    });

    // ── POST /enqueue ─────────────────────────────────────────────────────────
    // Frontend sends track JSON + position ("start"|"end")
    server_.Post("/enqueue", [this](const httplib::Request& req, httplib::Response& res) {
        handle_enqueue(req, res);
    });

    // ── GET /health ───────────────────────────────────────────────────────────
    server_.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handle_health(req, res);
    });
}

void CommandHandler::handle_status(const httplib::Request&, httplib::Response& res) {
    set_cors(res);

    Track current = queue_.current_track();
    auto  next3   = queue_.peek_next(3);

    json next_tracks = json::array();
    for (auto& t : next3) {
        if (t.is_valid()) next_tracks.push_back(track_to_json(t));
    }

    json body = {
        {"state",       sm_.state_name()},
        {"state_color", state_to_color(sm_.current_state())},
        {"volume",      volume_.load()},
        {"queue_size",  queue_.size()},
        {"current_track", current.is_valid() ? track_to_json(current) : nullptr},
        {"up_next",     next_tracks}
    };

    res.set_content(body.dump(), "application/json");
}

void CommandHandler::handle_command(const httplib::Request& req, httplib::Response& res) {
    set_cors(res);

    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
        return;
    }

    std::string action = body.value("action", "");
    Logger::instance().info("HTTP", "Received command: " + action);

    // ── Fault injection commands ───────────────────────────────────────────
    if (action == "simulate_hang") {
        watchdog_.simulate_hang();
        res.set_content(json{{"ok", true}, {"message", "Hang simulated"}}.dump(),
                        "application/json");
        return;
    }

    if (action == "simulate_api_failure") {
        bool enable = body.value("enabled", true);
        itunes_.simulate_failure(enable);
        res.set_content(json{{"ok", true},
            {"message", "API failure simulation " + std::string(enable ? "on" : "off")}
        }.dump(), "application/json");
        return;
    }

    if (action == "simulate_invalid_transition") {
        // Try a definitely invalid transition
        bool result = sm_.transition(PlayerCommand::PAUSE);
        res.set_content(json{{"ok", false},
            {"message", "Invalid transition attempted, rejected: " +
             std::string(result ? "false" : "true")}
        }.dump(), "application/json");
        return;
    }

    // ── Volume ────────────────────────────────────────────────────────────
    if (action == "volume") {
        int vol = body.value("value", 75);
        vol = std::max(0, std::min(100, vol));
        volume_ = vol;
        Logger::instance().info("HTTP", "Volume set to " + std::to_string(vol));
        res.set_content(json{{"ok", true}, {"volume", vol}}.dump(), "application/json");
        return;
    }

    // ── Playback commands ─────────────────────────────────────────────────
    bool ok = false;

    if (action == "play") {
        ok = sm_.transition(PlayerCommand::PLAY);
        if (ok) start_buffer_timer();

    } else if (action == "pause") {
        ok = sm_.transition(PlayerCommand::PAUSE);

    } else if (action == "resume") {
        ok = sm_.transition(PlayerCommand::RESUME);

    } else if (action == "stop") {
        buffering_ = false;
        ok = sm_.transition(PlayerCommand::STOP);
        if (ok && sm_.current_state() == PlayerState::STOPPED) {
            sm_.transition(PlayerCommand::PLAY);  // STOPPED → IDLE
        }

    } else if (action == "skip") {
        buffering_ = false;
        ok = sm_.transition(PlayerCommand::SKIP);
        if (ok) {
            Track finished = queue_.current_track();
            queue_.advance();
            // Keep local catalog circular: re-enqueue finished track if below 10
            if (queue_.size() < 10) queue_.enqueue(finished);
            start_buffer_timer();
        }
    } else {
        res.status = 400;
        res.set_content(json{{"error", "Unknown action: " + action}}.dump(),
                        "application/json");
        return;
    }

    json response = {
        {"ok",    ok},
        {"state", sm_.state_name()}
    };

    if (!ok) {
        response["error"] = "Invalid transition from " + sm_.state_name();
    }

    res.set_content(response.dump(), "application/json");
}

void CommandHandler::handle_search(const httplib::Request& req, httplib::Response& res) {
    set_cors(res);

    std::string query = req.get_param_value("q");
    if (query.empty()) {
        res.status = 400;
        res.set_content(json{{"error", "Missing query param ?q="}}.dump(), "application/json");
        return;
    }

    auto tracks = itunes_.search(query, 8);

    json results = json::array();
    for (auto& t : tracks) results.push_back(track_to_json(t));

    res.set_content(json{{"results", results}}.dump(), "application/json");
}

void CommandHandler::handle_enqueue(const httplib::Request& req, httplib::Response& res) {
    set_cors(res);

    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
        return;
    }

    Track track;
    track.id          = body.value("id", "");
    track.title       = body.value("title", "Unknown Title");
    track.artist      = body.value("artist", "Unknown Artist");
    track.album       = body.value("album", "Unknown Album");
    track.album_art   = body.value("album_art", "");
    track.preview_url = body.value("preview_url", "");

    if (!track.is_valid()) {
        res.status = 400;
        res.set_content(json{{"error", "Track must have a non-empty id"}}.dump(), "application/json");
        return;
    }

    std::string position = body.value("position", "end");
    bool ok = (position == "start") ? queue_.enqueue_front(track) : queue_.enqueue(track);

    Logger::instance().info("HTTP", "Enqueue [" + position + "]: " + track.title);
    res.set_content(json{{"ok", ok}, {"queue_size", queue_.size()}}.dump(), "application/json");
}

void CommandHandler::handle_health(const httplib::Request&, httplib::Response& res) {
    set_cors(res);

    json body = {
        {"status",        "ok"},
        {"uptime_seconds", uptime_seconds()},
        {"state",         sm_.state_name()},
        {"queue_size",    queue_.size()},
        {"memory_used",   MemoryBudget::instance().used()},
        {"memory_cap",    MemoryBudget::MAX_BYTES},
        {"watchdog",      "running"}
    };

    res.set_content(body.dump(), "application/json");
}