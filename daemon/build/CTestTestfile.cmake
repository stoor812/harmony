# CMake generated Testfile for 
# Source directory: /workspaces/harmony/daemon
# Build directory: /workspaces/harmony/daemon/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(HarmonyTests "/workspaces/harmony/daemon/build/test_harmony")
set_tests_properties(HarmonyTests PROPERTIES  _BACKTRACE_TRIPLES "/workspaces/harmony/daemon/CMakeLists.txt;83;add_test;/workspaces/harmony/daemon/CMakeLists.txt;0;")
subdirs("_deps/httplib-build")
subdirs("_deps/nlohmann_json-build")
subdirs("_deps/doctest-build")
