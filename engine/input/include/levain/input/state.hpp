#pragma once

#include <array>
#include <bitset>
#include <span>
#include <vector>

#include "levain/input/bindings.hpp"

namespace levain::input
{

/// L'état brut des appareils : ce qui est tenu, où en sont les sticks, et de combien la souris a
/// bougé pendant cette image.
struct RawInput
{
    std::bitset<platform::KeyCodeCount> keys;
    std::bitset<platform::MouseButtonCount> mouseButtons;
    std::bitset<platform::PadButtonCount> padButtons;
    std::array<float, platform::PadAxisCount> padAxes{};  ///< Position, entre -1 et 1.
    std::array<float, platform::MouseAxisCount> motion{}; ///< Déplacement de l'image, en pixels.
};

/// Range un événement brut dans l'état. Sans SDL, sans fichier : elle se teste seule.
void applyEvent(RawInput& raw, const platform::InputEvent& event);

/// Ce que le jeu lit : une valeur par action et par axe, et l'état de l'image précédente pour
/// distinguer « tenu » de « vient d'être appuyé ».
struct InputState
{
    std::vector<bool> actionsHeld;
    std::vector<bool> actionsHeldPreviously;
    std::vector<float> axes;
    RawInput raw;
};

/// Prépare l'état pour ces liaisons : une case par action et par axe, toutes au repos.
[[nodiscard]] InputState makeInputState(const Bindings& bindings);

/// Avance d'une image : range les événements, puis calcule les actions et les axes.
///
/// `frameSeconds` sert à une seule chose, mais elle est importante : **un axe est une vitesse**,
/// que le jeu multipliera par la durée de son pas. Un stick donne déjà une vitesse ; la souris,
/// elle, donne un déplacement, divisé ici par la durée de l'image. Sans cette division, tourner la
/// caméra à la souris irait deux fois plus vite à 120 images/s qu'à 60.
void updateInput(InputState& state, const Bindings& bindings,
                 std::span<const platform::InputEvent> events, float frameSeconds);

/// L'action est-elle tenue maintenant ? `actionPressed` ne dit oui que sur **l'image de l'appui** :
/// c'est ce qu'il faut pour sauter ou ouvrir un menu, là où `actionHeld` sert à courir ou viser.
[[nodiscard]] bool actionHeld(const InputState& state, int action);
[[nodiscard]] bool actionPressed(const InputState& state, int action);
[[nodiscard]] float axisValue(const InputState& state, int axis);

/// Un stick au repos n'est jamais exactement à zéro. En deçà de `deadzone`, on rend 0 ; au-delà, on
/// réétale la plage restante sur [0, 1], pour qu'il n'y ait pas de saut au franchissement du seuil.
[[nodiscard]] float applyDeadzone(float value, float deadzone);

/// La valeur d'une source, échelle comprise : 0 ou 1 pour un bouton, la position pour un stick, la
/// vitesse pour la souris.
[[nodiscard]] float sourceValue(const RawInput& raw, const Source& source, float deadzone,
                                float frameSeconds);

/// Au-delà de cette valeur absolue, une source continue (une gâchette) compte comme un appui.
inline constexpr float ActionThreshold = 0.5f;

} // namespace levain::input
