#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "state_machine.h"

// ─── Initialization ───────────────────────────────────────────────────────────

TEST_SUITE("Initialization") {

    TEST_CASE("starts in IDLE state") {
        StateMachine sm;
        CHECK(sm.current_state() == PlayerState::IDLE);
    }

    TEST_CASE("state_name() returns IDLE on construction") {
        StateMachine sm;
        CHECK(sm.state_name() == "IDLE");
    }

    TEST_CASE("two independent instances start in IDLE") {
        StateMachine a, b;
        CHECK(a.current_state() == PlayerState::IDLE);
        CHECK(b.current_state() == PlayerState::IDLE);
    }
}

// ─── Valid Transitions ────────────────────────────────────────────────────────

TEST_SUITE("Valid Transitions") {

    TEST_CASE("IDLE → BUFFERING via PLAY") {
        StateMachine sm;
        bool ok = sm.transition(PlayerCommand::PLAY);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("BUFFERING → PLAYING via PLAY (buffer fill complete)") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);                     // IDLE → BUFFERING
        bool ok = sm.transition(PlayerCommand::PLAY);           // BUFFERING → PLAYING
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::PLAYING);
    }

    TEST_CASE("BUFFERING → IDLE via STOP") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);                     // IDLE → BUFFERING
        bool ok = sm.transition(PlayerCommand::STOP);           // BUFFERING → IDLE
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::IDLE);
    }

    TEST_CASE("PLAYING → PAUSED via PAUSE") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        bool ok = sm.transition(PlayerCommand::PAUSE);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::PAUSED);
    }

    TEST_CASE("PLAYING → STOPPED via STOP") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        bool ok = sm.transition(PlayerCommand::STOP);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::STOPPED);
    }

    TEST_CASE("PLAYING → BUFFERING via SKIP") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        bool ok = sm.transition(PlayerCommand::SKIP);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("PAUSED → PLAYING via RESUME") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        bool ok = sm.transition(PlayerCommand::RESUME);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::PLAYING);
    }

    TEST_CASE("PAUSED → STOPPED via STOP") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        bool ok = sm.transition(PlayerCommand::STOP);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::STOPPED);
    }

    TEST_CASE("PAUSED → BUFFERING via SKIP") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        bool ok = sm.transition(PlayerCommand::SKIP);
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("STOPPED → IDLE via PLAY") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);                     // PLAYING → STOPPED
        bool ok = sm.transition(PlayerCommand::PLAY);           // STOPPED → IDLE
        CHECK(ok == true);
        CHECK(sm.current_state() == PlayerState::IDLE);
    }
}

// ─── Invalid Transitions ──────────────────────────────────────────────────────

TEST_SUITE("Invalid Transitions - IDLE") {

    TEST_CASE("cannot PAUSE from IDLE") {
        StateMachine sm;
        CHECK(sm.transition(PlayerCommand::PAUSE) == false);
    }

    TEST_CASE("cannot STOP from IDLE") {
        StateMachine sm;
        CHECK(sm.transition(PlayerCommand::STOP) == false);
    }

    TEST_CASE("cannot SKIP from IDLE") {
        StateMachine sm;
        CHECK(sm.transition(PlayerCommand::SKIP) == false);
    }

    TEST_CASE("cannot RESUME from IDLE") {
        StateMachine sm;
        CHECK(sm.transition(PlayerCommand::RESUME) == false);
    }

    TEST_CASE("cannot SIMULATE_HANG from IDLE") {
        StateMachine sm;
        CHECK(sm.transition(PlayerCommand::SIMULATE_HANG) == false);
    }

    TEST_CASE("cannot SIMULATE_INVALID_TRANSITION from IDLE") {
        StateMachine sm;
        CHECK(sm.transition(PlayerCommand::SIMULATE_INVALID_TRANSITION) == false);
    }
}

TEST_SUITE("Invalid Transitions - BUFFERING") {

    TEST_CASE("cannot PAUSE from BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::PAUSE) == false);
    }

    TEST_CASE("cannot SKIP from BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::SKIP) == false);
    }

    TEST_CASE("cannot RESUME from BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::RESUME) == false);
    }

    TEST_CASE("cannot SIMULATE_HANG from BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::SIMULATE_HANG) == false);
    }

    TEST_CASE("cannot SIMULATE_INVALID_TRANSITION from BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::SIMULATE_INVALID_TRANSITION) == false);
    }
}

TEST_SUITE("Invalid Transitions - PLAYING") {

    TEST_CASE("cannot PLAY from PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::PLAY) == false);
    }

    TEST_CASE("cannot RESUME from PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::RESUME) == false);
    }

    TEST_CASE("cannot SIMULATE_HANG from PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::SIMULATE_HANG) == false);
    }

    TEST_CASE("cannot SIMULATE_INVALID_TRANSITION from PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.transition(PlayerCommand::SIMULATE_INVALID_TRANSITION) == false);
    }
}

