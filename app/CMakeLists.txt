include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(dmx_gui PRODUCT_NAME "DMX Controller")

target_sources(dmx_gui PRIVATE
    Main.cpp
    MainComponent.cpp)

target_compile_definitions(dmx_gui PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(dmx_gui PRIVATE
    dmx_core
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

target_include_directories(dmx_gui PRIVATE
    ..)
