cmake_minimum_required(VERSION 3.28)
project(Levain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Garde-fou de conformité C++23 (ADR-0001, périmètre revu par ADR-0011).
#
# -pedantic-errors transforme toute extension post-C++23 en erreur de compilation : du
# code C++26 écrit par distraction casse le build au lieu de passer en douce.
#
# Le projet cible Linux et Clang uniquement (ADR-0011). Le bloc MSVC de la v0.3 — et son
# /Zc:__cplusplus, sans lequel MSVC prétend être en C++98 — est retiré. À remettre le jour
# où Windows redevient une cible, avec la CI qui va avec.
add_compile_options(-pedantic-errors)

# ADR-0001 dit « sans modules ». CMake active pourtant le scan de modules dès C++20 avec
# Ninja : deux étapes par fichier (scan + dyndep) pour un résultat toujours vide. On le coupe.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

enable_testing()

add_subdirectory(engine/core)
add_subdirectory(sandbox)
add_subdirectory(tests)
