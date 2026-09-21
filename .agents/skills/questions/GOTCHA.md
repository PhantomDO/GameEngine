# Pièges — questions et comparaisons

Un piège par entrée : symptôme, cause, parade. Le plus récent en haut.

## Comparer la meilleure version de chaque option (2026-09-20)

- **Symptôme** : le projet est passé à Rust (ADR-0010) puis revenu au C++ le jour même (ADR-0011).
- **Cause** : la comparaison penchait. Le C++ montré n'était pas le meilleur possible (la glu flecs dans la
  logique, là où bevy_ecs met les dépendances dans la signature), et le C++ payait un Windows hors périmètre.
- **Parade** : avant de conclure qu'une option est plus lisible ou plus simple, écrire la meilleure version de
  l'autre, sur le même périmètre. Dire explicitement ce qui vient du langage et ce qui vient de la bibliothèque.

## « C'est stable en 2026 » se vérifie (2026-09-20)

- **Symptôme** : Donnovan supposait C++23 « stable » partout. MSVC n'a pas de `/std:c++23`, seulement
  `/std:c++23preview` et `/std:c++latest`.
- **Parade** : vérifier sur la chaîne réelle (sonde de macros de fonctionnalités, `-pedantic-errors`) avant de
  confirmer, et dire ce qui manque.
