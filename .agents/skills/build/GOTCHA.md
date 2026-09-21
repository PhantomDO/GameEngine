# Pièges — build, tests, sanitizers, CI

Un piège par entrée : symptôme, cause, parade. Le plus récent en haut. Les pièges propres à SDL sont détaillés
dans `engine/platform/README.md`, section « Pièges connus ».

## Fenêtre et SDL

- **Fenêtre invisible sous Wayland** (2026-09-21). Une surface Wayland n'apparaît qu'après sa première image ;
  tant que le moteur ne présente rien (avant M1.2), KWin ne la connaît pas. Parade : `SDL_VIDEO_DRIVER=x11`.
- **Deux SIGTERM font sauter une assertion du SDL de Debug** (`SDL_quit.c:171`), et SDL ouvre une boîte de
  dialogue zenity sur le bureau, qui attend une réponse. Cause : `timeout` signale l'enfant **puis** son groupe
  de processus. Parade : `timeout --foreground`, toujours.
- **SIGINT ignoré en arrière-plan.** Un processus lancé avec `&` depuis un shell non interactif hérite d'un
  SIGINT ignoré, et SDL respecte ce choix : `kill -INT` ne fait rien. Arrêter le sandbox par SIGTERM.
- **Titres non ASCII perdus sous X11** (`SDL_x11window.c:2300`) : SDL abandonne en silence et fuit. Une
  assertion ASCII dans `setWindowTitle` l'empêche.
- **Dépendances système de SDL en CI.** Les paquets suggérés par le port vcpkg ne suffisent pas : SDL exige
  huit extensions X11 (contrôles stricts de `cmake/sdlchecks.cmake`). La liste de `ci.yml` vient du
  `README-linux` de SDL ; en cas d'erreur `Couldn't find dependency package for X`, chercher le `-dev` de X.
- **Sous KWin, `Scripting.start()` exécute le script plus tard** : le décharger tout de suite l'annule sans
  message. `tools/kwin-window-smoke.sh` attend 0,5 s entre les deux.

## CI

- **Le démarrage peut manger tout le délai du sandbox** (2026-09-21). Sur le runner, lavapipe, les couches de
  validation et les sanitizers prennent jusqu'à ~3 s ; avec un délai de 3 s, la boucle ne tournait plus du tout
  et l'étape restait verte. Le sandbox journalise « boucle arrêtée après X s », la CI exige X ≥ 1. Un démarrage
  qui s'allonge (shaders en M1.3) se verra là.
- **`timeout … | tee`** : sans `set -o pipefail`, le code de sortie est celui de `tee`, et une fuite signalée
  par LeakSanitizer passerait inaperçue.

## Sanitizers

- **LeakSanitizer et RADV : 128 octets** (2026-09-21). Le loader Vulkan décharge le pilote à la destruction de
  l'instance ; la mémoire que RADV gardait dans une globale paraît alors perdue. Diagnostic : la fuite
  disparaît avec `LD_PRELOAD=/usr/lib/libvulkan_radeon.so`, persiste sans couche de validation et sans
  `device_select`. Vérification sans faux positif : **Wayland + RADV préchargé**, code 0.
- **Ne pas précharger RADV et libX11 ensemble sous X11** : `SDL_CreateWindow` bloque dans `X11_ShowWindow`
  (`XIfEvent` attend un `MapNotify` qui ne vient pas). Configuration de diagnostic seulement, sans incidence sur
  un lancement normal.

- **Faux positifs LeakSanitizer sous X11** : ~50 Ko en ~900 allocations, la mémoire permanente de libX11 que SDL
  décharge par `dlclose`. Pour vérifier qu'il ne reste rien de vrai, précharger les bibliothèques X11
  (`LD_PRELOAD=/usr/lib/libX11.so.6:…`) : les faux positifs disparaissent, les vraies fuites restent. La CI
  tourne en offscreen et n'est pas concernée.
- **Pile tronquée à une bibliothèque système** : `ASAN_OPTIONS=fast_unwind_on_malloc=0` pour une pile complète.
- **Un code de sortie lu à travers un pipe** est celui du dernier programme (`| tail` rend 0). Rediriger vers
  un fichier, puis lire `$?`.

## Contre-tests

- **Une faute injectée pour un contre-test se retire depuis une copie** (`cp fichier copie`, puis `cp copie
  fichier`), jamais par `git checkout -- fichier` : il efface aussi tout ce qui n'était pas encore commité. C'est
  arrivé le 2026-09-21 à l'intégration du device dans le sandbox, réécrite ensuite.
- Méthode qui marche pour la validation : un buffer Vulkan de taille 0 (`VUID-VkBufferCreateInfo-size-00912`) et
  une texture NVRHI de largeur 0 doivent chacun arrêter le programme (code 133, SIGTRAP).

## Avertissements et clang-tidy

- **Une variable qui ne sert qu'à une assertion** : `LEVAIN_ASSERT` compile son expression en Release sans
  l'évaluer (`sizeof`), donc pas d'avertissement. Une **fonction interne** dans le même cas reste signalée par
  clang (`-Wunneeded-internal-declaration`) : `[[maybe_unused]]`.
- **Constante globale d'un type non `constexpr`** (`const nvrhi::Color c{…}`) : clang-tidy la refuse
  (`bugprone-throwing-static-initialization`), une exception levée avant `main` ne se rattrape pas. La rendre
  locale à la fonction qui s'en sert.
- **Initialisation désignée incomplète** (`-Wmissing-designated-field-initializers`) : donner un initialiseur
  par défaut au champ (`PixelSize pixelSize{};`) plutôt que d'écrire `.champ = {}` partout.
- **clang-tidy 22 et `std::optional`** : `.value()` compte comme un accès non vérifié, et `REQUIRE` de doctest
  n'est pas reconnu comme une garde. Dans les tests : `value_or(T{})`, dont la valeur par défaut fait échouer
  les `CHECK`.
- **Macros de doctest** : `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` est posée par `set_source_files_properties` dans
  `tests/CMakeLists.txt`, pas par un `#define` que clang-tidy refuserait (préfixe `LEVAIN_`).
