#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include "levain/core/error.hpp"
#include "levain/input/bindings.hpp"
#include "levain/input/state.hpp"

using levain::input::actionHeld;
using levain::input::actionIndex;
using levain::input::actionPressed;
using levain::input::axisIndex;
using levain::input::axisValue;
using levain::input::Bindings;
using levain::input::InputState;
using levain::input::makeInputState;
using levain::input::parseBindings;
using levain::input::updateInput;
using levain::platform::InputDevice;
using levain::platform::InputEvent;
using levain::platform::InputEventType;

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

InputEvent key(std::uint16_t code, bool down)
{
    return {.type = down ? InputEventType::ButtonDown : InputEventType::ButtonUp,
            .device = InputDevice::Keyboard,
            .code = code};
}

InputEvent padAxis(std::uint16_t code, float value)
{
    return {.type = InputEventType::AxisMotion,
            .device = InputDevice::Gamepad,
            .code = code,
            .value = value};
}

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

TEST_CASE("la même action répond au clavier et à la manette")
{
    const Bindings bindings = sample();
    const std::optional<int> found = actionIndex(bindings, "jump");
    REQUIRE(found.has_value());
    const int jump = found.value_or(-1);
    InputState state = makeInputState(bindings);

    const InputEvent space = key(44, true); // « Space », vérifié par SDL dans platform
    updateInput(state, bindings, {&space, 1}, 1.0f / 60.0f);
    CHECK(actionHeld(state, jump));
    CHECK(actionPressed(state, jump)); // l'image de l'appui

    updateInput(state, bindings, {}, 1.0f / 60.0f);
    CHECK(actionHeld(state, jump));          // toujours tenue
    CHECK_FALSE(actionPressed(state, jump)); // mais plus « appuyée cette image »

    const InputEvent release = key(44, false);
    updateInput(state, bindings, {&release, 1}, 1.0f / 60.0f);
    CHECK_FALSE(actionHeld(state, jump));

    // Et maintenant la manette, sans que rien d'autre ne change : c'est le critère de M3.4.
    const InputEvent padButton{
        .type = InputEventType::ButtonDown, .device = InputDevice::Gamepad, .code = 0}; // « a »
    updateInput(state, bindings, {&padButton, 1}, 1.0f / 60.0f);
    CHECK(actionHeld(state, jump));
}

TEST_CASE("deux touches opposées font un axe, et se neutralisent")
{
    const Bindings bindings = sample();
    const std::optional<int> found = axisIndex(bindings, "move_right");
    REQUIRE(found.has_value());
    const int moveRight = found.value_or(-1);
    InputState state = makeInputState(bindings);

    const InputEvent d = key(7, true); // « D »
    updateInput(state, bindings, {&d, 1}, 1.0f / 60.0f);
    CHECK(axisValue(state, moveRight) == doctest::Approx(1.0f));

    const InputEvent a = key(4, true); // « A », d'échelle -1
    updateInput(state, bindings, {&a, 1}, 1.0f / 60.0f);
    CHECK(axisValue(state, moveRight) == doctest::Approx(0.0f));
}

TEST_CASE("la zone morte annule le repos du stick sans faire sauter la valeur")
{
    const Bindings bindings = sample();
    const int moveRight = axisIndex(bindings, "move_right").value_or(-1);
    InputState state = makeInputState(bindings);

    const InputEvent resting = padAxis(0, 0.1f); // sous la zone morte de 0,2
    updateInput(state, bindings, {&resting, 1}, 1.0f / 60.0f);
    CHECK(axisValue(state, moveRight) == doctest::Approx(0.0f));

    const InputEvent pushed = padAxis(0, 0.6f);
    updateInput(state, bindings, {&pushed, 1}, 1.0f / 60.0f);
    CHECK(axisValue(state, moveRight) == doctest::Approx(0.5f)); // (0,6 - 0,2) / 0,8

    const InputEvent full = padAxis(0, 1.0f);
    updateInput(state, bindings, {&full, 1}, 1.0f / 60.0f);
    CHECK(axisValue(state, moveRight) == doctest::Approx(1.0f)); // la butée reste la butée
}

TEST_CASE("le regard à la souris ne dépend pas de la cadence du rendu")
{
    const Bindings bindings = sample();
    const int lookRight = axisIndex(bindings, "look_right").value_or(-1);
    InputState state = makeInputState(bindings);

    // Un axe est une **vitesse** : le jeu le multiplie par la durée de son pas. Dix pixels en une
    // image de 1/60 s ou de 1/120 s doivent donc donner le même angle une fois multipliés.
    const InputEvent tenPixels{.type = InputEventType::AxisMotion,
                               .device = InputDevice::Mouse,
                               .code = 0,
                               .value = 10.0f};

    updateInput(state, bindings, {&tenPixels, 1}, 1.0f / 60.0f);
    const float slow = axisValue(state, lookRight) * (1.0f / 60.0f);

    updateInput(state, bindings, {&tenPixels, 1}, 1.0f / 120.0f);
    const float fast = axisValue(state, lookRight) * (1.0f / 120.0f);

    CHECK(slow == doctest::Approx(fast));
    CHECK(slow == doctest::Approx(10.0f * 0.5f)); // l'échelle du fichier

    // Et le déplacement n'appartient qu'à son image : sans nouvel événement, il retombe à zéro.
    updateInput(state, bindings, {}, 1.0f / 60.0f);
    CHECK(axisValue(state, lookRight) == doctest::Approx(0.0f));
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
