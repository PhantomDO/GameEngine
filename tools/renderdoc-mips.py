# Vérifie dans une capture RenderDoc les niveaux de mip de la texture « checker » (critère de M2.2, #43).
#
# Capture une frame du sandbox, puis en extrait chaque niveau de mip de la texture, tel que le GPU l'a reçu.
# Depuis la racine du dépôt, après un build linux-debug :
#
#   QT_QPA_PLATFORM=offscreen qrenderdoc --python tools/renderdoc-mips.py
#
# Sorties dans captures/ : m2.2_frame<N>.rdc, m2.2-mip<N>.png pour chaque niveau, et renderdoc-mips.log. Le
# code de sortie dit si la vérification a réussi.

import os
import sys

sys.path.insert(0, os.path.join(os.getcwd(), "tools"))  # qrenderdoc ne l'ajoute pas lui-même

import renderdoc_capture as capture  # noqa: E402

TEXTURE_NAME = "checker"


def main():
    path = capture.capture_frame("--seconds 6", "m2.2")
    print(f"capture : {path}", flush=True)
    replay, controller = capture.open_replay(path)

    # Le nom vient du debugName de NVRHI, transmis au pilote par VK_EXT_debug_utils.
    names = {resource.resourceId: resource.name for resource in controller.GetResources()}
    textures = [texture for texture in controller.GetTextures()
                if names.get(texture.resourceId) == TEXTURE_NAME]
    if len(textures) != 1:
        capture.fail(f"{len(textures)} texture(s) nommée(s) « {TEXTURE_NAME} » dans la capture, 1 attendue")
    texture = textures[0]

    # Le contenu tel qu'il est à la fin de la frame capturée.
    controller.SetFrameEvent(controller.GetRootActions()[-1].eventId, True)
    for mip in range(texture.mips):
        output = os.path.join(capture.OUTPUT, f"m2.2-mip{mip}.png")
        capture.save_png(controller, texture.resourceId, output, mip)
        width, height = max(texture.width >> mip, 1), max(texture.height >> mip, 1)
        print(f"niveau {mip} : {width} × {height} → captures/m2.2-mip{mip}.png", flush=True)

    capture.close_replay(replay, controller)


capture.redirect_output("renderdoc-mips.log")
capture.run(main)
