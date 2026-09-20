add_library(levain_core STATIC src/version.cpp)
add_library(levain::core ALIAS levain_core)

target_include_directories(levain_core PUBLIC include)

# La version vient du project() racine : une seule source de vérité.
target_compile_definitions(levain_core PRIVATE LEVAIN_VERSION="${PROJECT_VERSION}")
