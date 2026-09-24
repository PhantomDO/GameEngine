# SPECS §7 : fastgltf n'est inclus que dans engine/assets/src ; le reste du moteur ne voit que les
# types de levain/assets/gltf.hpp. Lancé par ctest : cmake -DROOT=<dépôt> -P check_fastgltf_visibility.cmake
#
# Le contrôle échoue bruyamment (règle n°7) : sans aucun fichier à lire, il ne vérifierait rien.
file(GLOB_RECURSE files "${ROOT}/engine/*.cpp" "${ROOT}/engine/*.hpp" "${ROOT}/sandbox/*.cpp"
     "${ROOT}/tests/*.cpp")
if(NOT files)
    message(FATAL_ERROR "aucune source sous ${ROOT} : le contrôle ne vérifierait rien")
endif()
set(failures 0)
foreach(file IN LISTS files)
    string(FIND "${file}" "${ROOT}/engine/assets/src/" position)
    if(position EQUAL 0)
        continue()
    endif()
    file(STRINGS "${file}" hits REGEX "#include <fastgltf")
    if(hits)
        message(SEND_ERROR "${file} inclut fastgltf, réservé à engine/assets/src (SPECS §7)")
        math(EXPR failures "${failures} + 1")
    endif()
endforeach()
if(failures EQUAL 0)
    message(STATUS "fastgltf absent hors de engine/assets/src")
endif()
