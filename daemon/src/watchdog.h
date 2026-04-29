#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <functional>

class Watchdog {
public:
    Watchdog();
    ~Watchdog();

    // Start both threads
    void start(std::function<void()> on_trigger);
    void stop();

    // Main thread calls this regularly to prove it's alive
    void heartbeat();

    // For fault injection demo: stop feeding the watchdog
    void simulate_hang();

    bool is_running() const { return running_; }

private:
    std::atomic<bool> running_;
    std::atomic<bool> simulating_hang_;

    // Two threads: one feeds heartbeat, one checks it
    std::thread heartbeat_thread_;
    std::thread watchdog_thread_;

    std::chrono::steady_clock::time_point last_heartbeat_;
    std::function<void()> on_trigger_;

    void heartbeat_loop();   // Updates timestamp every 100ms
    void watchdog_loop();    // Checks timestamp every 250ms
};