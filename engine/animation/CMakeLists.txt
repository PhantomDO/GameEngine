find_package(fastgltf CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(ozz-animation CONFIG REQUIRED)

add_library(levain_animation STATIC src/gltf_bridge.cpp src/pose.cpp)
add_library(levain::animation ALIAS levain_animation)

target_include_directories(levain_animation PUBLIC include)

# ozz et fastgltf sont PRIVATE : seul src/ les inclut, l'API ne rend que nos types et glm (ADR-0022,
# tests/check_asset_libraries_visibility.cmake). animation_offline apporte les builders de la
# passerelle, et avec eux l'exécution (ozz::animation) et la base.
target_link_libraries(levain_animation PUBLIC levain::core glm::glm
                      PRIVATE fastgltf::fastgltf ozz::animation_offline)
