#include "levain/core/version.hpp"

// Les macros de version des compilateurs sont numériques : il faut deux niveaux
// d'indirection pour que l'argument soit développé avant d'être transformé en chaîne.
#define LEVAIN_STRINGIFY_IMPL(x) #x
#define LEVAIN_STRINGIFY(x) LEVAIN_STRINGIFY_IMPL(x)

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
    return "MSVC " LEVAIN_STRINGIFY(_MSC_FULL_VER);
#elif defined(__GNUC__)
    return "gcc " __VERSION__;
#else
    return "compilateur inconnu";
#endif
}

} // namespace levain::core
