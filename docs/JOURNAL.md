# Journal de bord

Une entrée par session de travail, la plus récente en haut. Les heures Donnovan sont celles qu'il déclare ;
les chiffres de performance viennent de commandes versionnées, sur la machine de référence.

## Format d'une entrée

```
## AAAA-MM-JJ — Mx.y — titre court
- Temps Donnovan : x h (estimé y h)
- Sessions Claude Code : n
- Fait : …
- Mesures : critère → valeur (commande)
- Décisions : ADR-XXXX …
- Écarts et problèmes : …
- Prochaine étape : …
```

## Cumul

| Phase | Estimé (h) | Passé (h) | Ratio |
|---|---:|---:|---:|
| 0 | 6,0 | **5,0** | **0,83** |
| 1 | 4,5 | — | — |

---

## 2026-09-21 — M1.1 — Fenêtre SDL3, boucle et événements (#10)

- Temps Donnovan : à renseigner (estimé 0,75 h pour #10)
- Sessions Claude Code : 1
- Fait : module `engine/platform` sur SDL3 3.4.12 (vcpkg, fonctionnalités `x11` et `wayland` seulement, sans
  ibus ni dbus) ; événements traduits vers nos types (`CloseRequested`, `Resized` en pixels, `Hidden`,
  `Shown`) ; boucle du sandbox qui dort quand la fenêtre est masquée ; script `tools/kwin-window-smoke.sh`
  qui pilote la vraie fenêtre à travers KWin ; sandbox lancé 3 s en CI, arrêté par SIGTERM.
- Mesures :
  - redimensionnement puis minimisation et restauration sous X11 : aucun plantage, tailles reçues
    640 × 332 et 1600 × 872 px pour des cadres de 640 × 360 et 1600 × 900 — la barre de titre prend 28 px
    (`tools/kwin-window-smoke.sh`) ;
  - temps CPU : **2 010 ms en 2 s visible, 0 ms minimisée** (même script) ;
  - arrêt propre sur SIGTERM, code 0, en Debug et en Release
    (`SDL_VIDEO_DRIVER=offscreen timeout --foreground --preserve-status -k 10 3 ./build/<preset>/sandbox/levain_sandbox`) ;
  - aucun en-tête SDL hors de `engine/platform/src/` (`grep -rn "include.*SDL" engine sandbox tests`) ;
  - SDL3 compilé par vcpkg en 22 s à froid (`time cmake --preset linux-debug`).
- Écarts et problèmes :
  - **Sous Wayland, la fenêtre n'apparaît pas.** Une surface Wayland n'est affichée qu'après son premier
    buffer, et le moteur ne présente encore rien : KWin ne la connaît pas, alors qu'elle s'affiche sous X11.
    M1.1 se teste donc sous XWayland (`SDL_VIDEO_DRIVER=x11`), et la minimisation sous Wayland est reportée à
    M1.2, ajoutée à l'issue #13.
  - **Assertion de SDL sur un double signal** (`SDL_quit.c:171`) : `timeout` envoie SIGTERM à l'enfant puis
    au groupe de processus ; le second signal tombe entre la remise à zéro d'un drapeau et l'assertion qui la
    vérifie. Le SDL de Debug a alors ouvert une boîte de dialogue zenity sur le bureau, et le programme a
    attendu jusqu'au SIGKILL. Parade : `timeout --foreground`, un seul signal.
  - Un processus lancé avec `&` depuis un shell non interactif hérite d'un SIGINT **ignoré**, et SDL respecte
    ce choix : c'est pourquoi la CI et le script arrêtent le sandbox par SIGTERM.
  - **PR découpée** : la version complète de M1.1 faisait environ 710 lignes, près du double de la règle
    n°2. Le frame time et les sanitizers (#11) suivent dans une seconde PR, déjà prête.
  - Tracy (#38) : vcpkg `master` est toujours en 0.13.1 au 21/09.
- Prochaine étape : PR de l'issue #11 — frame time dans le titre, ASan et UBSan en CI.

## 2026-09-20 — M0.3 et phase 0 — Clôture

- **Temps Donnovan : 5,0 h** (12h00–14h30 et 21h30–23h55, soit ~4,9 h, arrondi au quart d'heure sur le board)
- Définition de « terminé » (SPECS §9) : 5 critères sur 7 remplis — démo lançable sous Linux, mesures
  consignées, CI verte, README du module à jour, **étude E0 écrite**. Deux sans objet : pas de démo Windows
  (hors périmètre, ADR-0011), pas de binaire à publier.

### L'erreur de mesure, corrigée par Donnovan

**Je mesurais la mauvaise chose depuis le début du projet.** Je demandais « combien de temps sur la
**relecture** » et n'enregistrais que ça, alors que les « Heures Donnovan » de la ROADMAP comptent **tout**
l'engagement : pilotage, questions, décisions, relecture.

| | Estimé | Passé | Ratio | Conséquence |
|---|---:|---:|---:|---|
| Mesure erronée (relectures seules) | 6,0 h | 3,0 h | **0,50** | recalibrage déclenché, −30 % sur 63 h |
| **Mesure réelle** (Donnovan, 20/09) | 6,0 h | **5,0 h** | **0,83** | **dans la fourchette, rien à changer** |

Le ratio erroné aurait amputé la roadmap d'environ 19 h sans aucune raison. CLAUDE.md est corrigé : la question
de fin de session porte désormais sur le **temps total**, avec la raison écrite pour qu'elle ne redérive pas.

### Phase 0 — bilan

| Milestone | Estimé | Passé |
|---|---:|---:|
| M0.1 Dépôt, suivi et specs | 1,0 h | 1,25 h |
| M0.2 Squelette de build et CI | 1,5 h | 1,50 h |
| M0.4 Socle Rust | 1,0 h | 0,25 h |
| M0.5 Retour au C++ | 1,0 h | 0,75 h |
| M0.3 Core minimal | 1,5 h | 1,25 h |
| **Phase 0** | **6,0 h** | **5,0 h** — ratio **0,83** |

**Aucun recalibrage** : le ratio est dans la fourchette 0,8–1,25 de la ROADMAP. À réexaminer à la clôture de la
phase 1, premier échantillon de vrai code de rendu — la phase 0 était faite de specs, d'ADR et de
configuration, et ne prédit pas grand-chose.

### Trois pannes silencieuses, un même motif

La journée en a produit trois, toutes « vertes » pendant qu'elles ne faisaient rien :

1. **L'épinglage de LLVM 22 en CI** — inopérant depuis M0.2, trois milestones sur clang 18. Le workflow
   imprimait la version à chaque run ; personne ne l'a lue.
2. **La baseline vcpkg** — contournée en local par le `spdlog` d'Arch, sans un mot. Découverte parce que Tracy
   n'existe pas en paquet système.
3. **Tracy sans `TRACY_NO_EXIT`** — le programme se termine avant qu'un profileur ait pu se connecter, sans
   avertissement.

Règle inscrite : **un contrôle doit échouer bruyamment quand sa condition n'est pas réunie, jamais se contenter
de ne pas s'exécuter.** Deux garde-fous ajoutés en conséquence (vérification de la chaîne LLVM, refus de
configurer sans toolchain vcpkg).

### Numérotation des ADR

La ROADMAP pré-attribuait cinq numéros d'ADR ; l'aller-retour Rust en a consommé deux (0010, 0011) et toute la
suite avait glissé — M2.1 renvoyait à « ADR-0009 », devenu le style C++. **Les numéros pré-attribués sont
retirés** : un ADR prend son numéro au moment où on l'écrit. Règle ajoutée à la ROADMAP.

### Phase 2 détaillée

Le rituel de clôture de phase demande de détailler la phase N+2. **8 issues créées** (#40 à #47) pour 5,5 h,
conformes au découpage de la ROADMAP : stratégie de binding, caméra et meshes, instancing et timestamps,
textures et mipmaps, samplers, hot-reload Slang, repli sur erreur, étude E2.

- Prochaine étape : **phase 1**, M1.1 — fenêtre SDL3, boucle principale, frame time, sanitizers. Voir aussi
  l'issue #38 (capture Tracy reportée) et #32 déjà close.

## 2026-09-20 — M0.3 — Allocateurs, Tracy et étude E0 (issues #7, #8, #9)

- Temps Donnovan : à renseigner (relecture estimée 0,3 h)
- Sessions Claude Code : 1
- Fait : `LinearAllocator` et `PoolAllocator` avec 11 tests, benchmark, macros de profilage Tracy,
  instrumentation du sandbox, étude **E0 — Comment démarre un moteur**.
- **Benchmark des allocateurs** (Release, machine de référence, médiane de 5 exécutions de 100 000 allocations
  de 64 octets) :

  | Allocateur | ns / allocation | Rapport à `malloc` |
  |---|---:|---:|
  | `malloc` + `free` | 14,50 | 1,0× |
  | `LinearAllocator` | **1,32** | **11,0×** |
  | `PoolAllocator` (alloc + free) | **2,38** | **6,1×** |

  Commande : `cmake --preset linux-release && cmake --build --preset linux-release &&
  ./build/linux-release/tests/levain_bench`

- **Tracy, coût nul quand désactivé — vérifié, pas affirmé** :

  | | Symboles Tracy | Bibliothèques liées | Taille du sandbox |
  |---|---:|---:|---:|
  | Désactivé (défaut) | **0** | **0** | 7 607 232 o |
  | `-DLEVAIN_PROFILING=ON` | 1 025 | 1 | 9 422 024 o |

  Commandes : `nm -C <binaire> \| grep -ci tracy`, `ldd <binaire> \| grep -ci tracy`.

- **Un test a trouvé un vrai bug dans mon allocateur.** `LinearAllocator::allocate` alignait l'**offset** dans
  le tampon et non l'**adresse réelle** ; `make_unique<std::byte[]>` ne garantit que l'alignement par défaut
  (16 octets), donc toute demande supérieure rendait un pointeur mal aligné — silencieusement, puisque ça
  « marche » sur x86. Corrigé en alignant l'adresse. C'est exactement ce que le critère « tests d'alignement »
  de l'issue devait attraper.
- **clang-tidy a trouvé quatre défauts de plus** : deux conversions implicites `void**` → `void*` dans les
  `memcpy` de la liste des libres, une multiplication en `int` élargie en `size_t`, et une exception pouvant
  s'échapper du `main` du benchmark. Tous corrigés, aucun désactivé. Troisième passage de l'outil sur du code
  neuf, troisième récolte.
- **Défaut de ma démo, trouvé par Donnovan en la lançant** : 120 frames à 200 µs font ~24 ms, impossible d'y
  connecter un profileur à la main. La parade est `TRACY_NO_EXIT=1`, qui fait attendre le client jusqu'à ce que
  le profileur se connecte et ait tout reçu. Documenté dans CLAUDE.md. Sans elle, le programme se termine sans
  le moindre avertissement — encore une panne silencieuse.
- Écarts et problèmes : **le critère « capture d'écran Tracy » de l'issue #8 n'est pas rempli, et il est
  reporté à M1.1** (issue #38) plutôt que maquillé. Trois raisons cumulées :
  1. **Versions incompatibles.** Le client vient de vcpkg en **0.13.1** ; les binaires Linux du profileur ne
     commencent qu'à **0.14.0**, et Tracy refuse une connexion dont le protocole ne correspond pas. vcpkg ne
     connaît aucune version ≥ 0.14 (`versions/t-/tracy.json`), donc pas d'`override` possible. Compiler le
     profileur 0.13.1 par `tracy[gui-tools]` reste faisable, mais c'est une interface graphique complète à
     construire depuis les sources.
  2. Aucun profileur installé sur la machine, et pas de paquet Arch.
  3. **La capture n'aurait rien montré d'utile** : boucle factice de 120 frames dont l'essentiel est un `sleep`.
     À M1.1 il y aura une vraie boucle, et une capture dira enfin où part la frame.

  Ce qui est vérifié aujourd'hui : les symboles `__tracy_source_location` sont présents dans le binaire
  instrumenté, et le coût nul quand Tracy est désactivé est mesuré.
- Le sandbox a maintenant une **boucle simulée de 120 frames** : il fallait quelque chose à découper pour que
  `LEVAIN_PROFILE_FRAME` ait un sens. La vraie boucle arrive en M1.1.
- **Deuxième contrôle silencieusement inopérant de la session**, trouvé par Donnovan en lançant la commande
  Tracy que je lui avais donnée. `VCPKG_ROOT` n'était pas exportée dans son shell, donc la toolchain vcpkg
  n'était pas chargée — et **CMake a trouvé le `spdlog` d'Arch dans `/usr/lib/cmake/spdlog` et continué sans
  rien dire**, contournant la baseline figée de l'ADR-0007. Le build n'a échoué que sur Tracy, qui n'existe pas
  en paquet système. Une dépendance de moins et personne ne s'apercevait de rien.
  Garde-fou posé : le `CMakeLists.txt` racine refuse de se configurer si `VCPKG_TOOLCHAIN` n'est pas défini,
  avec un message qui dit quoi faire. Vérifié dans les deux sens.
- **Le motif de la session** : deux vérifications ont passé pendant des semaines en ne faisant rien — l'épinglage
  de LLVM en CI, et la baseline vcpkg en local. Les deux étaient « vertes ». À retenir : **un contrôle doit
  échouer bruyamment quand sa condition n'est pas réunie, jamais se contenter de ne pas s'exécuter.**
- Prochaine étape : clôture de M0.3 et de la phase 0 — ratio, recalibrage de la roadmap, et détail des issues
  de la phase 2.

## 2026-09-20 — M0.3 — Logs, assertions et gestion d'erreurs (issue #6)

- Temps Donnovan : à renseigner (relecture estimée 0,25 h)
- Sessions Claude Code : 1
- Fait : `log.hpp` (catégories et niveaux, spdlog), `assert.hpp` (`LEVAIN_ASSERT`, `LEVAIN_VERIFY`),
  `error.hpp` (`Result<T>` = `std::expected<T, Error>`), **ADR-0008**, 7 nouveaux tests, README du module
  `core` mis à jour.
- **ADR-0008 : le débat « exceptions ou codes de retour » est mal posé.** Il y a deux sortes d'échecs et elles
  n'appellent pas la même réponse : un **bug du moteur** s'arrête au plus près de la faute (`LEVAIN_ASSERT`),
  un **échec de l'environnement** se renvoie (`Result<T>`). C'est le cœur de l'ADR.
- Décision inattendue : **`-fno-exceptions` n'est pas activé**, alors que c'est la pratique courante des
  moteurs. Raison trouvée dans la doc Godot : avec les exceptions désactivées, le `throw` de libstdc++ se
  replie sur `__builtin_trap()` — arrêt brutal sans message. Unreal et Godot peuvent se le permettre parce
  qu'ils ont remplacé la STL par leurs propres conteneurs ; nous l'utilisons pleinement.
- Mesures :

  | Critère de l'issue | Résultat | Commande |
  |---|---|---|
  | Une assertion affiche fichier, ligne et message, puis s'arrête dans le débogueur | **oui**, et code de sortie **133** (SIGTRAP) | programme de démonstration lié à `levain_core` |
  | Tests | **9 verts en Debug et en Release** | `ctest --test-dir build/linux-{debug,release}` |
  | clang-tidy | **0 finding** | `clang-tidy -p build/linux-debug --warnings-as-errors='*'` |

  Sortie de l'assertion violée :
  ```
  [critical] [assert] assertion violée : frameCount >= 0
    message  : le compteur de frames ne peut pas être négatif
    assert_demo.cpp:5 (int main())
  ```

- **clang-tidy a encore trouvé deux vrais points** dès la première exécution sur ce code : `ErrorCode` et
  `LogLevel` utilisaient `int` comme type sous-jacent là où `std::uint8_t` suffit (`performance-enum-size`).
  Corrigé, pas désactivé. C'est la deuxième fois que l'outil paye dès son premier passage sur du code neuf.
- Conformité à l'ADR-0011 (forme du code) : tout est en **fonctions libres avec les dépendances dans la
  signature**, et **chaque piège porte son nom** — `isLogEnabled` avant le formatage pour qu'un `Trace` dans
  une boucle de rendu ne paie pas son `std::format`, `LEVAIN_VERIFY` pour l'expression à effet qui ne doit pas
  disparaître en Release. Les trois règles ont tenu sur du vrai code.
- Écarts et problèmes : `isLogEnabled` fait une **recherche par chaîne à chaque appel**. Noté dans le code :
  si une capture Tracy (issue #8) montre le log dans le profil, la réponse sera un handle de catégorie obtenu
  une fois, pas une optimisation de la table.
- Prochaine étape : issues #7 (allocateurs et benchmarks), #8 (Tracy) et #9 (étude E0), dans une seconde PR.

## 2026-09-20 — M0.5 — Clôture du milestone

- Temps Donnovan : inclus dans les 0,25 h de la relecture de la PR #33
- **Cumul de la phase 0 à ce stade** : **2,75 h passées pour 3,75 h estimées**, soit un **ratio de 0,73** sur
  quatre milestones terminés (M0.1, M0.2, M0.4, M0.5). Reste M0.3, estimé 1,5 h.
  **L'aller-retour par Rust aura coûté 0,5 h de Donnovan au total** — deux milestones, quatre relectures d'un
  quart d'heure. C'est le prix d'avoir tranché la question du langage définitivement, à 107 lignes de code.
- Définition de « terminé » (SPECS §9) : 4 critères sur 7 s'appliquent et sont remplis (démo lançable sous
  Linux, mesures consignées, CI verte, docs à jour). Trois restent sans objet à ce stade : pas de démo Windows
  (hors périmètre, ADR-0011), pas d'étude de phase due en M0.5, pas de binaire à publier.
- Fait : ADR-0011 accepté, tag `m0.5`, release avec les mesures, milestone fermé. Issue #32 fermée — elle
  demandait de réécrire les issues de phase 1 pour la pile Rust, sans objet depuis le retour au C++.
- **Question archivée en Q&R** : « Clang existe aussi sous Windows, pourquoi MSVC ? » Réponse contre-intuitive —
  clang-cl ne règle **aucun** des deux bugs de M0.2 (il reproduit volontairement le piège `__cplusplus` et
  consomme la STL de Microsoft), mais il donnerait **exactement C++23 sur les deux plateformes** au lieu du
  sur-ensemble `/std:c++latest`. Noté dans l'ADR-0011 comme première option à évaluer quand Windows reviendra.
- Observation sur la journée : trois décisions prises par défaut ont été attrapées par Donnovan et transformées
  en choix argumentés — la forme du code (variante C), le périmètre Windows, et le compilateur Windows. Aucune
  n'était signalée comme incertaine dans les ADR d'origine. **À faire systématiquement : marquer dans un ADR ce
  qui est un choix raisonné et ce qui est une convention reprise sans examen.**
- Prochaine étape : M0.3 — logs, assertions, ADR-0008 (gestion d'erreurs), allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.5 — Retour au C++ (ADR-0011)

- Temps Donnovan : **0,25 h** pour 1,0 h estimée (ratio 0,25) — relecture de la PR #33. Issue #35 créée
  rétroactivement pour que le board porte ce temps : le travail avait été décidé en conversation, sans issue.
- Sessions Claude Code : 1
- Contexte : Donnovan revient sur la décision Rust. Sa thèse : ce qu'il trouvait plus lisible venait de la
  **simplicité du langage**, et du C++ écrit en exploitant ses atouts devrait se lire aussi bien. Il précise
  aussi que **le projet est sous Linux** et que Windows n'est pas un sujet pour l'instant.
- **Deux biais reconnus dans l'ADR-0010**, et c'est ce qui rend le revirement fondé :
  1. **La manche 1 comparait flecs à bevy_ecs, pas C++ à Rust.** La propriété que Donnovan a aimée — les
     dépendances dans la signature — appartient à bevy_ecs, pas à Rust. Mon C++ était handicapé par la glu
     flecs (lambda `[](flecs::iter&, size_t, …)`, `it.world().get<>()` caché dans le corps). Une fonction libre
     a exactement la même propriété. **Je n'ai pas écrit le meilleur C++ possible.**
  2. **La manche 3 facturait au C++ un Windows dont le projet n'a pas besoin.** Et surtout : **les deux seuls
     bugs de M0.2 étaient des bugs Windows** (`/Zc:__cplusplus`, `<ostream>` non inclus en cascade par la STL
     de Microsoft). En périmètre Linux, ni l'un ni l'autre n'existe.
- Décisions (ADR-0011, remplace ADR-0010) : retour au C++23 ; **Linux d'abord**, presets et CI Windows retirés,
  M1.4 différé (et non plus supprimé) ; **forme du code fixée** — variante C choisie par Donnovan sur lecture
  de trois variantes : fonctions libres avec toutes les dépendances dans la signature, glu ECS confinée à une
  ligne, chaque piège portant son nom (`normalizeOrZero`, `clampPitch`, `horizontalBasisFrom`).
- Méthode : le revert n'a **pas** annulé `docs/JOURNAL.md` ni l'ADR-0010, restaurés depuis `main`. Un journal
  et une décision sont de l'historique, ils ne se revertent pas.
- Mesures :

  | Critère | Résultat | Commande |
  |---|---|---|
  | Build et sandbox | `Levain 0.1.0 — clang 22.1.8 — __cplusplus 202302` | `cmake --build --preset linux-debug` |
  | Tests | **2 verts** | `ctest --test-dir build/linux-debug` |
  | Format | conforme | `clang-format --dry-run --Werror` |
  | clang-tidy | **0 finding** | `clang-tidy -p build/linux-debug --warnings-as-errors='*'` |
  | Infrastructure | **347 lignes** (contre 435 en v0.3) | `wc -l` sur les 9 fichiers |

- **Correction d'une estimation que j'avais donnée pour une mesure** : j'annonçais « environ 300 lignes » avant
  d'avoir mesuré ; le vrai chiffre est **347**. La moitié des 435 lignes de la v0.3 était du format et du lint,
  indépendants de la plateforme. Ce que le périmètre Linux fait disparaître n'est pas du volume mais **la part
  qui causait les pannes**.
- Écarts et problèmes : les jobs Windows n'existent plus, donc les checks requis par la protection de `main`
  (`linux-debug`, `linux-release`) restent valides sans modification — les noms de presets n'ont pas changé.
  L'issue #32 (réécrire les issues de phase 1) devient sans objet et sera refermée.
- Bilan des deux allers-retours : **le détour par Rust a produit quelque chose**. Sans la comparaison, la
  variante C n'aurait jamais été écrite, et l'ADR-0009 seul n'avait pas suffi à la produire. Règle retenue pour
  la suite : **avant de conclure qu'une alternative est meilleure, vérifier qu'on a écrit la meilleure version
  de ce qu'on compare.**
- Prochaine étape : validation de l'ADR-0011, clôture de M0.5, puis M0.3 — logs, assertions, ADR-0008,
  allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.4 — Socle Rust (issues #27 à #30)

- Temps Donnovan : **0,25 h** pour 1,0 h estimée (ratio 0,25) — relecture de la PR #31. Les quatre issues ayant
  été livrées en une seule PR, le temps est porté sur l'issue #27 au board plutôt que réparti.
- Sessions Claude Code : 1
- Fait : migration complète vers Rust. Workspace cargo, crates `levain-core` et `levain-sandbox`, CI réécrite,
  documentation répercutée, C++ supprimé de l'arbre de travail.
- Mesures :

  | Critère | Résultat | Commande |
  |---|---|---|
  | Le sandbox tourne | `Levain 0.1.0 — linux/x86_64 — rust edition 2024` | `cargo run -p levain-sandbox` |
  | Tests | **2 tests verts** | `cargo test --workspace` |
  | Un code mal formaté fait échouer la CI | code de sortie **1** | `cargo fmt --all --check` |
  | Un défaut clippy fait échouer la CI | **erreur** sur une fonction jamais utilisée | `cargo clippy --workspace --all-targets -- -D warnings` |
  | Infrastructure de build et CI | **136 lignes** contre 435 en C++ | `wc -l` sur Cargo.toml ×3 et ci.yml |

- Durées de CI, premier run, **les 4 jobs verts du premier coup** :

  | Job | Rust (à froid) | C++ à froid | C++ à chaud |
  |---|---:|---:|---:|
  | `linux-debug` | **16 s** | 85 s | 24 s |
  | `linux-release` | **8 s** | 125 s | 24 s |
  | `windows-debug` | **38 s** | 233 s | 52 s |
  | `windows-release` | **34 s** | 288 s | 91 s |
  | **Total** | **96 s** | 731 s | 191 s |

  **Réserve importante : ce n'est pas une comparaison équitable.** Le workspace Rust n'a aujourd'hui
  **aucune dépendance externe**, là où le build C++ compilait nvrhi, flecs et vulkan-headers. L'écart mesuré
  reflète surtout ça. La comparaison honnête viendra quand wgpu et bevy_ecs seront réellement ajoutés (M1.2 et
  M3.1) — et wgpu est un gros crate. Ce que ces chiffres établissent vraiment, c'est que **le chemin Windows a
  fonctionné du premier coup**, sans `vcvars`, sans `vswhere` et sans divergence de bibliothèque standard, là
  où M0.2 avait demandé deux correctifs.

- Outillage : `rustup` installé en mode utilisateur dans `~/.cargo` (pas de sudo), Rust **1.98.1**, avec
  rustfmt et clippy.
- Décisions prises en chemin :
  - **Noms de jobs de CI conservés à l'identique** (`linux-debug`, `linux-release`, `windows-debug`,
    `windows-release`) alors que le profil cargo s'appelle `dev`. Les renommer aurait rendu `main`
    infusionnable : ce sont les checks requis par la protection de branche. Le piège était noté dans l'issue #28,
    et le contourner coûte moins cher que de reconfigurer la protection.
  - **`clippy::all` seulement, pas `pedantic`.** Même raisonnement que pour clang-tidy en M0.2 : on active ce
    qui attrape de vrais défauts. `pedantic` pousse vers `must_use` partout, ce que l'ADR-0009 a écarté sous le
    nom de « décoration maximale ».
  - **Le test de version du C++ n'a pas été porté tel quel.** Il vérifiait que CMake injectait correctement
    `LEVAIN_VERSION` — un câblage qui n'existe plus, `env!("CARGO_PKG_VERSION")` étant automatique. Remplacé par
    un test qui vérifie que la version est un semver à trois composants, ce qui attrape un `Cargo.toml` malformé.
  - **Pas de dossier `tests/` à la racine** : les tests unitaires vivent dans le crate (`#[cfg(test)] mod tests`).
    SPECS §7 mis à jour en conséquence.
- **Erreur corrigée dans l'ADR-0010** : l'en-tête ne listait pas l'ADR-0003 (SDL3) parmi les ADR remplacés, alors
  que la décision remplace bien SDL3 par winit. Relevé en appliquant l'ADR. L'ADR-0003 est passé à « remplacé ».
- Écarts et problèmes : **les issues de la phase 1 nomment encore NVRHI, SDL3 et Slang** (#10, #12 à #17). Elles
  ne sont pas réécrites ici — le périmètre de l'issue #30 s'arrêtait à SPECS, ROADMAP, CLAUDE.md et aux statuts
  d'ADR — mais elles devront l'être **avant d'attaquer M1.1**.
- Roadmap : **M1.4 supprimé**, **M0.4 ajouté**, total inchangé à 68 h. Phase 0 passe de 4,0 à 5,0 h, phase 1 de
  5,5 à 4,5 h.
- Prochaine étape : clôture de M0.4, puis M0.3 — logs, assertions, ADR-0008, allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.2 — Réévaluation du langage : passage à Rust (ADR-0010)

- Temps Donnovan : à renseigner (trois manches de lecture de code + relecture de l'ADR, estimée 0,3 h)
- Sessions Claude Code : 1
- Contexte : Donnovan demande à la clôture de M0.2 pourquoi ne pas passer à Rust, puisqu'il n'écrit pas le code
  et qu'il fait du C++ toute la semaine. L'ADR-0001 prévoyait explicitement cette réévaluation.
- Méthode : trois manches de comparaison sur du code réel plutôt qu'une discussion de principes, comme pour
  l'ADR-0009.
  1. **Transform + déplacement FPS** → Donnovan trouve le **Rust plus lisible sans en avoir jamais lu**, et le
     C++ plus verbeux alors qu'il en fait tous les jours.
  2. **Propagation hiérarchique des transforms** → manche **favorable au C++** : le `cascade()` de flecs tient
     en 10 lignes, la version Rust sûre en demande 20. Bevy utilise `unsafe` dans la sienne (bevy#4697).
  3. **Build et CI** → mesuré sur le dépôt réel : **435 lignes d'infrastructure C++ contre 94 en Rust**, pour
     107 lignes de moteur. Commande : `wc -l` sur ci.yml, CMakePresets.json, les CMakeLists, vcpkg.json,
     .clang-format et .clang-tidy, contre l'équivalent cargo écrit et compté.
- Décision proposée : **ADR-0010**, passage à Rust edition 2024. wgpu, bevy_ecs, winit, glam, rapier3d, gltf,
  WGSL, egui, tracing, tracy-client, kira. Remplace les ADR 0001, 0002, 0004, 0005, 0007 et la moitié nommage
  de l'ADR-0009.
- `bevy_ecs` et non `flecs_ecs` : le binding Rust de flecs existe et couvre les hiérarchies, mais il est
  auto-déclaré **alpha**, maintenu par une personne, à 11 359 téléchargements, et son `World` est
  `!Send`/`!Sync`. On ne pose pas le cœur du moteur dessus.
- Conséquence sur la roadmap : **M1.4 (backend Direct3D 12) disparaît** — wgpu choisit son backend seul.
  1,0 h de budget et une session rendues. Nouveau milestone **M0.4 — Socle Rust** pour refaire l'équivalent
  de M0.2.
- Correction d'une erreur de l'ADR-0001 : « le borrow checker résiste aux graphes d'objets » était vrai en
  général et hors sujet ici, la réponse Rust à ce problème étant l'ECS, qu'on avait déjà choisi.
- Écarts et problèmes : on perd le parcours de lecture Donut, sur lequel l'ADR-0002 et CLAUDE.md étaient
  bâtis, et l'objectif « comprendre Unreal, Unity, Godot » se paie plus cher puisqu'ils sont tous en C++.
  Assumé et écrit dans l'ADR.
- Prochaine étape : validation de l'ADR-0010 par Donnovan. **Rien n'est migré tant qu'il n'est pas accepté.**
  Ensuite : ROADMAP et SPECS mis à jour, milestone M0.4 créé, puis socle Rust.

## 2026-09-20 — M0.2 — clang-tidy, doctest et protection de main (issue #5)

- Temps Donnovan : à renseigner (relecture estimée 0,3 h)
- Sessions Claude Code : 1
- Fait : `.clang-tidy` (nommage de l'ADR-0009 + bugprone, performance, quelques modernize), doctest par vcpkg
  avec 2 cas découverts individuellement, `ctest` branché, contrôle de format et analyse statique en CI,
  protection de `main`.
- Mesures — les trois critères de l'issue :

  | Critère | Résultat | Commande |
  |---|---|---|
  | Un code mal formaté fait échouer la CI | code de sortie **123** sur format invalide, **0** sinon | `find … \| xargs clang-format --dry-run --Werror` |
  | `ctest` lance au moins un test | **2 tests**, verts | `ctest --test-dir build/linux-debug` |
  | Push direct sur `main` refusé | **refusé** (voir plus bas) | `git push origin main` |

  Le test sait échouer : `LEVAIN_EXPECTED_VERSION` forcé à `9.9.9` → `50% tests passed, 1 tests failed`.

- Durées de CI avec LLVM 22 et les tests : `linux-debug` 69 s, `linux-release` 56 s, `windows-debug` 289 s,
  `windows-release` 278 s. L'installation de LLVM 22 coûte une trentaine de secondes aux jobs Linux par rapport
  au run précédent (24 s), pour la garantie que clang-format et clang-tidy sont les mêmes qu'en local.

- **Deux vraies trouvailles, aucune stylistique.**

  1. **clang-tidy, première exécution** : `std::print` peut lever et `main` laissait l'exception s'échapper, ce
     qui appelle `std::terminate`. Corrigé dans `main`, pas désactivé (règle n°4). La politique générale reste
     l'affaire de l'ADR-0008 en M0.3.
  2. **CI Windows** : `version_test.cpp` ne compilait pas sous MSVC alors que Linux était vert. Pour afficher la
     valeur d'un `CHECK` qui échoue, doctest instancie `operator<<` vers un `ostream` ; la STL de Microsoft
     déclare cet opérateur pour `std::string_view` **sans inclure `<ostream>` en cascade**, là où libstdc++ le
     fait. Réglé par un `#include <ostream>`. **Première divergence de plateforme du projet**, sur 20 lignes de
     test, et détectable uniquement par le job Windows — celui qu'on avait gardé non bloquant « au cas où ».

- Correction d'une erreur d'analyse de ma part : j'avais justifié Clang 18 en CI par « la CI est plus
  conservatrice ». C'est faux — Clang 18 n'est pas plus strict, il est moins complet, et surtout clang-format 18
  et 22 ne produisent pas la même sortie. Un contrôle de format en 18 aurait rejeté des fichiers corrects
  formatés en 22. LLVM 22 installé en CI via `apt.llvm.org`, `LLVM_VERSION` dans le workflow à garder égal à la
  machine de référence (SPECS §10).

- Protection de `main` : PR obligatoire, `linux-debug` et `linux-release` requis, force-push et suppression
  interdits, **`enforce_admins` activé**. Les jobs Windows sont volontairement **hors des checks requis** :
  ils sont `continue-on-error`, les inscrire annulerait ce compromis. À ajouter le jour où ils deviendront
  bloquants.

- Écarts et problèmes : `enforce_admins: true` s'applique aussi à Donnovan. Pour lever la protection en cas de
  besoin : `gh api -X DELETE repos/PhantomDO/Levain/branches/main/protection`. Conséquence pour les sessions
  suivantes : **plus aucun push direct sur `main`**, y compris pour une entrée de journal.

- `bugprone-easily-swappable-parameters` est le seul check écarté : il signalerait
  `allocate(size_t size, size_t alignment)` et à peu près toute l'API d'un moteur.

- Prochaine étape : clôture de M0.2, puis M0.3 — logs, assertions, ADR-0008, allocateurs, Tracy, étude E0.

## 2026-09-20 — M0.2 — CI Windows + Linux (issue #4)

- Temps Donnovan : à renseigner (relecture estimée 0,25 h)
- Sessions Claude Code : 1
- Fait : `.github/workflows/ci.yml` — matrice de 4 jobs (Debug et Release × `ubuntu-latest` et
  `windows-latest`), Ninja partout, vcpkg cloné au tag `2026.07.29` et cache binaire via `actions/cache`.
  Jobs Windows en `continue-on-error`. **Les 4 jobs sont verts au premier run.**
- Mesures — durées de CI, à froid puis à chaud (cache vcpkg peuplé) :

  | Job | À froid | À chaud | Gain |
  |---|---:|---:|---:|
  | `linux-debug` | 85 s | **24 s** | −72 % |
  | `linux-release` | 125 s | **24 s** | −81 % |
  | `windows-debug` | 233 s | **52 s** | −78 % |
  | `windows-release` | 288 s | **91 s** | −68 % |
  | **Total** | **731 s** | **191 s** | **−74 %** |

  Commande : `gh api repos/PhantomDO/Levain/actions/runs/<id>/jobs`, différence entre `started_at` et
  `completed_at`. Runs `35518846705` (froid) et `35519196360` (chaud).

  Windows coûte 2 à 3 fois plus cher que Linux, à froid comme à chaud.

- **Confirmation empirique de l'ADR-0001.** Le même sandbox, mêmes sources, annonce :
  `Linux / Clang 18.1.3 / -std=c++23 → __cplusplus 202302` et
  `Windows / MSVC 19.51.36256 / /std:c++latest → __cplusplus 202400`.
  `202400` n'est la valeur d'aucune norme publiée : c'est un mode brouillon post-C++23. Le raisonnement de
  l'ADR-0001 est donc vérifié par la mesure, et le `-pedantic-errors` côté Linux n'est pas une précaution
  théorique.
- Écarts et problèmes :
  - **Erreur de ma part** : le premier run annonçait `__cplusplus 199711` sous Windows. MSVC épingle cette macro
    à la valeur de C++98 sauf si on passe `/Zc:__cplusplus`. La doc Microsoft que j'avais lue en écrivant
    l'ADR-0001 le dit explicitement ; je ne l'avais pas appliqué. Corrigé (`19b5fbc`), avec le constat écrit
    dans le commentaire du `CMakeLists.txt`.
  - **La CI Linux tourne sur Clang 18.1.3, la machine de référence sur Clang 22.1.8** — quatre versions
    majeures d'écart. L'écart va dans le sens le moins dangereux (la CI est plus conservatrice que la machine
    de dev), mais du code qui compile localement peut casser en CI. Laissé tel quel faute de cas concret ;
    à rouvrir si ça mord. Alternative : installer Clang 22 via `apt.llvm.org`, ~20 s par job.
  - **Pas de `ctest`** : aucun test n'existe encore. Test de fumée (lancement du sandbox) en attendant
    doctest, issue #5.
  - **`VCPKG_TAG` et `builtin-baseline` ne sont pas couplés automatiquement** : deux valeurs écrites à la main
    dans deux fichiers. Cohérentes aujourd'hui, vérifiées manuellement.
- Prochaine étape : issue #5 — clang-tidy (nommage de l'ADR-0009), doctest et `ctest`, vérification du format
  en CI, protection de `main`.

## 2026-09-20 — M0.2 — Norme de style C++ (ADR-0009)

- Temps Donnovan : 0,25 h de relecture de la PR #21 (portée au board), plus le choix de style
- Sessions Claude Code : 1
- Contexte : Donnovan demande une norme d'écriture fixée pour tout le projet. Le C++ propose plusieurs
  conventions incompatibles et il relit tout avec 1 à 2 h par semaine : sans règle, chaque fichier dérive et la
  relecture coûte de l'attention pour rien. Le dépôt comptait 27 lignes de code — aucune conversion à faire.
- Méthode : trois variantes complètes du même allocateur linéaire (le vrai, issue #7) soumises en lecture
  plutôt qu'en discussion. Choix fait sur le code, pas sur des principes.
- Décisions (ADR-0009) : **variante C** — types `PascalCase`, fonctions et variables `camelCase`, membres
  `m_` ; Allman, 4 espaces, 100 colonnes ; décoration **modérée** (`[[nodiscard]]` seulement quand ignorer le
  retour est un bug, `noexcept` seulement quand c'est garanti) ; commentaires **en français**.
- SPECS §8 amendé : les commentaires de code passent officiellement en français. J'étais déjà en infraction
  dans la PR #21 sans l'avoir signalé — corrigé.
- Mesures : `.clang-format` appliqué aux 3 fichiers existants, build et exécution vérifiés après reformatage
  (`clang-format -i` puis `cmake --build --preset linux-debug`), puis `clang-format --dry-run --Werror` passe
  sur les 3 fichiers.
- Écarts et problèmes : clang-format **ne vérifie pas le nommage** — c'est clang-tidy
  (`readability-identifier-naming`) qui s'en chargera dans l'issue #5. Tant que ce n'est pas en place, la moitié
  nommage de l'ADR-0009 repose sur ma discipline, pas sur l'outil.
- Décision de périmètre : la CI Windows de l'issue #4 sera **non bloquante** au début. Donnovan n'a pas de
  machine Windows ; les runners GitHub n'en demandent pas non plus, et c'est le seul endroit qui vérifiera le
  pari `/std:c++latest` de l'ADR-0001. Job gardé, `continue-on-error`, rendu bloquant quand une machine sera
  disponible.
- Prochaine étape : issue #4 (CI), puis #5 (clang-tidy, doctest, protection de `main`).

## 2026-09-20 — M0.2 — Squelette de build (issue #3)

- Temps Donnovan : à renseigner (relecture estimée 0,3 h)
- Sessions Claude Code : 1
- Fait : `CMakeLists.txt` racine, `CMakePresets.json` (4 presets, Ninja), `vcpkg.json` avec baseline figée sur la
  release vcpkg `2026.07.29` (`9e593bb…`), bibliothèque `levain_core` et exécutable `levain_sandbox`,
  `engine/core/README.md`, section « Commandes de build » de CLAUDE.md complétée.
- Mesures :
  - Build complet depuis zéro (Clang 22.1.8, Debug, sans vcpkg) → **4 étapes Ninja**, sandbox lancé :
    `Levain 0.1.0 — clang 22.1.8 — __cplusplus 202302`.
    Commande : `cmake -S . -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++`
  - Garde-fou C++23 vérifié **dans le vrai build** : `compile_commands.json` contient bien `-std=c++23`
    (et non `gnu++23`) et `-pedantic-errors` ; une indexation de paquets C++26 insérée dans `main.cpp` fait
    échouer la compilation avec `error: pack indexing is a C++2c extension`.
  - `CMAKE_CXX_SCAN_FOR_MODULES OFF` : le build passe de **8 à 4 étapes** Ninja. CMake activait le scan de
    modules C++20 par défaut alors que l'ADR-0001 dit « sans modules » — deux étapes par fichier pour un
    résultat toujours vide.
- Décisions : pas d'ADR, rien de structurant. Deux choix signalés en PR (arborescence partielle, dépendances
  déclarées non liées).
- Écart levé le jour même : `zip` installé par Donnovan, vcpkg bootstrappé
  (`2026-07-27-98d7cb0c`) dans `~/vcpkg`. Le critère **« build depuis un clone propre en deux commandes » est
  vérifié sur Linux**, sur un vrai `git clone` du dépôt public :

  | Mesure | Valeur | Détail |
  |---|---:|---|
  | `cmake --preset linux-debug` **à froid** | **25 s** | dont 23 s de vcpkg compilant `nvrhi` (2026-02-26), `flecs` et `vulkan-headers` depuis les sources |
  | `cmake --build --preset linux-debug` | **1 s** | 4 étapes Ninja |
  | `cmake --preset linux-debug` **à chaud** (2ᵉ clone) | **1 s** | vcpkg résout en **16,5 ms** depuis le cache binaire |
  | Taille du cache binaire vcpkg | **19 Mo** | `~/.cache/vcpkg` |

  Commandes : `git clone https://github.com/PhantomDO/Levain.git <dir> && cd <dir> && VCPKG_ROOT=~/vcpkg
  cmake --preset linux-debug && VCPKG_ROOT=~/vcpkg cmake --build --preset linux-debug`

  Sortie du sandbox : `Levain 0.1.0 — clang 22.1.8 — __cplusplus 202302`. `compile_commands.json` : 2 entrées,
  flags `-std=c++23 -pedantic-errors`.

  **Conséquence pour l'issue #4** : 19 Mo de cache pour un gain de 23 s à chaque exécution. Le cache binaire
  vcpkg vaut clairement le coup en CI, et il tient largement dans les quotas du cache GitHub Actions.
- Note pour M1.2 : le port vcpkg de `nvrhi` indique qu'il faut lier `nvrhi` **et** `nvrhi_vk` sous Linux
  (build statique), `nvrhi` seul en build partagé. Les cibles `nvrhi_d3d11`/`nvrhi_d3d12` n'existent que sous
  Windows.
- Prochaine étape : issue #4 (CI Windows + Linux avec cache vcpkg), puis #5 (clang-format, clang-tidy, doctest,
  protection de `main`). Reste non vérifié : le chemin **Windows**, qui n'existe que sur le papier tant que la CI
  n'a pas tourné — c'est l'objet de #4.

## 2026-09-20 — M0.1 — Clôture du milestone

- Temps Donnovan : 1,25 h (estimé 1,0 h — **ratio 1,25**)
- Sessions Claude Code : 1
- Définition de « terminé » (SPECS §9) : 4 critères sur 7 s'appliquent et sont remplis (mesures consignées,
  board à jour, étude E1 écrite d'avance, docs à jour). Trois ne s'appliquent pas encore, faute de code :
  démo `sandbox/`, CI verte, binaires de Release.
- Fait : tag `m0.1` posé et poussé. **Pas de GitHub Release** — §9 la prévoit pour livrer les binaires et les
  mesures de la démo, il n'y en a aucun à ce stade. Le premier Release sera celui de M0.2, qui aura une CI.
  Milestone n°1 fermé.
- Écarts et problèmes : le dépassement de 0,25 h vient entièrement de l'amendement C++23, hors estimation
  initiale. À surveiller sur M0.2 et M0.3 : si le ratio 1,25 se confirme, la roadmap passe de 68 h à ~85 h et il
  faudra recalibrer (ROADMAP, section « Recalibrage »).
- Prochaine étape : **M0.2 — Squelette de build et CI** (3 issues, 1,5 h estimée). Deux acquis de M0.1 à
  reprendre : `cxx_std_23` avec `-pedantic-errors` sur le job Linux comme garde-fou de conformité (ADR-0001), et
  la préférence GPU discret à prévoir pour M1.2 (SPECS §10).

## 2026-09-20 — M0.1 — Le moteur s'appelle Levain

- Temps Donnovan : inclus dans l'heure de M0.1
- Sessions Claude Code : 1
- Fait : nom choisi, **Levain**. Dépôt renommé `PhantomDO/GameEngine` → `PhantomDO/Levain` (GitHub redirige
  l'ancienne URL), board renommé « Levain — Roadmap », SPECS en v0.3, README, CLAUDE.md et SETUP mis à jour.
  Namespace racine `levain`, cibles CMake préfixées `levain_`.
- Décisions : le namespace français est une **exception assumée** à la règle « identifiants en anglais »
  (SPECS §8) — nom propre, comme Godot. Notée dans les conventions plutôt que subie.
- Écarts et problèmes : le dossier local est encore `~/Projects/GameEngine`. À renommer entre deux sessions
  (`mv ~/Projects/GameEngine ~/Projects/Levain`), pas pendant, pour ne pas casser la session en cours.
- Prochaine étape : M0.1 close. M0.2 — arborescence, presets CMake avec `cxx_std_23`, vcpkg en manifeste, CI
  avec `-pedantic-errors` sur le job Linux.

## 2026-09-20 — M0.1 — Passage à C++23 (ADR-0001 amendé)

- Temps Donnovan : 1,0 h (estimé 1,0 h pour M0.1 — ratio 1,0), porté sur le board (0,75 h sur l'issue #1,
  0,25 h sur l'issue #2)
- Sessions Claude Code : 1
- Contexte : Donnovan valide SPECS, ROADMAP et les ADR, et demande de passer à C++23 si la norme est stable.
  La v1 de l'ADR-0001 prévoyait exactement cette réévaluation.
- Mesures :
  - Sonde de macros de test de fonctionnalité, `-std=c++23` : Clang 22.1.8 et GCC 16.2.1 (libstdc++ 16) →
    `__cplusplus = 202302`, 19 fonctionnalités C++23 sur 19 présentes.
  - Garde-fou conformité : `clang++ -std=c++23 -pedantic-errors` et `g++ -std=c++23 -pedantic-errors` refusent
    bien l'indexation de paquets C++26 (`P2662`), acceptée en `-std=c++26`.
- Décisions : ADR-0001 amendé, C++20 → **C++23**, fichier renommé `0001-langage-cpp23.md`. Sans modules,
  inchangé.
- Écarts et problèmes : **MSVC n'a pas de `/std:c++23`**, ni en VS 2022 ni en VS 2026 — seulement
  `/std:c++23preview` (ABI non garantie) et `/std:c++latest` (sur-ensemble débordant sur C++26). CMake 4.4 mappe
  `CMAKE_CXX_STANDARD 23` vers `-std:c++latest` chez MSVC. Conséquence : la CI Linux en `-pedantic-errors` fait
  autorité sur la conformité, à mettre en place en M0.2. Trous MSVC à éviter : `[[assume]]` (P1774R8), P2448R2,
  P2582R1, échappements Unicode.
- Relecture : PR #20 relue et fusionnée en **0,25 h** (15 min). ADR-0001 accepté, et avec lui les ADR 0002 à
  0007 (« ça me va »). SPECS passe en v0.3, statut « validé ».
- Cumul M0.1 : **1,25 h passée pour 1,0 h estimée** (ratio 1,25) — l'amendement C++23 n'était pas prévu
  dans l'estimation initiale.
- Prochaine étape : M0.2 — `cxx_std_23` dans les presets CMake et `-pedantic-errors` en CI Linux comme
  garde-fou de conformité, en plus de l'arborescence, de vcpkg et de la CI.

## 2026-09-20 — M0.1 — Dépôt local, modèles GitHub et machine de référence

- Temps Donnovan : à renseigner
- Sessions Claude Code : 1
- Fait : modèles d'issue et de PR déplacés de `tools/github-templates/` vers `.github/` ; dépôt git local initialisé
  (`main`) et premier commit ; SPECS §10 complété à partir de la machine.
- Mesures : machine de référence → CachyOS noyau 7.2.6-1-cachyos, Ryzen 7 7800X3D, RX 9070 XT (GFX1201),
  Mesa RADV 26.2.3-arch3.1, Vulkan 1.4.354, 16 Go (`vulkaninfo --summary`, `uname -r`, `/proc/cpuinfo`,
  `/proc/meminfo`). Outils présents : CMake 4.4.3, Ninja 1.13.2, Clang 22.1.8, GCC 16.2.1, git 2.55.0, gh 2.101.0.
- Écarts et problèmes : deux GPU Vulkan sur la machine (le discret et l'iGPU du 7800X3D) — la sélection de device
  en M1.2 devra préférer le discret ; plusieurs couches Vulkan implicites tierces sont installées et celle de
  Lossless Scaling est cassée (erreur du loader à chaque `vkCreateInstance`). Les deux points sont notés
  dans SPECS §10.
- Décision : licence **MIT** (`LICENSE`), retirée des questions ouvertes de SPECS §11 et inscrite dans les
  conventions (SPECS §8). Compatible avec le code adapté de Donut, qui garde son en-tête MIT.
- Suivi GitHub créé par `./tools/github-bootstrap.sh GameEngine` : dépôt public
  [PhantomDO/GameEngine](https://github.com/PhantomDO/GameEngine), 17 labels, 35 milestones (M0.1 échéance
  27/09/2026 → M8.3 échéance 08/08/2027), board n°1 avec les champs Estimé (h), Passé (h) et Phase, et les
  19 issues des phases 0 et 1, chacune avec son milestone, ses labels, son estimation et sa phase.
- Prochaine étape : M0.2 — arborescence, presets CMake, vcpkg en mode manifeste, CI, puis protection de
  `main` (issue #5). Les issues #1 et #2 (M0.1) attendent Donnovan.

## 2026-09-20 — M0.1 — Machine de référence et vérification Windows

- Temps Donnovan : à renseigner
- Fait : machine de référence renseignée (CachyOS, Ryzen 7 7800X3D, Radeon RX 9070 XT, 16 Go) ; versions d'OS et
  de pilote à compléter avec `vulkaninfo --summary`.
- Décision : pas de machine Windows ; vérification en trois niveaux (CI + WARP, binaire Windows sous Proton sur la
  machine de référence, vraie machine Windows ponctuellement). Une VM Windows écartée : sans passthrough GPU, elle
  n'apporte rien de plus que WARP.
- Dépôt GitHub : pas encore créé ; Claude Code le créera avec `tools/github-bootstrap.sh` après la connexion de
  Donnovan à `gh`. Nom provisoire : GameEngine.

## 2026-09-20 — M0.1 — Révision : NVRHI et flecs

- Temps Donnovan : à renseigner (échange vocal + relecture)
- Sessions Claude Code : 0
- Contexte : Donnovan précise que **la priorité est de faire un jeu avec notre moteur**, et qu'apprendre Vulkan en
  détail ne l'intéresse pas.
- Décisions proposées : NVRHI à la place d'une RHI maison, backends Vulkan + Direct3D 12 (ADR-0002, réécrit) ;
  flecs plutôt qu'EnTT ou un ECS maison, pour sa documentation, son explorer, ses hiérarchies, ses pipelines et sa
  réflexion JSON (ADR-0004, réécrit) ; Slang compilé en SPIR-V et DXIL (ADR-0005, révisé).
- Correction apportée : NVRHI n'est pas la norme de l'industrie (Unreal, Unity et Godot ont leur propre RHI) et
  ne gère ni OpenGL ni Metal (backends : D3D11, D3D12, Vulkan).
- Roadmap v0.2 : 77 h → 68 h Donnovan, 55 → 49 sessions, 35 milestones ; « RHI mince » supprimé ; ajout de M1.4
  (backend D3D12) et M3.5 (choix du jeu) ; le jeu jouable visé début août 2027 à 1,5 h/semaine.
- Ajouts : étude E1 (couches RHI) écrite en avance, `docs/LECTURES.md` (lectures commentées).
- Prochaine étape : inchangée (validation des specs, puis création du dépôt et M0.2).

## 2026-09-20 — M0.1 — Specs initiales

- Temps Donnovan : à renseigner (estimé 1,0 h pour tout M0.1)
- Sessions Claude Code : 0 (rédaction dans une conversation claude.ai)
- Fait : SPECS v0.1, ROADMAP v0.1 (77 h Donnovan, 55 sessions, 34 milestones), ADR 0001 à 0007, CLAUDE.md,
  modèles d'issue et de PR, script `tools/github-bootstrap.sh`.
- Décisions proposées : C++20 sans modules, Vulkan 1.3 avec RHI maison, SDL3, ECS maison à sparse sets, Slang,
  GitHub public, CMake + vcpkg.
- Prochaine étape : Donnovan valide ou amende les specs et les ADR, renseigne la machine de référence, choisit
  le nom ; puis création du dépôt avec le script et démarrage de M0.2 dans Claude Code.
