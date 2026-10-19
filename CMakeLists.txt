cmake_minimum_required(VERSION 3.22)
project(JT4000Controller VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pure C++ core and its tests (no JUCE needed): ctest, or `sh tests/run.sh`
enable_testing()
add_executable(core_test tests/test_core.cpp)
add_test(NAME core COMMAND core_test ${CMAKE_SOURCE_DIR}/map/params.csv)

# The plugin needs JUCE. CI clones it into third_party/JUCE.
if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt)
    add_subdirectory(third_party/JUCE)
    include(FetchContent)
    FetchContent_Declare(cat_ui GIT_REPOSITORY https://github.com/SpeakingColors/cat-ui GIT_TAG main GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(cat_ui)

    juce_add_binary_data(JT4000ControllerData SOURCES map/params.csv)
    set_target_properties(JT4000ControllerData PROPERTIES POSITION_INDEPENDENT_CODE ON)

    juce_add_plugin(JT4000Controller
        COMPANY_NAME "Studio 87"
        PLUGIN_MANUFACTURER_CODE Stu8
        PLUGIN_CODE Jt01
        FORMATS VST3 Standalone
        PRODUCT_NAME "JT-4000 Controller"
        IS_MIDI_EFFECT TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        VST3_CATEGORIES Tools
        COPY_PLUGIN_AFTER_BUILD FALSE)
    target_sources(JT4000Controller PRIVATE src/Plugin.cpp)
    target_include_directories(JT4000Controller PRIVATE src)
    target_compile_definitions(JT4000Controller PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(JT4000Controller PRIVATE JT4000ControllerData cat_ui::cat_ui juce::juce_audio_utils juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
