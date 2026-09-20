#include <print>

#include "levain/core/version.hpp"

int main() {
  // std::print est le témoin de la bibliothèque standard C++23 : si une plateforme de la
  // matrice ne la fournit pas, le build casse ici, à la première ligne de code du projet,
  // plutôt qu'au milieu du renderer trois mois plus tard (ADR-0001).
  std::print("Levain {} — {} — __cplusplus {}\n", levain::core::version(),
             levain::core::toolchain(), __cplusplus);
  return 0;
}
