# Vérifie que CheckVcpkgManifest.cmake accepte une copie fidèle du manifeste du moteur, et refuse
# chaque écart. Lancé par ctest : cmake -DROOT=<dépôt> -DWORK=<dossier> -P check_vcpkg_manifest_test.cmake

cmake_minimum_required(VERSION 3.28) # un script n'a sinon aucune politique

set(check "${ROOT}/cmake/CheckVcpkgManifest.cmake")

# Un « jeu » neuf dans WORK, copie du manifeste et des ports du moteur.
function(fresh_game)
    file(REMOVE_RECURSE "${WORK}")
    file(MAKE_DIRECTORY "${WORK}")
    file(COPY "${ROOT}/vcpkg.json" "${ROOT}/ports" DESTINATION "${WORK}")
endfunction()

function(expect outcome scenario)
    execute_process(COMMAND "${CMAKE_COMMAND}" -DENGINE_DIR=${ROOT} -DGAME_DIR=${WORK} -P "${check}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
    if(outcome STREQUAL "accepte" AND NOT result EQUAL 0)
        message(SEND_ERROR "${scenario} : refusé à tort\n${output}")
    elseif(outcome STREQUAL "refuse" AND result EQUAL 0)
        message(SEND_ERROR "${scenario} : accepté à tort")
    endif()
endfunction()

fresh_game()
expect(accepte "copie fidèle")

# Une autre mise en forme du même JSON, et une dépendance en plus : le jeu a les siennes.
file(READ "${WORK}/vcpkg.json" manifest)
string(JSON manifest SET "${manifest}" dependencies 999 "\"jolt-physics\"")
file(WRITE "${WORK}/vcpkg.json" "${manifest}")
expect(accepte "mise en forme différente et dépendance en plus")

fresh_game()
file(READ "${WORK}/vcpkg.json" manifest)
string(JSON manifest SET "${manifest}" builtin-baseline "\"0000000000000000000000000000000000000000\"")
file(WRITE "${WORK}/vcpkg.json" "${manifest}")
expect(refuse "autre baseline")

fresh_game()
file(READ "${WORK}/vcpkg.json" manifest)
string(JSON manifest REMOVE "${manifest}" dependencies 0)
file(WRITE "${WORK}/vcpkg.json" "${manifest}")
expect(refuse "dépendance manquante")

fresh_game()
file(GLOB_RECURSE portFiles "${WORK}/ports/*")
list(GET portFiles 0 firstPortFile)
file(APPEND "${firstPortFile}" "\n# modifié\n")
expect(refuse "port modifié")
