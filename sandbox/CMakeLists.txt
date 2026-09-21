add_executable(levain_sandbox src/main.cpp)
target_link_libraries(levain_sandbox PRIVATE levain::core levain::platform levain::gpu levain::render)
