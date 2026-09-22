add_library(levain_input STATIC src/bindings.cpp)
add_library(levain::input ALIAS levain_input)

target_include_directories(levain_input PUBLIC include)

# platform est PUBLIC : les codes et les appareils de `platform/input.hpp` sont dans notre API.
# SDL, lui, ne traverse pas platform (ADR-0003).
target_link_libraries(levain_input PUBLIC levain::core levain::platform)
