#include <array>
#include <string>

#include <doctest/doctest.h>

#include "levain/platform/process.hpp"

using levain::platform::runProcess;

// `cmake -E` fournit des commandes portables : le test tourne partout où le projet se compile.

TEST_CASE("runProcess rend la sortie et le code de retour d'un programme")
{
    const std::array<std::string, 4> command{LEVAIN_CMAKE_COMMAND, "-E", "echo", "bonjour"};
    const auto result = runProcess(command);

    REQUIRE(result.has_value());
    CHECK(result->exitCode == 0);
    CHECK(result->output == "bonjour\n");
}

TEST_CASE("runProcess rend le code d'échec d'un programme qui échoue")
{
    const std::array<std::string, 3> command{LEVAIN_CMAKE_COMMAND, "-E", "false"};
    const auto result = runProcess(command);

    REQUIRE(result.has_value());
    CHECK(result->exitCode != 0);
}

TEST_CASE("runProcess signale un programme introuvable comme un échec récupérable")
{
    const std::array<std::string, 1> command{"/chemin/qui/n/existe/pas"};
    const auto result = runProcess(command);

    // Selon la plateforme, l'échec vient du lancement lui-même ou du code de retour du
    // processus enfant, qui n'a pas pu exécuter le programme.
    CHECK((!result.has_value() || result->exitCode != 0));
}
