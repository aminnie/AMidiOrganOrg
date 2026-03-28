# CMake generated Testfile for 
# Source directory: /Users/anton.minnie/AMidiOrganOrg
# Build directory: /Users/anton.minnie/AMidiOrganOrg/build-release-local
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[AMidiOrganTestsBuildSanity]=] "/opt/homebrew/bin/cmake" "-E" "echo" "AMidiOrganTests built on macOS; runtime execution is temporarily disabled.")
set_tests_properties([=[AMidiOrganTestsBuildSanity]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/anton.minnie/AMidiOrganOrg/CMakeLists.txt;183;add_test;/Users/anton.minnie/AMidiOrganOrg/CMakeLists.txt;0;")
subdirs("JUCE")