TEST_SUITE("Invalid Transitions - PAUSED") {

    TEST_CASE("cannot PLAY from PAUSED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.transition(PlayerCommand::PLAY) == false);
    }

    TEST_CASE("cannot PAUSE from PAUSED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.transition(PlayerCommand::PAUSE) == false);
    }

    TEST_CASE("cannot SIMULATE_HANG from PAUSED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.transition(PlayerCommand::SIMULATE_HANG) == false);
    }

    TEST_CASE("cannot SIMULATE_INVALID_TRANSITION from PAUSED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.transition(PlayerCommand::SIMULATE_INVALID_TRANSITION) == false);
    }
}

TEST_SUITE("Invalid Transitions - STOPPED") {

    TEST_CASE("cannot PAUSE from STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.transition(PlayerCommand::PAUSE) == false);
    }

    TEST_CASE("cannot STOP from STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.transition(PlayerCommand::STOP) == false);
    }

    TEST_CASE("cannot SKIP from STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.transition(PlayerCommand::SKIP) == false);
    }

    TEST_CASE("cannot RESUME from STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.transition(PlayerCommand::RESUME) == false);
    }

    TEST_CASE("cannot SIMULATE_HANG from STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.transition(PlayerCommand::SIMULATE_HANG) == false);
    }

    TEST_CASE("cannot SIMULATE_INVALID_TRANSITION from STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.transition(PlayerCommand::SIMULATE_INVALID_TRANSITION) == false);
    }
}

// ─── State Enforcement ────────────────────────────────────────────────────────

TEST_SUITE("State Enforcement") {

    TEST_CASE("rejected command leaves state unchanged - IDLE") {
        StateMachine sm;
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.current_state() == PlayerState::IDLE);
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.current_state() == PlayerState::IDLE);
        sm.transition(PlayerCommand::SKIP);
        CHECK(sm.current_state() == PlayerState::IDLE);
        sm.transition(PlayerCommand::RESUME);
        CHECK(sm.current_state() == PlayerState::IDLE);
    }

    TEST_CASE("rejected command leaves state unchanged - BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
        sm.transition(PlayerCommand::SKIP);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
        sm.transition(PlayerCommand::RESUME);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("rejected command leaves state unchanged - PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);     // rejected - stays PLAYING
        CHECK(sm.current_state() == PlayerState::PLAYING);
        sm.transition(PlayerCommand::RESUME);   // rejected
        CHECK(sm.current_state() == PlayerState::PLAYING);
    }

    TEST_CASE("rejected command leaves state unchanged - PAUSED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        sm.transition(PlayerCommand::PLAY);     // rejected
        CHECK(sm.current_state() == PlayerState::PAUSED);
        sm.transition(PlayerCommand::PAUSE);    // rejected
        CHECK(sm.current_state() == PlayerState::PAUSED);
    }

    TEST_CASE("rejected command leaves state unchanged - STOPPED") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        sm.transition(PlayerCommand::PAUSE);    // rejected
        CHECK(sm.current_state() == PlayerState::STOPPED);
        sm.transition(PlayerCommand::STOP);     // rejected
        CHECK(sm.current_state() == PlayerState::STOPPED);
        sm.transition(PlayerCommand::RESUME);   // rejected
        CHECK(sm.current_state() == PlayerState::STOPPED);
    }

    TEST_CASE("many consecutive invalid commands don't corrupt state") {
        StateMachine sm;
        for (int i = 0; i < 10; ++i) {
            sm.transition(PlayerCommand::PAUSE);
            sm.transition(PlayerCommand::STOP);
            sm.transition(PlayerCommand::RESUME);
            sm.transition(PlayerCommand::SKIP);
        }
        CHECK(sm.current_state() == PlayerState::IDLE);
    }

    TEST_CASE("current_state() and state_name() stay consistent after rejections") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);    // rejected
        CHECK(sm.current_state() == PlayerState::BUFFERING);
        CHECK(sm.state_name() == "BUFFERING");

        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::RESUME);   // rejected
        CHECK(sm.current_state() == PlayerState::PLAYING);
        CHECK(sm.state_name() == "PLAYING");
    }
}

// ─── Transition Sequences ─────────────────────────────────────────────────────

