# Les plugins de Levain (ADR-0018) : un module flecs dans sa propre cible, dont les dépendances sont
# déclarées une fois pour toutes.
#
#   levain_add_plugin(terrain
#       SOURCES src/terrain.cpp
#       DEPENDS levain::render levain::scene)
#
# crée la cible `terrain`, avec son alias `plugin::terrain`, ses en-têtes dans `include/`, et ses
# dépendances liées en PUBLIC.
#
# Deux règles sont vérifiées à la fin de la configuration, sur tout le projet, jeu compris :
#   1. un plugin ne lie que ce qu'il a déclaré dans DEPENDS ;
#   2. une cible du moteur (sous `engine/`) ne lie jamais un plugin.
# Une règle enfreinte fait échouer la configuration (règle n°7) : rien ne se construit.

include_guard(GLOBAL)

# Le dossier du moteur : ses modules sont les sous-dossiers `engine/<module>` qu'il a ajoutés. Une
# propriété globale et non une variable : le contrôle s'exécute dans le dossier du jeu, qui ne voit
# pas les variables du moteur.
set_property(GLOBAL PROPERTY LEVAIN_ROOT_DIR "${PROJECT_SOURCE_DIR}")

function(levain_add_plugin name)
    cmake_parse_arguments(PARSE_ARGV 1 plugin "" "" "SOURCES;DEPENDS")
    if(NOT plugin_SOURCES)
        message(FATAL_ERROR "levain_add_plugin(${name}) : SOURCES est vide")
    endif()

    add_library(${name} STATIC ${plugin_SOURCES})
    add_library(plugin::${name} ALIAS ${name})
    target_include_directories(${name} PUBLIC include)
    target_link_libraries(${name} PUBLIC ${plugin_DEPENDS})
    set_target_properties(${name} PROPERTIES
        LEVAIN_PLUGIN TRUE
        LEVAIN_PLUGIN_DEPENDS "${plugin_DEPENDS}")
    set_property(GLOBAL APPEND PROPERTY LEVAIN_PLUGINS ${name})

    # Le contrôle se lance une seule fois, à la fin du CMakeLists.txt principal : c'est seulement
    # là que toutes les cibles, celles du jeu comprises, ont leurs dépendances.
    get_property(scheduled GLOBAL PROPERTY LEVAIN_PLUGIN_CHECK_SCHEDULED)
    if(NOT scheduled)
        set_property(GLOBAL PROPERTY LEVAIN_PLUGIN_CHECK_SCHEDULED TRUE)
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
                       CALL levain_check_plugin_boundaries)
    endif()
endfunction()

# Les cibles déclarées dans `directory` et tous ses sous-dossiers.
function(levain_targets_under directory out)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        levain_targets_under("${child}" childTargets)
        list(APPEND targets ${childTargets})
    endforeach()
    set(${out} ${targets} PARENT_SCOPE)
endfunction()

# Le nom réel d'une cible, alias résolu ; vide si `item` n'est pas une cible (une option de lien,
# une expression génératrice).
function(levain_resolved_target item out)
    set(resolved "")
    if(TARGET "${item}")
        get_target_property(aliased "${item}" ALIASED_TARGET)
        if(aliased)
            set(resolved "${aliased}")
        else()
            set(resolved "${item}")
        endif()
    endif()
    set(${out} "${resolved}" PARENT_SCOPE)
endfunction()

function(levain_check_plugin_boundaries)
    set(failures "")

    # 1. Un plugin ne lie que ce qu'il a déclaré.
    get_property(plugins GLOBAL PROPERTY LEVAIN_PLUGINS)
    foreach(plugin IN LISTS plugins)
        # get_property rend une liste vide pour une propriété absente, là où get_target_property
        # rendrait « linked-NOTFOUND », pris ici pour une dépendance.
        get_property(declared TARGET ${plugin} PROPERTY LEVAIN_PLUGIN_DEPENDS)
        get_property(linked TARGET ${plugin} PROPERTY LINK_LIBRARIES)
        foreach(item IN LISTS linked)
            if(NOT item IN_LIST declared)
                list(APPEND failures
                     "le plugin ${plugin} lie ${item} sans l'avoir déclaré dans DEPENDS")
            endif()
        endforeach()
    endforeach()

    # 2. Le moteur ne dépend jamais d'un plugin.
    get_property(root GLOBAL PROPERTY LEVAIN_ROOT_DIR)
    get_property(rootChildren DIRECTORY "${root}" PROPERTY SUBDIRECTORIES)
    set(engineTargets "")
    foreach(child IN LISTS rootChildren)
        # Un préfixe, pas une expression régulière : le chemin peut contenir « + » ou « . ».
        string(FIND "${child}" "${root}/engine/" position)
        if(position EQUAL 0)
            levain_targets_under("${child}" moduleTargets)
            list(APPEND engineTargets ${moduleTargets})
        endif()
    endforeach()
    if(NOT engineTargets)
        message(FATAL_ERROR "aucune cible sous ${root}/engine : le contrôle ne vérifierait rien")
    endif()
    foreach(target IN LISTS engineTargets)
        get_property(linked TARGET ${target} PROPERTY LINK_LIBRARIES)
        get_property(interface TARGET ${target} PROPERTY INTERFACE_LINK_LIBRARIES)
        foreach(item IN LISTS linked interface)
            levain_resolved_target("${item}" resolved)
            if(resolved AND resolved IN_LIST plugins)
                list(APPEND failures
                     "la cible du moteur ${target} lie le plugin ${resolved} (ADR-0018)")
            endif()
        endforeach()
    endforeach()

    if(failures)
        list(JOIN failures "\n  " report)
        message(FATAL_ERROR "Frontière des plugins enfreinte :\n  ${report}")
    endif()
endfunction()
