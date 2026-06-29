include(FetchContent)


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
    target_compile_definitions(asio_standalone INTERFACE _WIN32_WINNT=0x0A00)
    target_link_libraries(asio_standalone INTERFACE ws2_32 wsock32)
endif()


add_library(dmx_core STATIC
    DmxEngine.h DmxEngine.cpp
    IDmxDriver.h
    Containers/DmxUniverse.h Containers/DmxUniverse.cpp
    Helpers.h
    "Drivers/Art-Net/ArtNetDriver.h" "Drivers/Art-Net/ArtNetDriver.cpp"
    "Drivers/Art-Net/ArtNetDiscovery.h" "Drivers/Art-Net/ArtNetDiscovery.cpp"
)

target_include_directories(dmx_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dmx_core PRIVATE asio_standalone)
