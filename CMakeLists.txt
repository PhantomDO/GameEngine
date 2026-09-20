cmake_minimum_required(VERSION 3.28)
project(Levain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Garde-fou de conformité C++23 (ADR-0001).
#
# MSVC n'expose pas de /std:c++23 : CMake y retombe sur /std:c++latest, qui active aussi
# des fonctionnalités du brouillon C++26. Les deux moitiés de la matrice ne compilent donc
# pas le même langage, et Windows est le plus permissif des deux.
#
# Clang et GCC font autorité : -pedantic-errors transforme toute extension post-C++23 en
# erreur de compilation, ce qui fait échouer la CI Linux avant que la dérive n'atteigne
# main. Ne pas retirer sans rouvrir l'ADR-0001.
if(NOT MSVC)
  add_compile_options(-pedantic-errors)
endif()

# ADR-0001 dit « sans modules ». CMake active pourtant le scan de modules dès C++20 avec
# Ninja : deux étapes par fichier (scan + dyndep) pour un résultat toujours vide. On le coupe.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_subdirectory(engine/core)
add_subdirectory(sandbox)
