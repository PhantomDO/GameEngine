#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "levain/core/error.hpp"

namespace levain::core
{

/// Lit un fichier entier en mémoire. Un fichier absent ou illisible est un échec récupérable
/// (ADR-0008), pas un bug : c'est au contenu du disque qu'on ne peut pas se fier.
[[nodiscard]] Result<std::vector<std::byte>> readFile(const std::filesystem::path& path);

} // namespace levain::core
