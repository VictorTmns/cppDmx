cmake_minimum_required(VERSION 3.22)

# C is required as well as C++: juce_graphics compiles a C source (Sheenbidi),
# so the GUI target won't generate with only CXX enabled.
project(DmxTestbench VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files contain literal UTF-8 (em dash, ellipsis). MSVC otherwise decodes
# them as the system code page and mangles the text; /utf-8 sets both the source
# and execution character set to UTF-8. No-op for Clang/GCC.
if(MSVC)
    add_compile_options(/utf-8)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4          # pin to the latest JUCE 8 release tag
)
FetchContent_MakeAvailable(JUCE)

# Standalone Asio (header-only, no Boost) — UDP networking for the JUCE-free dmx
# core. The repo has no top-level CMake target, so we fetch it and wrap the
# headers in an INTERFACE library ourselves.
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

find_package(Threads REQUIRED)

add_library(asio_standalone INTERFACE)
target_include_directories(asio_standalone INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
target_link_libraries(asio_standalone INTERFACE Threads::Threads)
if(WIN32)
    # Asio needs a Windows version baseline and the Winsock libraries.
    target_compile_definitions(asio_standalone INTERFACE _WIN32_WINNT=0x0A00)  # Windows 10
    target_link_libraries(asio_standalone INTERFACE ws2_32 wsock32)
endif()

# ---------------------------------------------------------------------------
# dmx_core — the engine + outputs. JUCE-free: depends only on the C++ standard
# library and Asio (for UDP). This is "the swap seam" the controller plugs into.
# Asio is linked PRIVATE: it is used only inside ArtNetOutput.cpp (pimpl), so it
# never leaks into this library's public headers, yet the static archive still
# carries the Winsock/Threads link requirement forward to whoever links dmx_core.
# ---------------------------------------------------------------------------
add_library(dmx_core STATIC
    dmx/DmxEngine.cpp
    "dmx/Outputs/ArtNetOutput.cpp"
    dmx/ArtNetDiscovery.cpp)

# Root lets consumers say "../dmx/Outputs/ArtNetOutput.h"; dmx/ lets the headers
# resolve their sibling "IDmxOutput.h" now that the outputs live in dmx/Outputs/.
target_include_directories(dmx_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dmx)
target_link_libraries(dmx_core PRIVATE asio_standalone)

# ---------------------------------------------------------------------------
# Visual bring-up demo: sends Art-Net to a receiver (e.g. BlenderDMX).
# Plain executable — no JUCE, it only uses dmx_core + the standard library.
# ---------------------------------------------------------------------------
add_executable(dmx_demo demo/dmx_demo.cpp)
target_link_libraries(dmx_demo PRIVATE dmx_core)

# ---------------------------------------------------------------------------
# GUI controller: detect Art-Net outputs and choose the target universe.
# Reuses dmx_core (engine + outputs + discovery, all JUCE-free); only this target
# pulls in the richer JUCE GUI modules for the UI itself.
# ---------------------------------------------------------------------------
juce_add_gui_app(dmx_gui PRODUCT_NAME "DMX Controller")

target_sources(dmx_gui PRIVATE
    app/Main.cpp
    app/MainComponent.cpp)

target_compile_definitions(dmx_gui PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(dmx_gui PRIVATE
    dmx_core
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Fake Art-Net node: a test stand-in that answers ArtPoll so the GUI's "Detect"
# can discover it, and logs ArtDmx it receives. Plain executable, JUCE-free
# (standard library + Asio, the same UDP stack as dmx_core).
# ---------------------------------------------------------------------------
add_executable(fake_node demo/fake_node.cpp)
target_link_libraries(fake_node PRIVATE asio_standalone)

# ---------------------------------------------------------------------------
# Loopback unit test (no network, no JUCE). Delete this target, the tests/
# folder and dmx/Outputs/LoopbackOutput.h to remove the harness.
# ---------------------------------------------------------------------------
add_executable(dmx_tests tests/dmx_loopback_test.cpp)
target_link_libraries(dmx_tests PRIVATE dmx_core)

enable_testing()
add_test(NAME dmx_tests COMMAND dmx_tests)
