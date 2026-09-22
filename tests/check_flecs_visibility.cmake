# SPECS §7 : flecs est l'API du modèle objet, visible dans scene/ et au-dessus, jamais en dessous.
# render/ n'est pas au-dessus de scene/ : les deux sont sur des branches séparées du graphe, et c'est
# l'application qui fait le lien. Lancé par ctest : cmake -DROOT=<dépôt> -P check_flecs_visibility.cmake
#
# Le contrôle échoue bruyamment (règle n°7) : un dossier absent est une erreur, pas un succès muet.
set(failures 0)
foreach(module core platform gpu render)
    set(directory "${ROOT}/engine/${module}")
    if(NOT IS_DIRECTORY "${directory}")
        message(FATAL_ERROR "${directory} introuvable : le contrôle ne vérifierait rien")
    endif()
    file(GLOB_RECURSE files "${directory}/*.cpp" "${directory}/*.hpp" "${directory}/CMakeLists.txt")
    foreach(file IN LISTS files)
        file(STRINGS "${file}" hits REGEX "#include <flecs|flecs::")
        if(hits)
            message(SEND_ERROR "${file} utilise flecs, interdit sous scene/ (SPECS §7) : ${hits}")
            math(EXPR failures "${failures} + 1")
        endif()
    endforeach()
endforeach()
if(failures EQUAL 0)
    message(STATUS "flecs absent de core/, platform/, gpu/ et render/")
endif()
