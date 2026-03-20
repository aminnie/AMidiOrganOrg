# CMake generated Testfile for 
# Source directory: C:/Users/a_min/AMidiOrganOrg
# Build directory: C:/Users/a_min/AMidiOrganOrg/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AMidiOrganTests]=] "C:/Users/a_min/AMidiOrganOrg/build/AMidiOrganTests_artefacts/Debug/AMidiOrganTests.exe")
  set_tests_properties([=[AMidiOrganTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;171;add_test;C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AMidiOrganTests]=] "C:/Users/a_min/AMidiOrganOrg/build/AMidiOrganTests_artefacts/Release/AMidiOrganTests.exe")
  set_tests_properties([=[AMidiOrganTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;171;add_test;C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[AMidiOrganTests]=] "C:/Users/a_min/AMidiOrganOrg/build/AMidiOrganTests_artefacts/MinSizeRel/AMidiOrganTests.exe")
  set_tests_properties([=[AMidiOrganTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;171;add_test;C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[AMidiOrganTests]=] "C:/Users/a_min/AMidiOrganOrg/build/AMidiOrganTests_artefacts/RelWithDebInfo/AMidiOrganTests.exe")
  set_tests_properties([=[AMidiOrganTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;171;add_test;C:/Users/a_min/AMidiOrganOrg/CMakeLists.txt;0;")
else()
  add_test([=[AMidiOrganTests]=] NOT_AVAILABLE)
endif()
subdirs("JUCE")
