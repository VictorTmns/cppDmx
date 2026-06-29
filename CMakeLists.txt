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

enable_testing()

add_subdirectory(cppDmx)
add_subdirectory(app)
add_subdirectory(demo)
add_subdirectory(tests)
