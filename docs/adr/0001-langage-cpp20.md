# ADR-0001 — Langage : C++20, sans modules

- **Statut** : proposé
- **Date** : 2026-09-20
- **Milestone** : M0.1

## Contexte

Donnovan travaille en C++ depuis ses débuts et se demande si un langage plus récent serait un meilleur choix
pour écrire un moteur en 2026. Le but du projet est de comprendre les moteurs du marché.

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| **C++20** | Langage de tous les moteurs étudiés ; toutes les bibliothèques choisies sont en C ou C++ ; outillage mûr (débogueurs, sanitizers, RenderDoc, Tracy, clang-tidy) ; Donnovan le lit vite | Sécurité mémoire à la charge du développeur ; temps de compilation |
| Rust | Sécurité mémoire ; Cargo ; Bevy montre qu'un moteur sérieux est possible | Le borrow checker résiste aux graphes d'objets et à la mémoire gérée à la main, omniprésents dans un moteur ; Jolt, fastgltf et Slang demandent des bindings ; Donnovan apprendrait Rust au lieu d'apprendre les moteurs |
| Zig | Excellent interop C, build intégré, simplicité | Pas encore en 1.0, les versions cassent encore du code ; les bibliothèques C++ (Jolt, fastgltf) demandent des surcouches C |
| Odin | Pensé pour le jeu vidéo, bindings Vulkan et SDL3 fournis | Écosystème petit ; bibliothèques C++ via surcouches C ; peu de ressources sur l'architecture moteur dans ce langage |
| C | Simplicité, interop universelle | Pas de templates ni de RAII : plus de code pour le même résultat ; Jolt et fastgltf sont en C++ |

## Décision

C++20, compilé avec MSVC sous Windows et Clang sous Linux.

**Sans modules C++20** : CMake les prend en charge, mais le support reste inégal selon les compilateurs, les IDE et
clang-tidy, pour un gain faible à notre échelle. **Pas de C++23** pour l'instant, pour garder un socle commun sûr
entre MSVC, libstdc++ et libc++ ; on réévaluera en cours de route.

## Conséquences

- Risques mémoire : ASan et UBSan activés en CI Linux dès M1.1, validation layers Vulkan en Debug.
- La politique de gestion d'erreurs (exceptions ou codes de retour) fait l'objet d'un ADR en M0.3. La plupart des
  moteurs désactivent les exceptions dans le code moteur.

## Ce que font les autres moteurs

Unreal, Godot, REEngine, Anvil, Frostbite : cœur en C++. Unity : cœur en C++, gameplay en C#. Bevy : Rust.
