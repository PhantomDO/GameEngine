# Levain

Moteur de jeu 3D en C++23 pour Linux, sur NVRHI (Vulkan) et flecs, construit étape par
étape pour faire un jeu et comprendre comment fonctionnent les moteurs du marché (Unreal, Unity, Godot…).

Un levain, c'est ce qu'on nourrit un peu chaque semaine, qui reste vivant entre deux fournées, et à partir de
quoi on cuit autre chose. C'est le rythme et le rôle de ce moteur.

**Statut** : fin de la phase 3 (scène et ECS). Le jeu construit avec, *[Rando](https://github.com/PhantomDO/Rando/blob/main/docs/JEU.md)*, vit dans son
propre dépôt.

- [Spécifications](docs/SPECS.md)
- [Roadmap chiffrée](docs/ROADMAP.md)
- [Journal de bord](docs/JOURNAL.md)
- [Décisions d'architecture (ADR)](docs/adr/)
- [Études comparatives](docs/etudes/)
- [Lectures commentées](docs/LECTURES.md)
- [Questions et réponses](docs/QA.md)
- [Mise en route](docs/SETUP.md)

## Compiler

Les outils sont listés dans [SETUP.md](docs/SETUP.md). Puis :

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --test-dir build/linux-debug
```

## Faire un jeu avec Levain

Un jeu récupère le moteur par `FetchContent`, à un commit figé
([ADR-0018](docs/adr/0018-moteur-plugins-et-jeu.md)) ; les tests et le sandbox ne sont alors pas construits.

```cmake
FetchContent_Declare(levain
    GIT_REPOSITORY https://github.com/PhantomDO/Levain.git
    GIT_TAG <commit>)
FetchContent_MakeAvailable(levain)
target_link_libraries(mon_jeu PRIVATE levain::scene levain::gpu)
```

- **Le `vcpkg.json` du jeu doit contenir celui du moteur** (même baseline, mêmes dépendances) et une copie de
  `ports/`. vcpkg ne lit que le manifeste du projet principal ; le moteur vérifie la copie à la configuration,
  et dit ce qui manque.
- **Pour modifier le moteur et le jeu en même temps**, sans rien publier :
  `cmake --preset linux-debug -DFETCHCONTENT_SOURCE_DIR_LEVAIN=../Levain`.
- **Un plugin** se déclare par `levain_add_plugin(<nom> SOURCES … DEPENDS …)` ([cmake/LevainPlugin.cmake](
  cmake/LevainPlugin.cmake)) : il ne lie que ce qu'il déclare, et le moteur ne dépend jamais de lui.

## Licence

[MIT](LICENSE).
