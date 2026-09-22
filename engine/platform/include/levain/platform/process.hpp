#pragma once

#include <span>
#include <string>

#include "levain/core/error.hpp"

namespace levain::platform
{

struct ProcessOutput
{
    int exitCode = 0;
    std::string output; ///< Sortie standard et sortie d'erreur, mêlées dans l'ordre d'écriture.
};

/// Lance `arguments[0]` avec les arguments suivants, attend sa fin et rend sa sortie. Bloquant.
/// Un programme introuvable est un échec récupérable (ADR-0008) ; un programme qui échoue rend un
/// `exitCode` non nul, que l'appelant interprète.
[[nodiscard]] core::Result<ProcessOutput> runProcess(std::span<const std::string> arguments);

} // namespace levain::platform
