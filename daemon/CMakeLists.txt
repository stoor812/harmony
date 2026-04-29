cmake_minimum_required(VERSION 3.16)
project(harmony VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Dependencies ─────────────────────────────────────────────────────────────

# cpr (HTTP client for iTunes API calls) - installed via setup.sh
find_package(cpr REQUIRED)

# cpp-httplib (HTTP server) - single header, fetched automatically
include(FetchContent)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.14.1
)
FetchContent_MakeAvailable(httplib)

# nlohmann/json (JSON parsing) - single header, fetched automatically
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# doctest (unit testing) - single header, fetched automatically
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

# ─── Main Executable ──────────────────────────────────────────────────────────

add_executable(harmony
  src/main.cpp
  src/logger.cpp
  src/state_machine.cpp
  src/queue.cpp
  src/watchdog.cpp
  src/memory_budget.cpp
  src/itunes_client.cpp
  src/command_handler.cpp
)

target_include_directories(harmony PRIVATE 
    src
    ${nlohmann_json_SOURCE_DIR}/include
)

target_link_libraries(harmony PRIVATE
  cpr::cpr
  httplib::httplib
  nlohmann_json::nlohmann_json
  pthread
)

# ─── Unit Tests ───────────────────────────────────────────────────────────────

add_executable(test_harmony
  tests/test_state_machine.cpp
  src/logger.cpp
  src/state_machine.cpp
  src/memory_budget.cpp
)

target_include_directories(test_harmony PRIVATE 
    src
    ${nlohmann_json_SOURCE_DIR}/include
)

target_link_libraries(test_harmony PRIVATE
  doctest::doctest
  nlohmann_json::nlohmann_json
  pthread
)

enable_testing()
add_test(NAME HarmonyTests COMMAND test_harmony)

# ─── Copy tracks.json to build directory ──────────────────────────────────────

configure_file(
  ${CMAKE_SOURCE_DIR}/data/tracks.json
  ${CMAKE_BINARY_DIR}/data/tracks.json
  COPYONLY
)