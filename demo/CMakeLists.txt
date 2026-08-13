add_executable(artnet_demo artnet_demo.cpp)
target_link_libraries(artnet_demo PRIVATE cppDmx::cppDmx)

add_executable(usbpro_demo usbpro_demo.cpp)
target_link_libraries(usbpro_demo PRIVATE cppDmx::cppDmx)

add_executable(opendmx_demo opendmx_demo.cpp)
target_link_libraries(opendmx_demo PRIVATE cppDmx::cppDmx)

add_executable(fake_node fake_node.cpp)
target_link_libraries(fake_node PRIVATE asio_standalone)

add_executable(fake_usb_node fake_usb_node.cpp)
target_link_libraries(fake_usb_node PRIVATE asio_standalone)
