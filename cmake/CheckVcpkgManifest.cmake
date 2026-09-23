# Le manifeste vcpkg d'un jeu doit contenir celui du moteur (ADR-0018).
#
# vcpkg installe les dépendances du projet principal, au moment de son `project()` : quand le jeu
# récupère le moteur par FetchContent, le vcpkg.json du moteur arrive trop tard et n'est jamais lu. Le
# jeu en recopie donc le contenu, et ce contrôle vérifie la copie :
#   - même `builtin-baseline`, sans quoi les versions des bibliothèques divergent en silence ;
#   - chaque dépendance du moteur, avec ses options, figure dans celles du jeu (qui peut en avoir
#     d'autres) ;
#   - chaque fichier des ports maison du moteur (`ports/`) existe à l'identique chez le jeu.
#
# Deux usages :
#   include(CheckVcpkgManifest) puis levain_check_vcpkg_manifest(<moteur> <jeu>)
#   cmake -DENGINE_DIR=<moteur> -DGAME_DIR=<jeu> -P CheckVcpkgManifest.cmake   (les tests)

function(levain_check_vcpkg_manifest engineDir gameDir)
    foreach(dir IN ITEMS "${engineDir}" "${gameDir}")
        if(NOT EXISTS "${dir}/vcpkg.json")
            message(FATAL_ERROR "${dir}/vcpkg.json introuvable : le contrôle ne vérifierait rien")
        endif()
    endforeach()
    file(READ "${engineDir}/vcpkg.json" engine)
    file(READ "${gameDir}/vcpkg.json" game)
    set(failures "")

    string(JSON engineBaseline GET "${engine}" builtin-baseline)
    string(JSON gameBaseline ERROR_VARIABLE missing GET "${game}" builtin-baseline)
    if(NOT gameBaseline STREQUAL engineBaseline)
        list(APPEND failures "builtin-baseline : « ${engineBaseline} » attendu, « ${gameBaseline} » trouvé")
    endif()

    # string(JSON GET) réécrit chaque élément dans une forme normalisée : deux écritures du même
    # objet, espacées différemment, donnent la même chaîne.
    set(gameDependencies "")
    string(JSON gameCount ERROR_VARIABLE missing LENGTH "${game}" dependencies)
    if(gameCount GREATER 0)
        math(EXPR last "${gameCount} - 1")
        foreach(i RANGE ${last})
            string(JSON dependency GET "${game}" dependencies ${i})
            list(APPEND gameDependencies "${dependency}")
        endforeach()
    endif()
    string(JSON engineCount LENGTH "${engine}" dependencies)
    math(EXPR last "${engineCount} - 1")
    foreach(i RANGE ${last})
        string(JSON dependency GET "${engine}" dependencies ${i})
        if(NOT dependency IN_LIST gameDependencies)
            string(REGEX REPLACE "[\n ]+" " " oneLine "${dependency}")
            list(APPEND failures "dépendance absente ou différente : ${oneLine}")
        endif()
    endforeach()

    file(GLOB_RECURSE portFiles RELATIVE "${engineDir}/ports" "${engineDir}/ports/*")
    foreach(file IN LISTS portFiles)
        if(NOT EXISTS "${gameDir}/ports/${file}")
            list(APPEND failures "port absent : ports/${file}")
            continue()
        endif()
        file(SHA256 "${engineDir}/ports/${file}" engineHash)
        file(SHA256 "${gameDir}/ports/${file}" gameHash)
        if(NOT engineHash STREQUAL gameHash)
            list(APPEND failures "port différent : ports/${file}")
        endif()
    endforeach()

    if(failures)
        list(JOIN failures "\n  " report)
        message(FATAL_ERROR
            "Le manifeste vcpkg du jeu ne contient pas celui du moteur (ADR-0018) :\n  ${report}\n"
            "Recopier dans le jeu la baseline et les dépendances de ${engineDir}/vcpkg.json, et le "
            "dossier ${engineDir}/ports.")
    endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    levain_check_vcpkg_manifest("${ENGINE_DIR}" "${GAME_DIR}")
endif()
