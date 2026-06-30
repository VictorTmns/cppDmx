include(FetchContent)
include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)

# --- Asio: a PRIVATE, build-time-only dependency --------------------------
# Confined to the .cpp files behind a pimpl, so it never leaks into the public
# headers. Guarded so a parent project (or package manager) can supply its own
# asio_standalone target before adding this library.
if(NOT TARGET asio_standalone)
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        asio-1-30-2
    )
    FetchContent_MakeAvailable(asio)

    add_library(asio_standalone INTERFACE)
    target_include_directories(asio_standalone INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
    if(WIN32)
        # Asio needs a Windows version baseline.
        target_compile_definitions(asio_standalone INTERFACE _WIN32_WINNT=0x0A00)
    endif()
endif()

find_package(Threads REQUIRED)

# --- The library ----------------------------------------------------------
add_library(cppDmx
    src/DmxEngine.cpp
    src/Containers/DmxUniverse.cpp
    "src/Drivers/Art-Net/ArtNetDriver.cpp"
    "src/Drivers/Art-Net/ArtNetDiscovery.cpp"
)
add_library(cppDmx::cppDmx ALIAS cppDmx)

target_compile_features(cppDmx PUBLIC cxx_std_17)

# Generates cppDmx_export.h defining the CPPDMX_API visibility macro. Put it
# under a cppDmx/ subdir so it is included as <cppDmx/cppDmx_export.h>, matching
# its installed location.
generate_export_header(cppDmx
    BASE_NAME        cppDmx
    EXPORT_MACRO_NAME CPPDMX_API
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/export/cppDmx/cppDmx_export.h)

# A static build must neutralise the dllimport/dllexport in the generated header.
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(cppDmx PUBLIC CPPDMX_STATIC_DEFINE)
endif()

target_include_directories(cppDmx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Asio is build-only (BUILD_INTERFACE), so it is NOT propagated into the export
# set — consumers of the installed/FetchContent'd library never need it. The
# system libraries that the compiled archive genuinely requires at link time
# (Threads, and Winsock on Windows) ARE propagated, PUBLIC.
target_link_libraries(cppDmx
    PRIVATE $<BUILD_INTERFACE:asio_standalone>
    PUBLIC  Threads::Threads)
if(WIN32)
    target_link_libraries(cppDmx PUBLIC ws2_32 wsock32)
endif()

if(MSVC)
    # C4251: an exported class (DmxEngine, ArtNetDriver) has STL data members
    # (std::string, std::unordered_map, ...) that lack a dll-interface. Benign
    # here: consumers build the library from source with their own toolchain
    # (FetchContent / find_package), so the DLL and consumer share one CRT/STL
    # and there is no ABI mismatch. (C4275 is fixed properly by exporting the
    # IDmxDriver base, so it is no longer suppressed.)
    target_compile_options(cppDmx PRIVATE /wd4251)
endif()

set_target_properties(cppDmx PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

# --- Install / export so consumers can use find_package(cppDmx) -----------
install(TARGETS cppDmx
    EXPORT  cppDmxTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Public headers and the generated export header.
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/export/cppDmx/cppDmx_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppDmx)

install(EXPORT cppDmxTargets
    FILE      cppDmxTargets.cmake
    NAMESPACE cppDmx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppDmx)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/cppDmxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cppDmxConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppDmx)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cppDmxConfigVersion.cmake
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cppDmxConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cppDmxConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppDmx)