TEST_SUITE("Transition Sequences") {

    TEST_CASE("full playback sequence: IDLE → play → pause → resume → stop") {
        StateMachine sm;
        CHECK(sm.current_state() == PlayerState::IDLE);

        CHECK(sm.transition(PlayerCommand::PLAY) == true);      // → BUFFERING
        CHECK(sm.current_state() == PlayerState::BUFFERING);

        CHECK(sm.transition(PlayerCommand::PLAY) == true);      // → PLAYING
        CHECK(sm.current_state() == PlayerState::PLAYING);

        CHECK(sm.transition(PlayerCommand::PAUSE) == true);     // → PAUSED
        CHECK(sm.current_state() == PlayerState::PAUSED);

        CHECK(sm.transition(PlayerCommand::RESUME) == true);    // → PLAYING
        CHECK(sm.current_state() == PlayerState::PLAYING);

        CHECK(sm.transition(PlayerCommand::STOP) == true);      // → STOPPED
        CHECK(sm.current_state() == PlayerState::STOPPED);
    }

    TEST_CASE("skip from PLAYING re-enters BUFFERING then PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);                     // → BUFFERING
        sm.transition(PlayerCommand::PLAY);                     // → PLAYING
        sm.transition(PlayerCommand::SKIP);                     // → BUFFERING
        CHECK(sm.current_state() == PlayerState::BUFFERING);

        sm.transition(PlayerCommand::PLAY);                     // → PLAYING
        CHECK(sm.current_state() == PlayerState::PLAYING);
    }

    TEST_CASE("skip from PAUSED re-enters BUFFERING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PAUSE);
        sm.transition(PlayerCommand::SKIP);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("stop during BUFFERING returns to IDLE") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);                     // → BUFFERING
        sm.transition(PlayerCommand::STOP);                     // → IDLE
        CHECK(sm.current_state() == PlayerState::IDLE);
    }

    TEST_CASE("full cycle: play → stop → reset → play again from IDLE") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);                     // PLAYING → STOPPED
        sm.transition(PlayerCommand::PLAY);                     // STOPPED → IDLE
        CHECK(sm.current_state() == PlayerState::IDLE);

        CHECK(sm.transition(PlayerCommand::PLAY) == true);      // IDLE → BUFFERING
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("multiple pause/resume cycles maintain correct state") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);

        for (int i = 0; i < 5; ++i) {
            sm.transition(PlayerCommand::PAUSE);
            CHECK(sm.current_state() == PlayerState::PAUSED);
            sm.transition(PlayerCommand::RESUME);
            CHECK(sm.current_state() == PlayerState::PLAYING);
        }
    }

    TEST_CASE("interleaved valid and invalid commands keep state correct") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);                     // → BUFFERING
        sm.transition(PlayerCommand::PAUSE);                    // rejected
        CHECK(sm.current_state() == PlayerState::BUFFERING);

        sm.transition(PlayerCommand::PLAY);                     // → PLAYING
        sm.transition(PlayerCommand::RESUME);                   // rejected
        CHECK(sm.current_state() == PlayerState::PLAYING);

        sm.transition(PlayerCommand::PAUSE);                    // → PAUSED
        sm.transition(PlayerCommand::PLAY);                     // rejected
        CHECK(sm.current_state() == PlayerState::PAUSED);
    }

    TEST_CASE("multiple skip chain: PLAYING → BUFFERING repeated") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);

        for (int i = 0; i < 3; ++i) {
            sm.transition(PlayerCommand::SKIP);                 // → BUFFERING
            CHECK(sm.current_state() == PlayerState::BUFFERING);
            sm.transition(PlayerCommand::PLAY);                 // → PLAYING
            CHECK(sm.current_state() == PlayerState::PLAYING);
        }
    }
}

// ─── Callback Behavior ────────────────────────────────────────────────────────

