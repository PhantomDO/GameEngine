#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "levain/core/error.hpp"
#include "levain/input/bindings.hpp"

using levain::input::actionIndex;
using levain::input::axisIndex;
using levain::input::Bindings;
using levain::input::parseBindings;

namespace
{

/// Le fichier de liaisons de référence des tests : une action au clavier **et** à la manette, et un
/// axe composé de deux touches.
constexpr std::string_view SampleBindings = R"(
# un commentaire, puis une ligne vide

action jump       = key:Space, pad:a
axis   move_right = key:D, key:A:-1, pad:leftx
axis   look_right = mouse:x:0.5
deadzone = 0.2
)";

Bindings sample()
{
    auto bindings = parseBindings(SampleBindings);
    REQUIRE(bindings.has_value());
    return *bindings;
}

} // namespace

TEST_CASE("un fichier de liaisons se lit en actions et en axes")
{
    const Bindings bindings = sample();

    REQUIRE(bindings.actions.size() == 1);
    CHECK(bindings.actions[0].name == "jump");
    CHECK(bindings.actions[0].sources.size() == 2); // clavier et manette, la même action
    REQUIRE(bindings.axes.size() == 2);
    CHECK(bindings.axes[0].sources[1].scale == doctest::Approx(-1.0f)); // key:A:-1
    CHECK(bindings.axes[0].sources[2].isAxis);                          // pad:leftx
    CHECK(bindings.deadzone == doctest::Approx(0.2f));
}

TEST_CASE("un nom inconnu de SDL échoue, avec son numéro de ligne")
{
    const auto bindings = parseBindings("action jump = key:Spacee\n");

    REQUIRE_FALSE(bindings.has_value());
    CHECK(bindings.error().code == levain::core::ErrorCode::InvalidData);
    // « south » est le nom de l'énumération de SDL, pas celui de sa table : le piège doit être
    // signalé, pas avalé.
    CHECK(std::string_view{bindings.error().message}.find("ligne 1") != std::string_view::npos);
    CHECK_FALSE(parseBindings("action jump = pad:south\n").has_value());
    CHECK_FALSE(parseBindings("action jump\n").has_value());            // pas de « = »
    CHECK_FALSE(parseBindings("saute jump = key:Space\n").has_value()); // ni action ni axis
}

TEST_CASE("le fichier de liaisons livré avec la démo est valide")
{
    // Sans ce test, une faute de frappe dans data/input.cfg ne se verrait qu'au lancement du
    // sandbox — et seulement si on pense à essayer la touche concernée.
    const auto bindings =
        levain::input::loadBindings(std::filesystem::path{LEVAIN_DATA_DIR} / "input.cfg");

    INFO("message d'erreur : " << (bindings ? std::string{} : bindings.error().message));
    REQUIRE(bindings.has_value());
    CHECK(axisIndex(bindings.value(), "move_forward").has_value());
    CHECK(axisIndex(bindings.value(), "look_right").has_value());
    CHECK(actionIndex(bindings.value(), "look_enable").has_value());
}
