find_package(doctest CONFIG REQUIRED)

add_executable(levain_tests
    allocator_test.cpp
    assert_test.cpp
    doctest_main.cpp
    error_test.cpp
    frame_time_test.cpp
    log_test.cpp
    version_test.cpp)
target_link_libraries(levain_tests PRIVATE levain::core doctest::doctest)

# La macro qui génère le main de doctest ne doit s'appliquer qu'à une seule unité de
# compilation, sinon chaque fichier de test en produirait un. On la pose donc sur ce fichier
# précis plutôt que sur la cible entière, et depuis le build plutôt que depuis les sources.
set_source_files_properties(doctest_main.cpp
    PROPERTIES COMPILE_DEFINITIONS DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

# La version attendue vient du project() racine, pas d'une constante recopiée : le test
# échoue si le CMakeLists et le binaire divergent.
target_compile_definitions(levain_tests PRIVATE LEVAIN_EXPECTED_VERSION="${PROJECT_VERSION}")

# doctest_discover_tests lance le binaire à la compilation pour lister ses cas, et en
# déclare un par TEST_CASE à ctest. Une régression pointe donc le cas précis plutôt qu'un
# « levain_tests a échoué ». Fourni par le port vcpkg de doctest.
include(doctest)
doctest_discover_tests(levain_tests)

# Benchmark : compilé par la CI pour qu'il ne pourrisse pas, mais pas lancé par ctest.
# Un chiffre de performance dépend de la machine ; il se mesure à la main sur la machine
# de référence et se consigne dans le journal (AGENTS.md, règle non négociable n°6).
add_executable(levain_bench allocator_bench.cpp)
target_link_libraries(levain_bench PRIVATE levain::core)
