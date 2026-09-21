# `engine/platform`

## Rôle

La frontière avec le système d'exploitation : fenêtre, événements, et plus tard l'input brut (clavier, souris,
manettes). C'est le **seul module qui inclut SDL3** ([ADR-0003](../../docs/adr/0003-plateforme-sdl3.md)) ; le
reste du moteur ne voit que nos propres types.

**État en M1.1** : une fenêtre et ses événements (fermeture, redimensionnement, masquée, visible).

## Invariants

1. **Aucun en-tête SDL dans l'API.** `window.hpp` ne connaît SDL que par la déclaration anticipée
   `struct SDL_Window;`, et SDL3 est lié en `PRIVATE`. Seule exception prévue : `gpu/` inclura SDL en M1.2 pour
   créer la surface Vulkan à partir de `Window::handle`.
2. **Une seule fenêtre à la fois.** La fenêtre possède SDL : la détruire appelle `SDL_Quit`. Une assertion le
   vérifie dans `createWindow`. À revoir quand l'éditeur ouvrira des fenêtres secondaires (M7.1).

## Points d'entrée

| Fichier | Contenu |
|---|---|
| [`include/levain/platform/window.hpp`](include/levain/platform/window.hpp) | `createWindow`, `pollEvents`, `waitEvents` |

## Trois choses à savoir sur les fenêtres

**Masquée, pas minimisée.** Sous Wayland, une application n'apprend jamais qu'elle a été minimisée : le
protocole ne le lui dit pas. Le compositeur peut seulement la déclarer « suspendue », ce que SDL traduit en
`OCCLUDED`. Nos événements s'appellent donc `Hidden` et `Shown`, et répondent à la seule question qui compte pour
le moteur : **y a-t-il quelque chose à dessiner ?** Quand la réponse est non, la boucle appelle `waitEvents` et
dort. Mesuré avec `tools/kwin-window-smoke.sh` : **0 ms de CPU en 2 s minimisée, contre 2 010 ms visible**.

**Sous Wayland, pas d'image, pas de fenêtre.** Une surface Wayland n'apparaît à l'écran qu'après avoir reçu son
premier buffer. Tant que le moteur ne présente aucune image (avant M1.2), la fenêtre existe pour SDL mais pas
pour le compositeur : on ne la voit pas. Pour tester en attendant, `SDL_VIDEO_DRIVER=x11` passe par XWayland,
où une fenêtre vide s'affiche.

**Des pixels, pas des points.** Sur un écran à 200 %, une fenêtre de 1280 × 720 points fait 2560 × 1440 pixels.
`createWindow` prend des points, parce que c'est le compositeur qui applique l'échelle ; `Resized` donne des
pixels, parce que c'est ce dont la swapchain aura besoin.

## Pièges connus (SDL 3.4.12)

| Piège | Symptôme | Parade |
|---|---|---|
| Deux signaux rapprochés (`SDL_quit.c:171`) | Une assertion du SDL compilé en Debug saute, et SDL ouvre une boîte de dialogue qui attend une réponse. | `timeout --foreground`, qui n'envoie qu'un signal. Ctrl+C n'en envoie qu'un par appui. |

## Équivalents ailleurs

| Moteur | Module | Ce qu'on y trouve |
|---|---|---|
| **Unreal** | `ApplicationCore` | `FGenericApplication` et `FGenericWindow`, une implémentation par plateforme. Sous Linux, `FLinuxApplication` est construit sur SDL : Unreal fait exactement le choix de l'ADR-0003 (**documenté** : sources publiques, `Runtime/ApplicationCore/Private/Linux/`). |
| **Godot** | `DisplayServer` | Une classe par système (`DisplayServerX11`, `DisplayServerWayland`). La boucle principale saute le rendu quand aucune fenêtre ne peut dessiner (`can_any_window_draw`, `main/main.cpp`) : c'est notre `Hidden` (**documenté** : dépôt public). |
| **Unity** | — | Côté C#, `OnApplicationFocus`, `OnApplicationPause` et `Application.runInBackground` exposent le même besoin. L'implémentation C++ n'est pas publique. |
