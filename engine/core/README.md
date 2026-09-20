# `engine/core`

## Rôle

Le socle : types de base, logs, assertions, allocateurs, temps, fichiers. Tout le moteur en dépend, et lui ne
dépend de rien — ni SDL3, ni NVRHI, ni flecs.

**État en M0.3** : logs par catégorie, assertions, et la politique d'erreurs de l'[ADR-0008](../../docs/adr/0008-gestion-erreurs.md).
Les allocateurs, l'horloge et les fichiers suivent dans le même milestone.

## Invariants

1. **Aucune dépendance tierce dans les en-têtes publics.** `core` est la racine du graphe (SPECS §7). spdlog
   est lié en `PRIVATE` et n'apparaît que dans les `.cpp` : c'est pour ça que `log.hpp` expose un
   `logMessage(std::string_view)` non générique, tout le formatage étant fait par l'appelant avec
   `std::format`. Un module qui lie `levain::core` ne voit pas spdlog.
2. **Pas de flecs ici.** Le modèle objet commence à `engine/scene`. `core` ne sait pas ce qu'est une entité.
3. **Compilable seul.** `levain_core` doit se construire sans qu'aucun autre module du moteur existe.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/core/log.hpp`](include/levain/core/log.hpp) | `log(category, level, fmt, …)` — une catégorie par module, le niveau est testé **avant** le formatage |
| [`include/levain/core/assert.hpp`](include/levain/core/assert.hpp) | `LEVAIN_ASSERT` (Debug seulement) et `LEVAIN_VERIFY` (évalue toujours) |
| [`include/levain/core/error.hpp`](include/levain/core/error.hpp) | `Result<T>` = `std::expected<T, Error>`, pour les échecs qui ne sont pas des bugs |
| [`include/levain/core/version.hpp`](include/levain/core/version.hpp) | `version()` et `toolchain()` — la bannière de démarrage |

**La règle de lecture qui découle de l'ADR-0008** : si une fonction rend un `Result`, elle peut échouer sans que
ce soit notre faute. Si elle n'en rend pas, tout échec est un bug et s'arrête sur une assertion.

Les en-têtes publics vivent sous `include/levain/core/`, l'implémentation sous `src/`. On inclut donc
`"levain/core/version.hpp"`, jamais un chemin relatif.

## Équivalents ailleurs

| Moteur | Module | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `Runtime/Core` | `FString`, `FMemory`, `check`/`ensure`/`verify`, `FPlatformTime`, `FMemStack`. Même rôle, même position au bas du graphe. Notre `LEVAIN_VERIFY` est directement leur `verify` (**documenté** : sources publiques). |
| **Godot** | `core/` | `Variant`, `Error`, `Memory`, `OS`, `String`. Godot y met aussi son système d'objets (`Object`, `ClassDB`), ce que nous **ne faisons pas** : chez nous le modèle objet est flecs et vit dans `scene` (**documenté** : dépôt public). |
| **Unity** | — | Le cœur C++ d'Unity n'est pas public. Ne pas supposer de correspondance. |

La différence qui compte : chez Unreal et Godot, `Core` porte aussi la réflexion et le système d'objets. Chez
nous, flecs s'en charge, donc `core` reste plus petit — logs, mémoire, temps, fichiers, et rien d'autre.
