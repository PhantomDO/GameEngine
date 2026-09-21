# Compare le filtrage trilinéaire et le filtrage anisotrope sur le sol du sandbox (critère de M2.2, #44).
#
# Capture une frame du sandbox avec --anisotropy 1 (trilinéaire seul), puis avec --anisotropy 16, et
# enregistre l'image finale de chacune. Depuis la racine du dépôt, après un build linux-debug :
#
#   QT_QPA_PLATFORM=offscreen qrenderdoc --python tools/renderdoc-anisotropy.py
#
# Sorties dans captures/ : m2.2-aniso<N>_frame<M>.rdc, m2.2-aniso<N>.png et renderdoc-anisotropy.log.

import os
import sys

sys.path.insert(0, os.path.join(os.getcwd(), "tools"))  # qrenderdoc ne l'ajoute pas lui-même

import renderdoc as rd  # noqa: E402
import renderdoc_capture as capture  # noqa: E402

LEVELS = (1, 16)


def last_draw(actions):
    """Le dernier draw de la frame, en descendant dans les groupes d'actions."""
    found = None
    for action in actions:
        if action.flags & rd.ActionFlags.Drawcall:
            found = action
        found = last_draw(action.children) or found
    return found


def main():
    for level in LEVELS:
        name = f"m2.2-aniso{level}"
        path = capture.capture_frame(f"--seconds 6 --anisotropy {level}", name)
        replay, controller = capture.open_replay(path)

        # Après le dernier draw, l'image de la swapchain est complète : c'est ce que l'écran affiche.
        draw = last_draw(controller.GetRootActions())
        if draw is None:
            capture.fail(f"{path} : aucun draw")
        controller.SetFrameEvent(draw.eventId, True)
        output = os.path.join(capture.OUTPUT, f"{name}.png")
        capture.save_png(controller, draw.outputs[0], output)
        print(f"anisotropie {level} : {path} → captures/{name}.png", flush=True)

        capture.close_replay(replay, controller)


capture.redirect_output("renderdoc-anisotropy.log")
capture.run(main)
