add_executable(dmx_demo dmx_demo.cpp)
target_link_libraries(dmx_demo PRIVATE dmx_core)
target_include_directories(dmx_demo PRIVATE ..)

add_executable(fake_node fake_node.cpp)
target_link_libraries(fake_node PRIVATE asio_standalone)
