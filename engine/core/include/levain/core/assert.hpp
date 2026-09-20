#pragma once

#include <source_location>
#include <string_view>

namespace levain::core
{

/// Journalise une assertion violée : expression, message, fichier, ligne et fonction.
///
/// N'arrête pas le programme : c'est la macro appelante qui déclenche l'arrêt, pour que le
/// débogueur se pose sur la ligne fautive et non au fond de cette fonction.
void reportFailedAssert(std::string_view expression, std::string_view message,
                        const std::source_location& where = std::source_location::current());

} // namespace levain::core

/// Arrêt dans le débogueur. `__builtin_debugtrap` rend la main si aucun débogueur n'est
/// attaché, là où le `__builtin_trap` de GCC termine le processus — d'où les deux branches.
#if defined(__clang__)
#define LEVAIN_DEBUG_BREAK() __builtin_debugtrap()
#elif defined(__GNUC__)
#define LEVAIN_DEBUG_BREAK() __builtin_trap()
#else
#define LEVAIN_DEBUG_BREAK() ((void)0)
#endif

#if !defined(LEVAIN_ASSERTIONS_ENABLED)
#define LEVAIN_ASSERTIONS_ENABLED 0
#endif

#if LEVAIN_ASSERTIONS_ENABLED

/// Vérifie un invariant du moteur, c'est-à-dire **un bug si c'est faux**. Compilée hors du
/// binaire en Release : n'y mettez jamais d'expression qui a un effet, utilisez
/// `LEVAIN_VERIFY` pour ça.
///
/// Pour un échec qui n'est pas un bug — fichier absent, shader invalide — c'est `Result`
/// qu'il faut (voir `error.hpp`), pas une assertion.
#define LEVAIN_ASSERT(expression, message)                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(expression)) [[unlikely]]                                                            \
        {                                                                                          \
            ::levain::core::reportFailedAssert(#expression, (message));                            \
            LEVAIN_DEBUG_BREAK();                                                                  \
        }                                                                                          \
    } while (false)

/// Comme `LEVAIN_ASSERT`, mais **l'expression est toujours évaluée**, y compris en Release.
/// Pour les appels dont on veut vérifier le retour sans perdre l'appel lui-même :
/// `LEVAIN_VERIFY(file.close(), "fermeture du fichier")`. C'est le `verify` d'Unreal.
#define LEVAIN_VERIFY(expression, message) LEVAIN_ASSERT(expression, message)

#else

#define LEVAIN_ASSERT(expression, message) ((void)0)
#define LEVAIN_VERIFY(expression, message) ((void)(expression))

#endif
