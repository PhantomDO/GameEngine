#pragma once

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace levain::core
{

enum class LogLevel : std::uint8_t
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

/// Le niveau en dessous duquel une catégorie ne journalise plus rien.
void setLogLevel(std::string_view category, LogLevel level);

/// Vrai si un message de ce niveau serait effectivement écrit.
[[nodiscard]] bool isLogEnabled(std::string_view category, LogLevel level);

/// Écrit un message déjà formaté. Point d'entrée non générique : c'est lui qui permet de
/// garder les en-têtes de spdlog hors de `core` (voir l'invariant n°1 du README du module).
void logMessage(std::string_view category, LogLevel level, std::string_view message);

/// Journalise dans une catégorie. Une catégorie par module : on peut faire taire `assets`
/// sans rien changer à `gpu`. Équivalent des `DECLARE_LOG_CATEGORY_EXTERN` d'Unreal et des
/// canaux de `print_verbose` de Godot.
///
/// Le test de niveau précède le formatage : sans lui, un `LEVAIN_LOG(Trace, …)` dans une
/// boucle de rendu paierait le `std::format` à chaque frame pour un message jeté ensuite.
template <typename... Args>
void log(std::string_view category, LogLevel level, std::format_string<Args...> fmt, Args&&... args)
{
    if (!isLogEnabled(category, level))
    {
        return;
    }

    logMessage(category, level, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace levain::core