TEST_SUITE("Callback Behavior") {

    TEST_CASE("callback fires on valid transition") {
        StateMachine sm;
        int call_count = 0;
        sm.set_on_transition([&](PlayerState, PlayerState) { ++call_count; });

        sm.transition(PlayerCommand::PLAY);
        CHECK(call_count == 1);
    }

    TEST_CASE("callback does not fire on invalid transition") {
        StateMachine sm;
        int call_count = 0;
        sm.set_on_transition([&](PlayerState, PlayerState) { ++call_count; });

        sm.transition(PlayerCommand::PAUSE);
        CHECK(call_count == 0);
    }

    TEST_CASE("callback receives correct from/to states") {
        StateMachine sm;
        PlayerState captured_from = PlayerState::STOPPED;
        PlayerState captured_to   = PlayerState::STOPPED;
        sm.set_on_transition([&](PlayerState f, PlayerState t) {
            captured_from = f;
            captured_to   = t;
        });

        sm.transition(PlayerCommand::PLAY);
        CHECK(captured_from == PlayerState::IDLE);
        CHECK(captured_to   == PlayerState::BUFFERING);
    }

    TEST_CASE("callback fires exactly once per valid transition, not on rejections") {
        StateMachine sm;
        int call_count = 0;
        sm.set_on_transition([&](PlayerState, PlayerState) { ++call_count; });

        sm.transition(PlayerCommand::PLAY);     // valid   → 1
        sm.transition(PlayerCommand::PAUSE);    // rejected
        sm.transition(PlayerCommand::PLAY);     // valid   → 2
        sm.transition(PlayerCommand::RESUME);   // rejected
        CHECK(call_count == 2);
    }

    TEST_CASE("callback tracks all transitions in a full sequence") {
        StateMachine sm;
        int call_count = 0;
        sm.set_on_transition([&](PlayerState, PlayerState) { ++call_count; });

        sm.transition(PlayerCommand::PLAY);     // IDLE → BUFFERING
        sm.transition(PlayerCommand::PLAY);     // BUFFERING → PLAYING
        sm.transition(PlayerCommand::PAUSE);    // PLAYING → PAUSED
        sm.transition(PlayerCommand::RESUME);   // PAUSED → PLAYING
        sm.transition(PlayerCommand::STOP);     // PLAYING → STOPPED
        CHECK(call_count == 5);
    }

    TEST_CASE("callback state history is correct across full sequence") {
        StateMachine sm;
        std::vector<std::pair<PlayerState, PlayerState>> history;
        sm.set_on_transition([&](PlayerState f, PlayerState t) {
            history.push_back({f, t});
        });

        sm.transition(PlayerCommand::PLAY);     // IDLE → BUFFERING
        sm.transition(PlayerCommand::PLAY);     // BUFFERING → PLAYING
        sm.transition(PlayerCommand::STOP);     // PLAYING → STOPPED

        REQUIRE(history.size() == 3);
        CHECK(history[0].first  == PlayerState::IDLE);
        CHECK(history[0].second == PlayerState::BUFFERING);
        CHECK(history[1].first  == PlayerState::BUFFERING);
        CHECK(history[1].second == PlayerState::PLAYING);
        CHECK(history[2].first  == PlayerState::PLAYING);
        CHECK(history[2].second == PlayerState::STOPPED);
    }
}

// ─── Force State (Watchdog Recovery) ─────────────────────────────────────────

TEST_SUITE("Force State") {

    TEST_CASE("force_state sets state directly regardless of current state") {
        StateMachine sm;
        sm.force_state(PlayerState::PLAYING);
        CHECK(sm.current_state() == PlayerState::PLAYING);
    }

    TEST_CASE("force_state to IDLE from PLAYING") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.force_state(PlayerState::IDLE);
        CHECK(sm.current_state() == PlayerState::IDLE);
    }

    TEST_CASE("after force_state, valid transitions apply from the forced state") {
        StateMachine sm;
        sm.force_state(PlayerState::PLAYING);
        CHECK(sm.transition(PlayerCommand::PAUSE) == true);
        CHECK(sm.current_state() == PlayerState::PAUSED);
    }

    TEST_CASE("after force_state, invalid transitions are still rejected") {
        StateMachine sm;
        sm.force_state(PlayerState::PLAYING);
        CHECK(sm.transition(PlayerCommand::RESUME) == false);
        CHECK(sm.current_state() == PlayerState::PLAYING);
    }

    TEST_CASE("force_state to IDLE enables a fresh play sequence") {
        StateMachine sm;
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::PLAY);
        sm.transition(PlayerCommand::STOP);
        sm.force_state(PlayerState::IDLE);
        CHECK(sm.current_state() == PlayerState::IDLE);
        CHECK(sm.transition(PlayerCommand::PLAY) == true);
        CHECK(sm.current_state() == PlayerState::BUFFERING);
    }

    TEST_CASE("force_state updates state_name()") {
        StateMachine sm;
        sm.force_state(PlayerState::PAUSED);
        CHECK(sm.state_name() == "PAUSED");
    }
}

// ─── State Name Strings ───────────────────────────────────────────────────────

TEST_SUITE("State Name Strings") {

    TEST_CASE("state_name(PlayerState) returns correct string for all states") {
        StateMachine sm;
        CHECK(sm.state_name(PlayerState::IDLE)      == "IDLE");
        CHECK(sm.state_name(PlayerState::BUFFERING) == "BUFFERING");
        CHECK(sm.state_name(PlayerState::PLAYING)   == "PLAYING");
        CHECK(sm.state_name(PlayerState::PAUSED)    == "PAUSED");
        CHECK(sm.state_name(PlayerState::STOPPED)   == "STOPPED");
    }

    TEST_CASE("state_name() tracks current state through transitions") {
        StateMachine sm;
        CHECK(sm.state_name() == "IDLE");
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.state_name() == "BUFFERING");
        sm.transition(PlayerCommand::PLAY);
        CHECK(sm.state_name() == "PLAYING");
        sm.transition(PlayerCommand::PAUSE);
        CHECK(sm.state_name() == "PAUSED");
        sm.transition(PlayerCommand::STOP);
        CHECK(sm.state_name() == "STOPPED");
    }
}
