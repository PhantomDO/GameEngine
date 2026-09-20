#include <doctest/doctest.h>

#include "levain/core/version.hpp"

// Ce test vérifie le câblage du build autant que le code : LEVAIN_VERSION est injecté par
// CMake depuis le project() racine, et une faute de frappe dans le CMakeLists donnerait une
// chaîne vide sans que rien ne casse à la compilation.
TEST_CASE("version() renvoie la version du project() racine")
{
    const auto version = levain::core::version();

    CHECK_FALSE(version.empty());
    CHECK(version == LEVAIN_EXPECTED_VERSION);
}

TEST_CASE("toolchain() identifie un compilateur connu")
{
    const auto toolchain = levain::core::toolchain();

    CHECK_FALSE(toolchain.empty());
    CHECK(toolchain != "compilateur inconnu");
}
