add_executable(dmx_tests dmx_loopback_test.cpp)

target_link_libraries(dmx_tests PRIVATE dmx_core)
target_include_directories(dmx_tests PRIVATE ..)

add_test(NAME dmx_tests COMMAND dmx_tests)
