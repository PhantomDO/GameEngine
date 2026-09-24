# SPECS §7 : les bibliothèques d'import et de cuisson, fastgltf et libktx (ADR-0020), ne sont incluses
# que dans engine/assets/src ; le reste du moteur, le cuiseur compris, ne voit que les types
# d'engine/assets. Lancé par ctest : cmake -DROOT=<dépôt> -P check_asset_libraries_visibility.cmake
#
# Le contrôle échoue bruyamment (règle n°7) : sans aucun fichier à lire, il ne vérifierait rien.
file(GLOB_RECURSE files "${ROOT}/engine/*.cpp" "${ROOT}/engine/*.hpp" "${ROOT}/sandbox/*.cpp"
     "${ROOT}/tests/*.cpp" "${ROOT}/tools/*.cpp")
if(NOT files)
    message(FATAL_ERROR "aucune source sous ${ROOT} : le contrôle ne vérifierait rien")
endif()
set(failures 0)
foreach(file IN LISTS files)
    string(FIND "${file}" "${ROOT}/engine/assets/src/" position)
    if(position EQUAL 0)
        continue()
    endif()
    file(STRINGS "${file}" hits REGEX "#include <(fastgltf|ktx)")
    if(hits)
        message(SEND_ERROR "${file} inclut ${hits}, réservé à engine/assets/src (SPECS §7)")
        math(EXPR failures "${failures} + 1")
    endif()
endforeach()
if(failures EQUAL 0)
    message(STATUS "fastgltf et libktx absents hors de engine/assets/src")
endif()
