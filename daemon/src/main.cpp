#include "logger.h"
#include "state_machine.h"
#include "queue.h"
#include "watchdog.h"
#include "memory_budget.h"
#include "itunes_client.h"
#include "command_handler.h"

#include <csignal>
#include <atomic>
#include <thread>

// Global flag so signal handler can trigger clean shutdown
std::atomic<bool> g_running{true};
httplib::Server*  g_server_ptr = nullptr;

void signal_handler(int signal) {
    Logger::instance().info("MAIN",
        "Signal " + std::to_string(signal) + " received - shutting down");
    g_running = false;
    if (g_server_ptr) g_server_ptr->stop();
}

int main() {
    // ── Signal handling ───────────────────────────────────────────────────
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    Logger::instance().info("MAIN", "Harmony daemon starting up");

    // ── Memory budget ─────────────────────────────────────────────────────
    MemoryBudget::instance().allocate("state_machine", 256);
    MemoryBudget::instance().allocate("track_queue",   sizeof(Track) * 10);
    MemoryBudget::instance().allocate("watchdog",      512);
    MemoryBudget::instance().allocate("http_server",   8192);
    MemoryBudget::instance().allocate("logger",        4096);
    MemoryBudget::instance().log_status();

    // ── Core components ───────────────────────────────────────────────────
    StateMachine  sm;
    TrackQueue    queue;
    ItunesClient  itunes;
    Watchdog      watchdog;

    // ── Load local catalog ────────────────────────────────────────────────
    if (!queue.load_from_file("data/tracks.json")) {
        Logger::instance().warn("MAIN", "Could not load tracks.json - queue empty");
    }

    // ── Watchdog: reset to IDLE on hang ───────────────────────────────────
    watchdog.start([&sm]() {
        Logger::instance().warn("MAIN", "Watchdog triggered - resetting to IDLE");
        sm.force_state(PlayerState::IDLE);
    });

    // ── HTTP server ───────────────────────────────────────────────────────
    CommandHandler handler(sm, queue, watchdog, itunes);

    // Wire signal handler to the live server so SIGINT/SIGTERM unblock listen()
    g_server_ptr = &handler.server();

    Logger::instance().info("MAIN", "Starting HTTP server on port 8080");
    Logger::instance().info("MAIN", "Endpoints: GET /status  POST /command  GET /search  GET /health");

    // Run server (blocks until stopped via signal handler)
    handler.start(8080);

    // ── Clean shutdown ────────────────────────────────────────────────────
    handler.stop();   // joins buffer thread; idempotent (destructor calls it too)
    watchdog.stop();
    Logger::instance().info("MAIN", "Harmony daemon stopped cleanly");
    return 0;
}