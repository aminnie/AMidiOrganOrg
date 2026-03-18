cmake_minimum_required(VERSION 3.22)

project(AMidiOrgan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Point this to your local JUCE checkout if JUCE isn't installed as a package.
# Example (Windows): -DJUCE_ROOT="C:/JUCE"
set(JUCE_ROOT "C:/JUCE" CACHE PATH "Path to JUCE root")

if(EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    add_subdirectory("${JUCE_ROOT}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_gui_app(AMidiOrgan
    PRODUCT_NAME "AMidiOrgan"
)

juce_generate_juce_header(AMidiOrgan)

juce_add_binary_data(AMidiOrganBinaryData
    SOURCES
        assets/keyboard.png
        assets/icons8arrowdown32.png
        assets/icons8arrowup32.png
        assets/icons8arrowdown32click.png
        assets/icons8arrowup32click.png
)

target_sources(AMidiOrgan
    PRIVATE
        Main.cpp
        AMidiControl.h
        AMidiDevices.h
        AMidiInstruments.h
        AMidiButtons.h
        AMidiRotors.h
        AMidiUtils.h
        DemoUtilities.h
)

target_include_directories(AMidiOrgan
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(AMidiOrgan
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(AMidiOrgan
    PRIVATE
        AMidiOrganBinaryData
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()

juce_add_console_app(AMidiOrganTests
    PRODUCT_NAME "AMidiOrganTests"
)

juce_generate_juce_header(AMidiOrganTests)

target_sources(AMidiOrganTests
    PRIVATE
        tests/test_main.cpp
)

target_include_directories(AMidiOrganTests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(AMidiOrganTests
    PRIVATE
        AMidiOrganBinaryData
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

target_compile_definitions(AMidiOrganTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

add_test(NAME AMidiOrganTests COMMAND AMidiOrganTests)
