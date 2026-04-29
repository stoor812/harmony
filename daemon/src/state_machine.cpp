#include "state_machine.h"
#include "logger.h"

StateMachine::StateMachine() : state_(PlayerState::IDLE) {
    Logger::instance().info("STATE", "State machine initialized in IDLE");
}

bool StateMachine::transition(PlayerCommand command) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!is_valid_transition(state_, command)) {
        Logger::instance().warn("STATE",
            "Rejected invalid transition from " + state_name(state_) +
            " with command " + std::to_string(static_cast<int>(command)));
        return false;
    }

    PlayerState old_state = state_;
    state_ = next_state(state_, command);

    Logger::instance().info("STATE",
        "Transition: " + state_name(old_state) + " → " + state_name(state_));

    // Fire callback so command handler can trigger side effects
    if (on_transition_) {
        on_transition_(old_state, state_);
    }

    return true;
}

void StateMachine::force_state(PlayerState new_state) {
    std::lock_guard<std::mutex> lock(mutex_);
    PlayerState old_state = state_;
    state_ = new_state;
    Logger::instance().warn("STATE",
        "FORCED state change: " + state_name(old_state) + " → " + state_name(new_state));
}

PlayerState StateMachine::current_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

std::string StateMachine::state_name() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_name(state_);
}

std::string StateMachine::state_name(PlayerState state) const {
    switch (state) {
        case PlayerState::IDLE:      return "IDLE";
        case PlayerState::BUFFERING: return "BUFFERING";
        case PlayerState::PLAYING:   return "PLAYING";
        case PlayerState::PAUSED:    return "PAUSED";
        case PlayerState::STOPPED:   return "STOPPED";
        default:                     return "UNKNOWN";
    }
}

void StateMachine::set_on_transition(std::function<void(PlayerState, PlayerState)> callback) {
    on_transition_ = callback;
}

// This is the heart of the state machine - the transition table.
// Every valid command from every state is defined here.
// Anything not listed is REJECTED.
bool StateMachine::is_valid_transition(PlayerState from, PlayerCommand command) const {
    switch (from) {
        case PlayerState::IDLE:
            return command == PlayerCommand::PLAY;

        case PlayerState::BUFFERING:
            // Can only stop or let it finish naturally into PLAYING
            return command == PlayerCommand::STOP ||
                   command == PlayerCommand::PLAY;  // PLAY = buffer fill complete

        case PlayerState::PLAYING:
            return command == PlayerCommand::PAUSE ||
                   command == PlayerCommand::STOP  ||
                   command == PlayerCommand::SKIP;

        case PlayerState::PAUSED:
            return command == PlayerCommand::RESUME ||
                   command == PlayerCommand::STOP   ||
                   command == PlayerCommand::SKIP;

        case PlayerState::STOPPED:
            return command == PlayerCommand::PLAY;

        default:
            return false;
    }
}

PlayerState StateMachine::next_state(PlayerState from, PlayerCommand command) const {
    switch (from) {
        case PlayerState::IDLE:
            if (command == PlayerCommand::PLAY)    return PlayerState::BUFFERING;
            break;

        case PlayerState::BUFFERING:
            if (command == PlayerCommand::PLAY)    return PlayerState::PLAYING;
            if (command == PlayerCommand::STOP)    return PlayerState::IDLE;
            break;

        case PlayerState::PLAYING:
            if (command == PlayerCommand::PAUSE)   return PlayerState::PAUSED;
            if (command == PlayerCommand::STOP)    return PlayerState::STOPPED;
            if (command == PlayerCommand::SKIP)    return PlayerState::BUFFERING;
            break;

        case PlayerState::PAUSED:
            if (command == PlayerCommand::RESUME)  return PlayerState::PLAYING;
            if (command == PlayerCommand::STOP)    return PlayerState::STOPPED;
            if (command == PlayerCommand::SKIP)    return PlayerState::BUFFERING;
            break;

        case PlayerState::STOPPED:
            if (command == PlayerCommand::PLAY)    return PlayerState::IDLE;
            break;

        default:
            break;
    }
    return from; // Should never reach here if is_valid_transition is called first
}