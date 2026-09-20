#include "levain/core/log.hpp"

#include <map>
#include <mutex>
#include <string>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace levain::core
{
namespace
{

spdlog::level::level_enum toSpdlog(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:
        return spdlog::level::trace;
    case LogLevel::Debug:
        return spdlog::level::debug;
    case LogLevel::Info:
        return spdlog::level::info;
    case LogLevel::Warning:
        return spdlog::level::warn;
    case LogLevel::Error:
        return spdlog::level::err;
    case LogLevel::Critical:
        return spdlog::level::critical;
    }

    return spdlog::level::info;
}

/// Table des catégories, créées à la première utilisation.
///
/// Une recherche par chaîne à chaque appel : c'est le point à surveiller si une capture
/// Tracy montre le log dans le profil (M0.3, issue #8). La réponse sera alors un handle de
/// catégorie obtenu une fois, pas une optimisation de cette table.
std::shared_ptr<spdlog::logger> categoryLogger(std::string_view category)
{
    static std::mutex mutex;
    static std::map<std::string, std::shared_ptr<spdlog::logger>, std::less<>> loggers;

    const std::scoped_lock lock{mutex};

    if (const auto it = loggers.find(category); it != loggers.end())
    {
        return it->second;
    }

    auto logger = spdlog::stdout_color_mt(std::string{category});
    logger->set_pattern("[%T] [%^%l%$] [%n] %v");
    loggers.emplace(category, logger);

    return logger;
}

} // namespace

void setLogLevel(std::string_view category, LogLevel level)
{
    categoryLogger(category)->set_level(toSpdlog(level));
}

bool isLogEnabled(std::string_view category, LogLevel level)
{
    return categoryLogger(category)->should_log(toSpdlog(level));
}

void logMessage(std::string_view category, LogLevel level, std::string_view message)
{
    categoryLogger(category)->log(toSpdlog(level), message);
}

} // namespace levain::core
