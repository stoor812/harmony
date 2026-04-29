#include "watchdog.h"
#include "logger.h"

#include <chrono>

using namespace std::chrono_literals;

Watchdog::Watchdog() 
    : running_(false)
    , simulating_hang_(false)
    , last_heartbeat_(std::chrono::steady_clock::now()) {
    Logger::instance().info("WATCHDOG", "Watchdog initialized");
}

Watchdog::~Watchdog() {
    stop();
}

void Watchdog::start(std::function<void()> on_trigger) {
    on_trigger_ = on_trigger;
    running_ = true;
    last_heartbeat_ = std::chrono::steady_clock::now();

    heartbeat_thread_ = std::thread(&Watchdog::heartbeat_loop, this);
    watchdog_thread_  = std::thread(&Watchdog::watchdog_loop, this);

    Logger::instance().info("WATCHDOG", "Watchdog started - heartbeat every 100ms, check every 250ms");
}

void Watchdog::stop() {
    if (running_) {
        running_ = false;
        if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
        if (watchdog_thread_.joinable())  watchdog_thread_.join();
        Logger::instance().info("WATCHDOG", "Watchdog stopped");
    }
}

void Watchdog::heartbeat() {
    last_heartbeat_ = std::chrono::steady_clock::now();
}

void Watchdog::simulate_hang() {
    simulating_hang_ = true;
    Logger::instance().warn("WATCHDOG", "FAULT INJECTION: Simulating hang - heartbeat suspended");
}

void Watchdog::heartbeat_loop() {
    // Updates the heartbeat timestamp every 100ms
    // If simulate_hang is active, stops updating so watchdog triggers
    while (running_) {
        if (!simulating_hang_) {
            heartbeat();
        }
        std::this_thread::sleep_for(100ms);
    }
}

void Watchdog::watchdog_loop() {
    // Checks every 250ms whether heartbeat is fresh
    // If stale for 500ms - system is considered hung, trigger recovery
    while (running_) {
        std::this_thread::sleep_for(250ms);

        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                       (now - last_heartbeat_).count();

        if (elapsed > 500) {
            Logger::instance().warn("WATCHDOG",
                "Heartbeat stale for " + std::to_string(elapsed) +
                "ms - triggering recovery");

            if (on_trigger_) {
                on_trigger_();
            }

            // Reset heartbeat so we don't fire repeatedly
            last_heartbeat_ = std::chrono::steady_clock::now();
            simulating_hang_ = false;
        }
    }
}