add_executable(dmx_tests dmx_loopback_test.cpp)

target_link_libraries(dmx_tests PRIVATE cppDmx::cppDmx)

add_test(NAME dmx_tests COMMAND dmx_tests)
