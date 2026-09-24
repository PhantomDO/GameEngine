# ADR-0019 : chaque asset versionné a son .meta, versionné lui aussi. Le moteur le crée au premier
# scan ; ce contrôle attrape celui qu'on a oublié de commiter. En CI, le dépôt ne contient que ce qui
# a été commité : un .meta absent y est un .meta oublié.
#
# Lancé par ctest : cmake -DROOT=<dépôt> -P check_asset_metas.cmake. Les racines d'assets versionnées
# sont listées ici ; tests/data n'en est pas une (les tests copient ses fichiers ailleurs avant de
# les scanner).
cmake_minimum_required(VERSION 3.28) # un script n'a sinon aucune politique

set(roots "${ROOT}/data")
set(checked 0)
set(failures 0)
foreach(root IN LISTS roots)
    file(GLOB_RECURSE files "${root}/*")
    foreach(file IN LISTS files)
        get_filename_component(extension "${file}" LAST_EXT)
        string(TOLOWER "${extension}" extension)
        # Les extensions d'isImportable (engine/assets/src/registry.cpp).
        if(NOT extension MATCHES "^\\.(png|jpg|jpeg|gltf|glb)$")
            continue()
        endif()
        math(EXPR checked "${checked} + 1")
        if(NOT EXISTS "${file}.meta")
            message(SEND_ERROR "${file} n'a pas de .meta : lancer le sandbox pour le créer, puis le "
                               "commiter (ADR-0019)")
            math(EXPR failures "${failures} + 1")
        endif()
    endforeach()
endforeach()

# Aucun asset trouvé : le contrôle ne vérifierait rien (règle n°7).
if(checked EQUAL 0)
    message(FATAL_ERROR "aucun asset sous ${roots} : le contrôle ne vérifierait rien")
endif()
if(failures EQUAL 0)
    message(STATUS "${checked} assets, tous avec leur .meta")
endif()
