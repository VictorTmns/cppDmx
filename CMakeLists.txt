cmake_minimum_required(VERSION 3.22)

project(cppDmx VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Is this the top-level project, or are we being pulled in via FetchContent /
# add_subdirectory by a consumer? The app/demo/tests only make sense when we are
# building the project standalone; a consumer wants the library alone.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CPPDMX_IS_TOP_LEVEL ON)
else()
    set(CPPDMX_IS_TOP_LEVEL OFF)
endif()

option(CPPDMX_BUILD_GUI   "Build the JUCE GUI controller (dmx_gui)" ${CPPDMX_IS_TOP_LEVEL})
option(CPPDMX_BUILD_DEMO  "Build the demo and fake_node executables" ${CPPDMX_IS_TOP_LEVEL})
option(CPPDMX_BUILD_TESTS "Build the loopback tests"                 ${CPPDMX_IS_TOP_LEVEL})

# Source files contain literal UTF-8 (em dash, ellipsis). MSVC otherwise decodes
# them as the system code page and mangles the text; /utf-8 sets both the source
# and execution character set to UTF-8. No-op for Clang/GCC.
if(MSVC)
    add_compile_options(/utf-8)
endif()

# The library — always built; this is what consumers link.
add_subdirectory(cppDmx)

if(CPPDMX_BUILD_GUI)
    add_subdirectory(app)
endif()

if(CPPDMX_BUILD_DEMO)
    add_subdirectory(demo)
endif()

if(CPPDMX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
