#include "levain/core/version.hpp"

namespace levain::core
{

std::string_view version() noexcept
{
    return LEVAIN_VERSION;
}

std::string_view toolchain() noexcept
{
#if defined(__clang__)
    return "clang " __clang_version__;
#elif defined(_MSC_VER)
    // _MSC_VER n'est pas stringifiable directement : _MSC_FULL_VER le serait, mais la
    // version lisible suffit ici. La CI Windows affiche la version exacte de toute façon.
    return "MSVC";
#elif defined(__GNUC__)
    return "gcc " __VERSION__;
#else
    return "compilateur inconnu";
#endif
}

} // namespace levain::core
