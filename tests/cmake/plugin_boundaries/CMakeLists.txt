# Un moteur miniature, pour vérifier que levain_add_plugin fait échouer la configuration quand la
# frontière de l'ADR-0018 est enfreinte. CASE choisit le scénario (tests/CMakeLists.txt).
cmake_minimum_required(VERSION 3.28)
project(PluginBoundaries LANGUAGES CXX)

include("${LEVAIN_CMAKE_DIR}/LevainPlugin.cmake")

add_subdirectory(engine/core)

# Deux plugins, dont l'un dépend de l'autre en le déclarant : c'est permis.
levain_add_plugin(terrain SOURCES empty.cxx DEPENDS fixture_core)
levain_add_plugin(grass SOURCES empty.cxx DEPENDS plugin::terrain)

if(CASE STREQUAL "engine-links-plugin")
    target_link_libraries(fixture_core PRIVATE plugin::terrain)
elseif(CASE STREQUAL "undeclared-dependency")
    target_link_libraries(grass PUBLIC fixture_core)
elseif(NOT CASE STREQUAL "allowed")
    message(FATAL_ERROR "CASE inconnu : « ${CASE} »")
endif()
