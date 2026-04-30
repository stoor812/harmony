#pragma once

#include "state_machine.h"
#include "queue.h"
#include "watchdog.h"
#include "memory_budget.h"
#include "itunes_client.h"
#include "logger.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

using json = nlohmann::json;

class CommandHandler {
public:
    CommandHandler(StateMachine& sm, TrackQueue& queue,
                   Watchdog& watchdog, ItunesClient& itunes);

    // Start HTTP server (blocks - run in a thread)
    void start(int port = 8080);
    void stop();

private:
    StateMachine&  sm_;
    TrackQueue&    queue_;
    Watchdog&      watchdog_;
    ItunesClient&  itunes_;
    httplib::Server server_;

    std::atomic<int>  volume_;       // 0-100
    std::chrono::steady_clock::time_point start_time_;

    void register_routes();

    // Route handlers
    void handle_status(const httplib::Request&, httplib::Response&);
    void handle_command(const httplib::Request&, httplib::Response&);
    void handle_search(const httplib::Request&, httplib::Response&);
    void handle_enqueue(const httplib::Request&, httplib::Response&);
    void handle_health(const httplib::Request&, httplib::Response&);

    // Helpers
    void set_cors(httplib::Response& res);
    json track_to_json(const Track& track);
    std::string state_to_color(PlayerState state);
    long uptime_seconds();

    // Buffering simulation - 800ms delay then auto-transition to PLAYING
    void start_buffer_timer();
    std::thread buffer_thread_;
    std::atomic<bool> buffering_;
};