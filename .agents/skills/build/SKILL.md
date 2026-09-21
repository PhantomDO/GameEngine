---
name: build
description: Compiler, tester et vérifier Levain — presets CMake et vcpkg, tests doctest, format, clang-tidy, sanitizers, profilage Tracy, CI, et tests manuels de la fenêtre sous KWin. À utiliser pour tout build, toute mesure et toute modification de la CI.
---

# Build, tests et CI

Lire d'abord [`GOTCHA.md`](GOTCHA.md).

## Prérequis, une seule fois

vcpkg cloné et bootstrappé, puis `VCPKG_ROOT` exporté : les presets lisent cette variable, et le
`CMakeLists.txt` racine refuse de se configurer sans la toolchain vcpkg.

```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
set -Ux VCPKG_ROOT ~/vcpkg   # fish ; bash : echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
```

## Compiler et tester

| Preset | Contenu |
|---|---|
| `linux-debug` | Debug, assertions actives |
| `linux-release` | RelWithDebInfo |
| `linux-asan` | Debug + AddressSanitizer, LeakSanitizer, UBSan |

```bash
cmake --preset linux-debug && cmake --build --preset linux-debug
ctest --test-dir build/linux-debug --output-on-failure
```

Le premier `cmake --preset` est long : vcpkg compile les dépendances depuis les sources. Les suivants sont
instantanés (cache `~/.cache/vcpkg`). Pour clangd : `ln -sf build/linux-debug/compile_commands.json .`

Format et analyse statique, comme la CI :

```bash
find engine sandbox tests -name '*.cpp' -o -name '*.hpp' | xargs clang-format --dry-run --Werror
find engine sandbox tests -name '*.cpp' | xargs clang-tidy -p build/linux-debug --warnings-as-errors='*'
```

`-pedantic-errors` (C++23 strict) et `-Wall -Wextra -Werror` sont dans le `CMakeLists.txt` racine : **ne jamais
les retirer**. Un avertissement ou une extension C++26 doit casser le build.

## Lancer le sandbox

```bash
SDL_VIDEO_DRIVER=x11 ./build/linux-debug/sandbox/levain_sandbox   # x11 : voir GOTCHA.md, jusqu'en M1.2
```

Comme la CI, avec les sanitizers (code 0 = ni fuite ni comportement indéfini) :

```bash
SDL_VIDEO_DRIVER=offscreen timeout --foreground --preserve-status -k 10 3 \
  ./build/linux-asan/sandbox/levain_sandbox
```

## Tester la fenêtre comme un utilisateur

Sandbox lancé sous X11, puis `./tools/kwin-window-smoke.sh` (Plasma uniquement) : KWin redimensionne, minimise et
restaure la vraie fenêtre, et le script mesure le CPU consommé. Lire le titre réellement affiché :

```bash
gdbus call --session --dest org.kde.KWin --object-path /WindowsRunner --method org.kde.krunner1.Match Levain
```

## Profilage Tracy

Désactivé par défaut. Le client est Tracy **0.14.1**, par le port overlay `ports/tracy` (ADR-0007, amendement).
Les outils de la même version (profileur, `tracy-capture`, `tracy-csvexport`) viennent de la release officielle,
décompressée dans `~/.local/opt/tracy-0.14.1/` (`TRACY_DIR` pour un autre dossier).

```bash
cmake -S . -B build/prof -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DLEVAIN_PROFILING=ON
cmake --build build/prof
./tools/tracy-capture.sh 3 captures/sandbox.tracy        # capture scriptée + statistiques des zones
~/.local/opt/tracy-0.14.1/tracy-profiler-x86_64.AppImage captures/sandbox.tracy   # ouvrir la capture
```

Pour profiler en direct : lancer le profileur, puis `TRACY_NO_EXIT=1 ./build/prof/sandbox/levain_sandbox`.

**Analyser une capture par MCP (à mettre en place en M1.2)**. Tracy 0.14.1 fournit un serveur MCP
(`extra/mcp/tracy_mcp.py` dans ses sources, manuel § « MCP Server ») : un agent y charge une capture `.tracy`
et l'interroge par du Python (`eval` sur l'objet `Worker`), ou compare deux captures. C'est la voie à préférer
aux captures d'écran. Prérequis : les bindings Python de Tracy (`-DTRACY_CLIENT_PYTHON=ON`), `pip install mcp`,
puis déclarer `http://127.0.0.1:47380/mcp` auprès de l'agent.

## CI

`.github/workflows/ci.yml` : une matrice `linux-debug`, `linux-release`, `linux-asan`, chacun compilé, testé et
lancé 3 s ; format et clang-tidy sur `linux-debug`. Les trois sont des checks requis pour fusionner sur `main`.
