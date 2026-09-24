find_package(Stb REQUIRED)
find_package(fastgltf CONFIG REQUIRED)

add_library(levain_assets STATIC
    src/asset_id.cpp
    src/gltf.cpp
    src/image.cpp
    src/registry.cpp)
add_library(levain::assets ALIAS levain_assets)

target_include_directories(levain_assets PUBLIC include)

# stb est PRIVATE : seul image.cpp l'inclut, l'API du module n'expose que des types standard.
# SYSTEM : le code de stb est compilé dans image.cpp (bibliothèque « en un en-tête »), ses
# avertissements ne sont pas les nôtres et ne doivent pas casser -Werror.
target_include_directories(levain_assets SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

# Les macros qui compilent le code de stb_image et de stb_image_write, sur ce seul fichier, et depuis
# le build plutôt que par un #define : leur nom est imposé par stb, et clang-tidy refuserait leur
# casse dans nos sources.
set_source_files_properties(src/image.cpp
    PROPERTIES COMPILE_DEFINITIONS "STB_IMAGE_IMPLEMENTATION;STB_IMAGE_WRITE_IMPLEMENTATION")

# scene est PUBLIC : l'import crée des entités, et l'en-tête expose leurs composants (SPECS §7 place
# assets au-dessus de scene). fastgltf est PRIVATE : seul src/gltf.cpp l'inclut, l'API ne rend que
# nos types (tests/check_fastgltf_visibility.cmake).
target_link_libraries(levain_assets PUBLIC levain::core levain::scene PRIVATE fastgltf::fastgltf)
