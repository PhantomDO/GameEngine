#pragma once

#include <string_view>

namespace levain::core {

/// Version du moteur, telle que déclarée par project() dans le CMakeLists racine.
[[nodiscard]] std::string_view version() noexcept;

/// Nom et version du compilateur qui a produit ce binaire, et valeur de __cplusplus.
/// Sert à vérifier sur chaque plateforme de la matrice que la chaîne est bien celle
/// qu'on croit : MSVC compile en /std:c++latest, pas en C++23 strict (ADR-0001).
[[nodiscard]] std::string_view toolchain() noexcept;

}  // namespace levain::core
