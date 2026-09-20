#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace levain::core
{

/// Catégorie d'un échec **récupérable** : une cause extérieure au moteur, pas un bug.
/// Fichier absent, shader qui ne compile pas, device GPU perdu. Un bug du moteur relève
/// de `LEVAIN_ASSERT` (voir `assert.hpp`), pas d'un code d'erreur.
///
/// Volontairement courte : on l'étend quand un cas concret se présente, pas avant.
enum class ErrorCode : std::uint8_t
{
    FileNotFound,
    InvalidData,
    OutOfMemory,
    Unsupported,
};

/// Un échec, avec de quoi le comprendre sans ouvrir le débogueur.
struct Error
{
    ErrorCode code;
    std::string message; ///< Contexte lisible : « shaders/tri.slang:12 : symbole inconnu ».
};

/// Résultat d'une opération qui peut échouer pour une raison qui n'est pas un bug.
///
/// `std::expected` est la raison pour laquelle le projet est passé à C++23 (ADR-0001) :
/// il porte la valeur **ou** l'erreur dans le type de retour, donc un appelant ne peut pas
/// ignorer l'échec par distraction, et il n'y a ni exception ni code de retour nu.
template <typename T> using Result = std::expected<T, Error>;

/// Fabrique l'échec à renvoyer depuis une fonction qui rend un `Result`.
[[nodiscard]] std::unexpected<Error> makeError(ErrorCode code, std::string message);

/// Nom lisible d'un code, pour les logs et les messages d'erreur.
[[nodiscard]] std::string_view describe(ErrorCode code);

} // namespace levain::core
