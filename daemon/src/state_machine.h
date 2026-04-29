#pragma once

#include <string>
#include <functional>
#include <mutex>

// Every possible state the player can be in
// This mirrors how real firmware defines device states
enum class PlayerState {
    IDLE,       // No track loaded, doing nothing
    BUFFERING,  // Track selected, filling audio buffer (800ms artificial delay)
    PLAYING,    // Audio actively playing
    PAUSED,     // Playback frozen, track still loaded
    STOPPED     // Playback ended, clearing track before returning to IDLE
};

// Every command the frontend can send
enum class PlayerCommand {
    PLAY,
    PAUSE,
    STOP,
    SKIP,
    RESUME,
    SIMULATE_HANG,
    SIMULATE_INVALID_TRANSITION
};

class StateMachine {
public:
    StateMachine();

    // Attempt a transition. Returns true if valid, false if rejected.
    bool transition(PlayerCommand command);

    // Force a state directly - used by watchdog recovery only
    void force_state(PlayerState new_state);

    PlayerState current_state() const;
    std::string state_name() const;
    std::string state_name(PlayerState state) const;

    // Callback fired on every successful transition - command handler uses this
    void set_on_transition(std::function<void(PlayerState, PlayerState)> callback);

private:
    PlayerState state_;
    mutable std::mutex mutex_;
    std::function<void(PlayerState, PlayerState)> on_transition_;

    bool is_valid_transition(PlayerState from, PlayerCommand command) const;
    PlayerState next_state(PlayerState from, PlayerCommand command) const;
};