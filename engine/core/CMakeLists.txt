find_package(spdlog CONFIG REQUIRED)

add_library(levain_core STATIC
    src/assert.cpp
    src/error.cpp
    src/log.cpp
    src/version.cpp)
add_library(levain::core ALIAS levain_core)

target_include_directories(levain_core PUBLIC include)

# spdlog est PRIVATE : ses en-têtes ne fuient pas dans l'API de core, et les modules qui
# lient levain::core ne le voient pas. C'est ce qui fait tenir l'invariant n°1 du README.
target_link_libraries(levain_core PRIVATE spdlog::spdlog)

# La version vient du project() racine : une seule source de vérité.
target_compile_definitions(levain_core PRIVATE LEVAIN_VERSION="${PROJECT_VERSION}")

# Assertions actives en Debug uniquement. PUBLIC : la macro est compilée chez l'appelant,
# donc la définition doit le suivre, sinon sandbox et tests la verraient désactivée.
target_compile_definitions(levain_core PUBLIC
    $<$<CONFIG:Debug>:LEVAIN_ASSERTIONS_ENABLED=1>)
