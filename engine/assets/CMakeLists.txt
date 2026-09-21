find_package(Stb REQUIRED)

add_library(levain_assets STATIC
    src/image.cpp)
add_library(levain::assets ALIAS levain_assets)

target_include_directories(levain_assets PUBLIC include)

# stb est PRIVATE : seul image.cpp l'inclut, l'API du module n'expose que des types standard.
# SYSTEM : le code de stb est compilé dans image.cpp (bibliothèque « en un en-tête »), ses
# avertissements ne sont pas les nôtres et ne doivent pas casser -Werror.
target_include_directories(levain_assets SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

# La macro qui compile le code de stb_image, sur ce seul fichier, et depuis le build plutôt que
# par un #define : son nom est imposé par stb, et clang-tidy refuserait sa casse dans nos sources.
set_source_files_properties(src/image.cpp
    PROPERTIES COMPILE_DEFINITIONS STB_IMAGE_IMPLEMENTATION)

target_link_libraries(levain_assets PUBLIC levain::core)
