# ADR-0008 — Gestion des erreurs : assertions pour les bugs, `std::expected` pour le reste

- **Statut** : proposé
- **Date** : 2026-09-20
- **Milestone** : M0.3

## Contexte

Il faut une politique unique pour signaler qu'une chose s'est mal passée. Sans règle, chaque module invente la
sienne et le code devient illisible : ici un `bool`, là un `int` de retour, ailleurs une exception.

L'énoncé habituel du débat — « exceptions ou codes de retour ? » — est mal posé, et c'est le cœur de cet ADR :
**il y a deux sortes d'échecs**, qui n'appellent pas la même réponse.

1. **Un bug du moteur.** Indice hors bornes, pointeur nul là où c'est impossible, invariant violé. La bonne
   réponse n'est pas de « gérer » l'erreur : c'est de s'arrêter le plus près possible de la faute pour la
   corriger. Un code de retour ici ne fait que déplacer le problème.
2. **Un échec de l'environnement.** Fichier absent, shader qui ne compile pas, device GPU perdu, mémoire
   insuffisante. Ce n'est pas un bug : l'appelant doit pouvoir le voir et décider.

## Options envisagées

| Option | Pour | Contre |
|---|---|---|
| Exceptions partout | Chemin nominal lisible, impossible d'ignorer un échec | Chemin de sortie invisible à la lecture ; coût au *throw* ; la plupart des moteurs les évitent |
| Codes de retour nus (`bool`, `int`) | Simple, sans coût | **S'ignorent en silence** ; ne portent aucun contexte ; obligent à un paramètre de sortie pour rendre une valeur |
| Macros à la Godot (`ERR_FAIL_COND_V`) | Éprouvé, uniforme | Le contrôle de flux est caché dans une macro ; conçu pour *continuer* après l'erreur, ce qui n'est pas notre besoin |
| **Assertions + `std::expected`** | Sépare les deux sortes d'échecs ; le type de retour porte la valeur **ou** l'erreur ; `[[nodiscard]]` empêche de l'ignorer | Deux mécanismes à connaître au lieu d'un |

## Décision

### 1. Les bugs s'arrêtent : `LEVAIN_ASSERT`

`LEVAIN_ASSERT(expression, message)` journalise l'expression, le message, le fichier, la ligne et la fonction,
puis déclenche un arrêt dans le débogueur. **Active en Debug seulement.**

`LEVAIN_VERIFY(expression, message)` fait la même chose mais **évalue toujours l'expression**, y compris en
Release. C'est le piège classique des assertions — `assert(file.close())` qui disparaît en Release et laisse le
fichier ouvert — et il porte un nom plutôt qu'un commentaire.

L'arrêt utilise `__builtin_debugtrap` sous Clang, qui **rend la main si aucun débogueur n'est attaché**, là où
le `__builtin_trap` de GCC termine le processus.

### 2. Les échecs récupérables se renvoient : `Result<T>`

```cpp
Result<Shader> compileShader(std::string_view path);
```

`Result<T>` est un alias de `std::expected<T, Error>`, et `Error` porte un `ErrorCode` et un message lisible.
**C'est la raison pour laquelle le projet est en C++23** (ADR-0001) : le type de retour porte la valeur *ou*
l'erreur, un appelant ne peut pas ignorer l'échec par distraction, et il n'y a ni exception ni paramètre de
sortie.

L'énumération `ErrorCode` est volontairement courte — quatre entrées — et s'étendra sur cas concret.

### 3. Les exceptions ne sont pas un outil de contrôle de flux, mais ne sont pas désactivées

Le code du moteur n'en lève pas et ne s'en sert pas pour signaler un échec. Elles sont rattrapées au sommet de
`main`, ce que le sandbox fait déjà.

**Mais `-fno-exceptions` n'est pas activé**, et c'est un point où nous divergeons de la pratique courante des
moteurs. La raison est concrète : **nous utilisons la bibliothèque standard**, et avec `-fno-exceptions`, un
`throw` de libstdc++ — `std::bad_alloc`, `std::vector::at`, `std::format` sur une chaîne invalide — se replie
sur `__builtin_trap()`, c'est-à-dire un arrêt brutal sans message
([discussion Godot](https://github.com/godotengine/godot-cpp/issues/1326)). Unreal et Godot peuvent désactiver
les exceptions parce qu'ils ont remplacé la quasi-totalité de la STL par leurs propres conteneurs et
allocateurs. Ce n'est pas notre cas, et ça ne le sera pas à notre échelle.

À rouvrir si un profil montre un coût réel, ou si nous finissons par écrire nos propres conteneurs.

## Conséquences

- **Une règle de lecture simple** : si une fonction rend un `Result`, elle peut échouer sans que ce soit notre
  faute. Si elle n'en rend pas, tout échec est un bug et s'arrête.
- `[[nodiscard]]` sur les fonctions qui rendent un `Result` — la décoration est ici porteuse d'information, ce
  que l'ADR-0009 appelle la « décoration modérée ».
- Les assertions disparaissant en Release, **un invariant qui doit tenir en production n'est pas une
  assertion** : c'est un `Result`, ou une correction du code.
- Pas de `try`/`catch` dans le code du moteur, hors du sommet de `main`.

## Ce que font les autres moteurs

| Moteur | Approche | Source |
|---|---|---|
| **Unreal** | `check` (fatal), `ensure` (journalise et continue), `verify` (évalue toujours). Exceptions désactivées dans le code moteur. | Sources publiques (documenté) |
| **Godot** | Macros `ERR_FAIL_COND_V` et consorts. **N'utilise pas d'exceptions.** Philosophie explicite : « bugs et données invalides ne sont pour la plupart pas fatals, ils ne doivent jamais faire planter une application qui tourne » | [Common engine methods and macros](https://docs.godotengine.org/en/stable/engine_details/architecture/common_engine_methods_and_macros.html) (documenté) |
| **Unity** | Cœur C++ non public | pas de source — ne pas supposer |

Notre `LEVAIN_VERIFY` est directement le `verify` d'Unreal. En revanche **nous divergeons de Godot sur la
philosophie** : leurs macros sont faites pour *continuer* après une erreur, parce que Godot héberge un éditeur
où planter fait perdre le travail de l'utilisateur. Un bug qui continue est un bug qu'on trouvera plus loin,
dans un état corrompu. À notre échelle, s'arrêter au plus près de la faute coûte moins cher — et rien n'empêche
d'ajouter un `LEVAIN_ENSURE` à la Godot le jour où l'éditeur (phase 7) en aura besoin.