- **`main` et les exceptions** : `std::print` peut lever, et une exception qui sort de `main` est signalée par
  clang-tidy. Un `try`/`catch` au sommet de `main` (ADR-0008).

## Bureau de Donnovan

- **Ne pas faire de capture d'écran de son bureau** (2026-09-21). `spectacle -a` capture la fenêtre active, pas
  celle qu'on vise : KWin a refusé de donner le focus au profileur, et l'image montrait son navigateur sur une
  page de connexion. Supprimée aussitôt. Une capture d'écran se demande à Donnovan.
- **`pkill -f <motif>` tue aussi le shell qui l'exécute** si sa ligne de commande contient le motif. Cibler par
  nom de processus (`pgrep -x`, `ps -eo pid,comm`).

## Chaîne d'outils

- **`VCPKG_ROOT` non exportée** : CMake prenait les paquets du système (le `spdlog` d'Arch) sans rien dire. Le
  `CMakeLists.txt` racine refuse maintenant de se configurer sans la toolchain vcpkg.
- **LLVM en CI** : `update-alternatives` ne remplace pas `/usr/bin/clang++`, qui est un vrai fichier. La CI a
  tourné trois milestones sur clang 18 en croyant utiliser clang 22. Parade en place : liens dans
  `/usr/local/bin` et étape « Vérifier la chaîne », qui échoue si la version n'est pas la bonne.
- **clang-format change de sortie d'une version majeure à l'autre** : la version de la CI (`LLVM_VERSION`) doit
  rester celle de la machine de référence.
- **`$env{…}` dans une chaîne de `message()` CMake** est interprété et casse le parsing : l'écrire autrement.
- **Le bootstrap de vcpkg exige `zip`** (paquet système).
- **Le shell de Donnovan est fish** : `set -Ux` pour une variable d'environnement persistante. Les scripts du
  dépôt commencent par `#!/usr/bin/env bash`.

## Tracy

- **`TRACY_ENABLE` est OFF par défaut depuis Tracy 0.14** (2026-09-21). Sans lui, les macros se compilent en
  rien : le build réussit, le binaire ne profile rien. Le port overlay le force, et `engine/core/CMakeLists.txt`
  refuse de configurer un build profilé sans lui. Preuve qu'un client est actif : il écoute sur le port 8086
  (`ss -ltnp | grep 8086`), et `TRACY_NO_EXIT=1` l'empêche de sortir.
- **`TRACY_NO_EXIT=1` n'est pas optionnel** pour un programme court : sans lui, le programme se termine avant
  qu'un profileur ait pu se connecter, et n'affiche aucun avertissement.
- **Client et profileur doivent avoir la même version de protocole** : 0.14.1 des deux côtés (port overlay pour
  le client, release officielle pour les outils, empreinte SHA-256 vérifiée contre celle publiée par GitHub).

## Quand Windows reviendra

Les presets et les jobs CI Windows ont été retirés (ADR-0011) : les remettre ensemble. Deux bugs de M0.2,
propres à MSVC : `__cplusplus` figé à 199711 sans `/Zc:__cplusplus`, et `<ostream>` non inclus en cascade par la
STL de Microsoft. Voir l'ADR-0011 pour le choix du compilateur (clang-cl d'abord).
