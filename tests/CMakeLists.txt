add_executable(dmx_tests dmx_loopback_test.cpp)

target_link_libraries(dmx_tests PRIVATE cppDmx::cppDmx)

add_test(NAME dmx_tests COMMAND dmx_tests)

add_executable(usbpro_framing_test usbpro_framing_test.cpp)

target_link_libraries(usbpro_framing_test PRIVATE cppDmx::cppDmx)

add_test(NAME usbpro_framing_test COMMAND usbpro_framing_test)

add_executable(opendmx_framing_test opendmx_framing_test.cpp)

target_link_libraries(opendmx_framing_test PRIVATE cppDmx::cppDmx)

add_test(NAME opendmx_framing_test COMMAND opendmx_framing_test)
