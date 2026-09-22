#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "levain/core/error.hpp"
#include "levain/platform/input.hpp"

namespace levain::input
{

/// Une source d'entrée liée à une action ou à un axe : un bouton ou un axe d'un appareil, avec le
/// facteur qui la transforme en valeur de jeu. C'est `scale` qui fait d'une touche un demi-axe
/// (`key:A:-1`) et qui règle la sensibilité de la souris.
struct Source
{
    platform::InputDevice device = platform::InputDevice::Keyboard;
    std::uint16_t code = 0;
    bool isAxis = false; ///< Un axe donne une valeur continue ; un bouton, 0 ou 1.
    float scale = 1.0f;
};

/// Une action ou un axe : un nom que le jeu connaît, et tout ce qui peut le déclencher.
struct Binding
{
    std::string name;
    std::vector<Source> sources;
};

/// Le contenu d'un fichier de liaisons (ADR-0017).
struct Bindings
{
    std::vector<Binding> actions; ///< Oui ou non : sauter, tirer, ouvrir le menu.
    std::vector<Binding> axes;    ///< Valeur continue : avancer, regarder.
    float deadzone = 0.15f;       ///< En deçà, un stick de manette est considéré au repos.
};

/// Lit un fichier de liaisons. Un fichier absent ou mal écrit est un échec récupérable (ADR-0008),
/// avec le numéro de ligne dans le message.
[[nodiscard]] core::Result<Bindings> loadBindings(const std::filesystem::path& path);

/// La même chose depuis le texte déjà lu : c'est la version qui se teste, sans disque.
///
/// Une ligne par liaison, « # » commence un commentaire :
/// ```
/// action jump       = key:Space, pad:a
/// axis   move_right = key:D, key:A:-1, pad:leftx
/// deadzone = 0.15
/// ```
/// Un nom de touche, de bouton ou d'axe inconnu **échoue** : une liaison qui ne répondrait jamais
/// serait pire qu'une erreur au démarrage (règle n°7).
[[nodiscard]] core::Result<Bindings> parseBindings(std::string_view text);

/// L'indice d'une action ou d'un axe, à résoudre **une fois** au chargement : le jeu garde
/// l'indice, jamais le nom, pour ne pas comparer des chaînes à chaque image.
[[nodiscard]] std::optional<int> actionIndex(const Bindings& bindings, std::string_view name);
[[nodiscard]] std::optional<int> axisIndex(const Bindings& bindings, std::string_view name);

} // namespace levain::input
