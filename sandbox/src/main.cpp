#include <cstdio>
#include <exception>
#include <print>

#include "levain/core/version.hpp"

int main()
{
    // std::print peut lever : format_error sur une chaîne de format invalide, system_error
    // si l'écriture échoue. Une exception qui s'échappe de main appelle std::terminate, donc
    // on la rattrape ici. La politique générale du moteur — exceptions ou codes de retour —
    // se décide en M0.3 (ADR-0008) ; en attendant, main ne laisse rien passer.
    try
    {
        // std::print est le témoin de la bibliothèque standard C++23 : si une plateforme de
        // la matrice ne la fournit pas, le build casse ici, à la première ligne de code du
        // projet, plutôt qu'au milieu du renderer trois mois plus tard (ADR-0001).
        std::print("Levain {} — {} — __cplusplus {}\n", levain::core::version(),
                   levain::core::toolchain(), __cplusplus);
    }
    catch (const std::exception& e)
    {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    }

    return 0;
}
