# `engine/core`

## Rôle

Le socle : types de base, logs, assertions, allocateurs, temps, fichiers. Tout le moteur en dépend, et lui ne
dépend de rien — ni SDL3, ni NVRHI, ni flecs.

**État en M0.2** : quasiment vide. Il ne contient que la bannière de version, dont le seul but est de prouver que
la chaîne de build tient debout de bout en bout (bibliothèque statique → exécutable, C++23 disponible). Le vrai
contenu arrive en **M0.3** : logs par catégorie, assertions, allocateurs linéaire et pool, horloge, fichiers.

## Invariants

1. **Aucune dépendance sortante.** `core` est la racine du graphe (SPECS §7). Il n'inclut aucun en-tête de
   bibliothèque tierce autre que la bibliothèque standard. Si un jour `core` a besoin de spdlog, ça se décide
   dans un ADR, pas dans un `#include`.
2. **Pas de flecs ici.** Le modèle objet commence à `engine/scene`. `core` ne sait pas ce qu'est une entité.
3. **Compilable seul.** `levain_core` doit se construire sans qu'aucun autre module du moteur existe.

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/core/version.hpp`](include/levain/core/version.hpp) | `version()` et `toolchain()` — la bannière de démarrage |

Les en-têtes publics vivent sous `include/levain/core/`, l'implémentation sous `src/`. On inclut donc
`"levain/core/version.hpp"`, jamais un chemin relatif.

## Équivalents ailleurs

| Moteur | Module | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `Runtime/Core` | `FString`, `FMemory`, `check`/`ensure`, `FPlatformTime`, `FMemStack`. Même rôle, même position au bas du graphe (**documenté** : le module est dans les sources publiques). |
| **Godot** | `core/` | `Variant`, `Error`, `Memory`, `OS`, `String`. Godot y met aussi son système d'objets (`Object`, `ClassDB`), ce que nous **ne faisons pas** : chez nous le modèle objet est flecs et vit dans `scene` (**documenté** : dépôt public). |
| **Unity** | — | Le cœur C++ d'Unity n'est pas public. Ne pas supposer de correspondance. |

La différence qui compte : chez Unreal et Godot, `Core` porte aussi la réflexion et le système d'objets. Chez
nous, flecs s'en charge, donc `core` reste plus petit — logs, mémoire, temps, fichiers, et rien d'autre.
