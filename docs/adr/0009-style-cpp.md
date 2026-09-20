# ADR-0009 — Style C++ : variante mixte, décoration modérée, commentaires en français

- **Statut** : accepté le 2026-09-20 ; partie nommage remise en vigueur par [ADR-0011](0011-retour-au-cpp.md), qui y ajoute la forme du code (fonctions libres, dépendances dans la signature, pièges nommés)
- **Date** : 2026-09-20
- **Milestone** : M0.2

## Contexte

Donnovan relit tout le code du projet avec un budget de 1 à 2 h par semaine. Le C++ est verbeux et propose
plusieurs conventions incompatibles pour écrire la même chose ; sans règle fixée, chaque fichier dérive vers le
style du moment et la relecture coûte de l'attention à rien.

Le moment est le bon : le dépôt contient 27 lignes de code. Aucune conversion à faire.

Trois variantes complètes du même allocateur linéaire (le vrai, celui de l'issue #7) ont été soumises à
Donnovan, qui a choisi en lisant plutôt qu'en discutant.

## Options envisagées

| Variante | Nommage | Pour | Contre |
|---|---|---|---|
| A — proche d'Unreal | Tout en PascalCase | Familier venant d'Unreal | Paramètres et locales au même niveau visuel que les membres : plus rien ne ressort |
| B — proche de la bibliothèque standard | Tout en `snake_case` | Aucun changement de registre entre `std::` et nous | Nos types deviennent indiscernables de ceux de `std` et de flecs |
| **C — mixte** | Types `PascalCase`, fonctions et variables `camelCase`, membres `m_` | Le `m_` sépare l'état des locales ; les types `PascalCase` séparent notre code des tiers | Aucun standard externe à invoquer : c'est notre règle, il faut l'outiller |

## Décision

**Variante C**, décoration **modérée**, commentaires **en français**.

### Nommage

| Élément | Forme | Exemple |
|---|---|---|
| Types (classes, structs, enums, alias) | `PascalCase` | `LinearAllocator`, `DeviceManager` |
| Fonctions et méthodes | `camelCase` | `allocate`, `usedBytes`, `createDevice` |
| Variables locales et paramètres | `camelCase` | `size`, `alignment`, `alignedOffset` |
| Membres de données non publics | `m_` + `camelCase` | `m_buffer`, `m_offset` |
| Constantes de compilation et enums | `PascalCase` | `MaxFramesInFlight` |
| Macros (à éviter) | `LEVAIN_SCREAMING_CASE` | `LEVAIN_ASSERT` |
| Namespaces, fichiers, dossiers | `snake_case` | `levain::core`, `device_manager_vk.cpp` |

Le `m_` est la règle qui porte le plus : à n'importe quelle ligne du corps d'une méthode, on sait si on touche
de l'état qui survit à l'appel ou une variable jetable, sans remonter à la déclaration. Dans un moteur, où la
majorité des bugs coûteux sont des bugs d'état, c'est le renseignement le plus utile qu'un identifiant puisse
porter.

Les types en `PascalCase` répondent à un problème concret : nos dépendances sont déjà un mélange de conventions
(`nvrhi::CommandListHandle`, `SDL_CreateWindow`, `ecs_world_t`, `JPH::Body`, `std::unique_ptr`). Un type
`PascalCase` dans un namespace `levain::` dit « ça, c'est à nous » sans y réfléchir.

### Mise en forme

Accolades **Allman**, indentation de **4 espaces**, **100 colonnes**, namespaces non indentés,
`void* p` et non `void *p`. Tout cela est appliqué mécaniquement par `.clang-format` : ce n'est jamais un sujet
de relecture.

Deux réglages méritent d'être justifiés parce qu'ils vont contre l'esthétique courante :

- **Pas d'alignement en colonnes** (`AlignConsecutiveAssignments: None`). C'est joli, mais renommer un membre
  reflow ses voisins : le diff gonfle et la PR devient plus longue à relire. La règle non négociable n°2 (une PR
  se relit en 30 minutes) passe avant l'esthétique.
- **Pas d'indentation de namespace**. `levain::core` coûterait 4 colonnes à tout le fichier pour une information
  que le chemin de l'en-tête donne déjà.

### Décoration

`explicit` et `const` partout où c'est vrai. `[[nodiscard]]` **seulement quand ignorer le retour est un bug**.
`noexcept` **seulement quand c'est réellement garanti**.

Le critère est l'information : une décoration mise partout ne se lit plus, donc ne renseigne plus.
`[[nodiscard]]` sur `allocate` dit « ignorer ce pointeur, c'est une fuite » ; le même attribut sur `usedBytes`
ne dirait rien. Poser `noexcept` par réflexe est pire que de l'omettre : c'est une promesse que le compilateur
transforme en `std::terminate` si elle est fausse.

### Langue des commentaires

**Français.** Les commentaires existent pour Donnovan, seul relecteur du projet, et il lit le français plus
vite. Les identifiants, les commits, les logs et les messages d'erreur restent **en anglais**.

C'est une exception explicite à SPECS §8, qui est amendé en conséquence. Le compromis assumé : le dépôt est
public, un lecteur non francophone comprendra le code mais pas les commentaires. À notre échelle et vu qu'il n'y
a qu'un relecteur, l'attention de Donnovan vaut plus que l'audience hypothétique.

## Conséquences

- `.clang-format` est versionné et fait autorité sur la mise en forme. Un désaccord de mise en forme en
  relecture est un bug de configuration, pas une discussion.
- clang-format **ne vérifie pas le nommage**. C'est clang-tidy, règle `readability-identifier-naming`, qui s'en
  charge — mise en place dans l'issue #5, avec la vérification en CI.
- Le style ne se négocie plus PR par PR. Le modifier demande d'amender cet ADR.
- Les 27 lignes de code existantes sont reformatées dans la PR qui introduit cet ADR.

## Ce que font les autres moteurs

| Moteur | Style | Source |
|---|---|---|
| **Unreal** | `PascalCase` partout, préfixes de type `F`/`U`/`A`, Allman, paramètres préfixés `In` | [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine) (documenté) |
| **Godot** | Classes `PascalCase`, constantes `SHOUTING_SNAKE_CASE`, **paramètres préfixés `p_`** (et `r_` pour ceux que la méthode modifie), indentation par **tabulations**, `void *p` (opérateur collé à la variable) | [Code style guidelines](https://docs.godotengine.org/en/4.4/contributing/development/code_style_guidelines.html) (documenté) |
| **Unity** | Cœur C++ non public | pas de source — ne pas supposer |

Le détail intéressant est celui de Godot : leur préfixe `p_` sur les **paramètres** résout exactement le même
problème que notre `m_` sur les **membres**, par l'autre bout. Les deux moteurs dont le style est publiquement
documenté marquent donc explicitement la portée des identifiants dans leur nom — Unreal par `In` sur les
paramètres, Godot par `p_`. Le `m_` n'est pas une lubie : c'est la même idée appliquée au côté qui, dans un
moteur, cause le plus de bugs.

Nous divergeons de Godot sur deux points, en connaissance de cause : espaces plutôt que tabulations, et
`void* p` plutôt que `void *p` — le type est « pointeur vers void » et se lit d'un bloc.
